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
#include "model.h"
#include "parser.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

class Importer { // TODO: make this accept multiple directories?
public:
    Importer(const std::filesystem::path& directory,
             ImportProgressCallback progressCallback = nullptr,
             ParserType prserType = ParserType::None,
             std::string dbFile = DEFAULT_DATABASE_FILE,
             size_t threadCount = std::thread::hardware_concurrency());
    ~Importer();

    bool finish();
    void forceStop();
    std::pair<std::filesystem::path, ParserType> getImportingDir() const { return {importDirectory, parserType}; }

private:
    bool finished = false;

    ImportProgressCallback progressCallback; // this will be called in insert thread, make sure it's thread safe
    ParserType parserType;
    std::string dbFile;
    std::filesystem::path importDirectory;

    std::vector<std::thread> workers;
    std::vector<std::filesystem::path> files;
    std::atomic<bool> readyFlag = false;
    std::condition_variable cvReady;
    std::atomic<size_t> nextFileIndex = 0;
    size_t importedCount = 0;
    std::atomic<size_t> supportedFileCount = 0; // to prevent importer from never stopping
                                                // when some unsupported files are in the directory
    // single insert thread
    std::thread insertThread;
    std::queue<ParsedPicture> parsedPictureQueue; // one ParsedPicture represents one image file
    std::mutex parsedPicQueueMutex;
    std::queue<std::vector<ParsedMetadata>> metadataVecQueue; // one vector represents one metadata file
    std::mutex parsedMetadataQueueMutex;

    std::atomic<bool> stopFlag = false;
    std::condition_variable cv;

    void workerThreadFunc();
    void insertThreadFunc();
};
