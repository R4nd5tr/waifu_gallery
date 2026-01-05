/*
 * Waifu Gallery - A anime illustration gallery application.
 * Copyright (C) 2025 R4nd5tr <r4nd5tr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pic_tagger.h"
#include "utils/logger.h"

PicTagger::PicTagger(const std::string& databaseFile) {
    databaseFileStr = databaseFile;
    if (!taggerLoader.load("./model/AutoTagger.dll")) {
        Error() << "Failed to load AutoTagger DLL.";
        return;
    }
    tagger = taggerLoader.getTagger();
    if (!tagger) {
        Error() << "Failed to create AutoTagger instance.";
        return;
    }

    // start analyze thread
    analyzeThread = std::thread(&PicTagger::analyzeThreadFunc, this);

    // start preprocess workers
    const size_t numWorkers = std::thread::hardware_concurrency();
    for (size_t i = 0; i < numWorkers; ++i) {
        preprocessWorkers.emplace_back(&PicTagger::preprocessWorkerFunc, this);
    }
}
PicTagger::~PicTagger() {
    stopFlag.store(true);
    readyCv.notify_all();
    preprocessedCv.notify_all();

    if (analyzeThread.joinable()) {
        analyzeThread.join();
    }
    for (auto& worker : preprocessWorkers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
void PicTagger::preprocessWorkerFunc() {}
void PicTagger::analyzeThreadFunc() {
    PicDatabase threadDb(databaseFileStr, DbMode::Import);
    picFilesForTagging = threadDb.getUntaggedPics();
    readyFlag.store(true);
    readyCv.notify_all();
}