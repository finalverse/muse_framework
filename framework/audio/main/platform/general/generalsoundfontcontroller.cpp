/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "generalsoundfontcontroller.h"

#include <thread>

#include "global/async/async.h"
#include "global/io/path.h"
#include "global/runtime.h"

#include "audio/common/rpc/rpcpacker.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::rpc;
using namespace muse::audio::synth;

void GeneralSoundFontController::loadSoundFonts()
{
    configuration()->soundFontDirectoriesChanged().onReceive(this, [this](const io::paths_t&) {
        doLoadSoundFonts();
    });

    doLoadSoundFonts();
}

void GeneralSoundFontController::doLoadSoundFonts()
{
    TRACEFUNC;

    io::paths_t dirs = configuration()->soundFontDirectories();
    std::shared_ptr<io::IFileSystem> fs = fileSystem();
    std::shared_ptr<ScanState> scanState = m_scanState;
    const uint64_t generation = ++scanState->generation;
    const std::thread::id mainThreadId = runtime::mainThreadId();
    std::weak_ptr<GeneralSoundFontController> weakSelf = weak_from_this();

    // User-configured SoundFont directories can live in Documents or on a
    // network/iCloud volume. Scanning them synchronously can block the main
    // thread (and therefore score opening) while macOS resolves the volume or
    // asks for permission.
    std::thread([weakSelf, scanState, generation, mainThreadId, dirs = std::move(dirs), fs = std::move(fs)]() mutable {
        static const std::vector<std::string> filters = { "*.sf2", "*.sf3" };

        std::vector<io::path_t> paths;
        for (const io::path_t& dir : dirs) {
            RetVal<io::paths_t> soundFonts = fs->scanFiles(dir, filters);
            if (!soundFonts.ret) {
                LOGE() << soundFonts.ret.toString();
                continue;
            }

            paths.insert(paths.end(), soundFonts.val.begin(), soundFonts.val.end());
        }

        if (scanState->generation != generation) {
            return;
        }

        // Keep the controller alive while registering the main-thread call.
        // Asyncable then cancels the queued callback if the controller is
        // destroyed before delivery.
        std::shared_ptr<GeneralSoundFontController> self = weakSelf.lock();
        if (!self) {
            return;
        }

        async::Async::call(self.get(), [weakSelf, scanState, generation, paths = std::move(paths)]() mutable {
            if (scanState->generation != generation) {
                return;
            }

            if (std::shared_ptr<GeneralSoundFontController> self = weakSelf.lock()) {
                self->loadSoundFonts(paths);
            }
        }, mainThreadId);
    }).detach();
}

void GeneralSoundFontController::loadSoundFonts(const std::vector<io::path_t>& paths)
{
    std::vector<synth::SoundFontUri> uris;
    uris.reserve(paths.size());
    for (const io::path_t& p : paths) {
        uris.push_back(synth::SoundFontUri::fromLocalFile(p));
    }
    channel()->send(rpc::make_request(rpc::GLOBAL_CTX_ID, MsgCode::LoadSoundFonts, RpcPacker::pack(uris)));
}

void GeneralSoundFontController::addSoundFont(const synth::SoundFontUri& uri)
{
    channel()->send(rpc::make_request(rpc::GLOBAL_CTX_ID, MsgCode::AddSoundFont, RpcPacker::pack(uri)));
}
