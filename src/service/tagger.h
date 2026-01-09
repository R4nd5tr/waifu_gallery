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

#pragma once
#include "database.h"
#include "utils/autotagger_loader.h"
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>

const int MAX_PREPROCESS_QUEUE_SIZE = 32;

class PicTagger {
public:
    PicTagger(const std::string& databaseFile = DEFAULT_DATABASE_FILE);
    ~PicTagger();

    std::vector<std::filesystem::path> getExistingTaggerDLLs() { return taggerLoader.discoverAutoTaggers(); }
    bool loadTaggerDLL(const std::filesystem::path& dllPath);
    bool taggerLoaded() const { return tagger != nullptr; }
    bool gpuAvailable() const { return tagger ? tagger->gpuAvailable() : false; }
    void loadTagSetToDatabase();
    void startTagging();
    void forceStop();
    bool isFinished() const { return finished.load(); }

private:
    std::string databaseFileStr;
    AutoTaggerLoader taggerLoader;
    AutoTagger* tagger = nullptr;

    std::thread analyzeThread;
    std::atomic<bool> readyFlag = false;
    std::condition_variable readyCv;

    std::vector<std::pair<uint64_t, std::vector<std::filesystem::path>>> picFilesForTagging; // (picID, filePath)
    std::atomic<size_t> nextIndex = 0;
    std::atomic<size_t> totalSupported = 0;
    std::mutex picFileMutex;

    std::vector<std::thread> preprocessWorkers;
    std::queue<std::pair<uint64_t, std::vector<float>>> preprocessedPic; // (picID, imageData)
    std::mutex preprocessedMutex;
    std::condition_variable preprocessedCv;

    std::atomic<bool> stopFlag = false;
    std::atomic<bool> finished = false;

    void preprocessWorkerFunc();
    void analyzeThreadFunc();

    LogCallback logCallback = [](const std::string& message) {
        Info() << "[AutoTagger] " << message;
    };
};
