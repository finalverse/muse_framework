/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Finalverse Song Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 Finalverse Inc.
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

#include <gtest/gtest.h>

#include "audio/engine/platform/general/generalaudioworker.h"

using namespace muse::audio::engine;

TEST(Audio_GeneralAudioWorkerTests, CanStopImmediatelyAfterRun)
{
    for (int i = 0; i < 100; ++i) {
        GeneralAudioWorker worker;
        worker.run([]() {});
        worker.stop();

        EXPECT_FALSE(worker.isRunning());
    }
}
