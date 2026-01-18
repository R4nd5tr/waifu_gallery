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

#include "importer.h"

void Importer::startImportFromDirectory(const std::filesystem::path& directory, ParserType parserType) {
    if (!finished || !finish()) {
        Warn() << "Importer is running.";
        return;
    }
    finished = false;
    importDirectory = directory;
    this->parserType = parserType;

    // start insert thread
    insertThread = std::thread(&Importer::insertThreadFunc, this);

    // start worker threads
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&Importer::workerThreadFunc, this);
    }
}
void Importer::forceStop() {
    if (finished) return;
    stopFlag.store(true);
    finish();
}
bool Importer::finish() {
    if (finished) return true; // already finished
    if (importedCount < supportedFileCount.load(std::memory_order_relaxed) && !stopFlag.load()) {
        return false; // not finished yet
    }

    // import finished, join all threads and clean up, reset state
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
    files.clear();
    nextFileIndex.store(0, std::memory_order_relaxed);

    cv.notify_all();
    if (insertThread.joinable()) {
        insertThread.join();
    }
    readyFlag.store(false, std::memory_order_relaxed);
    importedCount = 0;
    supportedFileCount.store(0, std::memory_order_relaxed);

    parsedPicQueueMutex.lock();
    while (!parsedPictureQueue.empty()) {
        parsedPictureQueue.pop();
    }
    parsedPicQueueMutex.unlock();
    parsedMetadataQueueMutex.lock();
    while (!metadataVecQueue.empty()) {
        metadataVecQueue.pop();
    }
    parsedMetadataQueueMutex.unlock();

    stopFlag.store(false, std::memory_order_relaxed);

    finished = true;
    return true;
}
bool processSingleFile(const std::filesystem::path& filePath,
                       ParserType parserType,
                       std::vector<ParsedPicture>& parsedPictures,
                       std::vector<std::vector<ParsedMetadata>>& parsedMetadataVecs) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            parsedPictures.emplace_back(parsePicture(filePath, parserType));
            return true;
        } else {
            switch (parserType) {
            case ParserType::PowerfulPixivDownloader: {
                std::vector<ParsedMetadata> parsedMetadata = powerfulPixivDownloaderMetadataParser(filePath);
                if (!parsedMetadata.empty()) {
                    parsedMetadataVecs.push_back(std::move(parsedMetadata));
                    return true;
                }
                break;
            }
            case ParserType::GallerydlTwitter: {
                ParsedMetadata parsedMetadata = gallerydlTwitterMetadataParser(filePath);
                if (parsedMetadata.platformType != PlatformType::Unknown) {
                    parsedMetadataVecs.push_back({std::move(parsedMetadata)});
                    return true;
                }
                break;
            }
            default:
                break;
            }
        }
    } catch (const std::exception& e) {
        Error() << "Error processing file:" << filePath << "Error:" << e.what();
        return false;
    }
    return false;
}
void Importer::workerThreadFunc() {
    std::vector<ParsedPicture> parsedPictures;
    parsedPictures.reserve(500);
    std::vector<std::vector<ParsedMetadata>> ParsedMetadataVecs;
    ParsedMetadataVecs.reserve(500);
    size_t index = 0;

    // Wait until files are collected
    while (!readyFlag.load(std::memory_order_acquire) && !stopFlag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (stopFlag.load()) return;

    while ((index = nextFileIndex.fetch_add(1, std::memory_order_relaxed)) < files.size()) {
        if (parsedPictures.size() >= 100) {
            if (parsedPicQueueMutex.try_lock()) {
                while (!parsedPictures.empty()) {
                    parsedPictureQueue.push(std::move(parsedPictures.back()));
                    parsedPictures.pop_back();
                }
                parsedPicQueueMutex.unlock();
                parsedPictures.clear();
                cv.notify_one();
            }
        }
        if (ParsedMetadataVecs.size() >= 100) {
            if (parsedMetadataQueueMutex.try_lock()) {
                while (!ParsedMetadataVecs.empty()) {
                    metadataVecQueue.push(std::move(ParsedMetadataVecs.back()));
                    ParsedMetadataVecs.pop_back();
                }
                parsedMetadataQueueMutex.unlock();
                ParsedMetadataVecs.clear();
                cv.notify_one();
            }
        }
        if (stopFlag.load()) return;
        const auto& filePath = files[index];
        if (!processSingleFile(filePath, parserType, parsedPictures, ParsedMetadataVecs))
            supportedFileCount.fetch_sub(1, std::memory_order_relaxed);
    }
    if (!parsedPictures.empty()) {
        std::lock_guard<std::mutex> lock(parsedPicQueueMutex);
        while (!parsedPictures.empty()) {
            parsedPictureQueue.push(std::move(parsedPictures.back()));
            parsedPictures.pop_back();
        }
        parsedPictures.clear();
        cv.notify_one();
    }
    if (!ParsedMetadataVecs.empty()) {
        std::lock_guard<std::mutex> lock(parsedPicQueueMutex);
        while (!ParsedMetadataVecs.empty()) {
            metadataVecQueue.push(std::move(ParsedMetadataVecs.back()));
            ParsedMetadataVecs.pop_back();
        }
        ParsedMetadataVecs.clear();
        cv.notify_one();
    }
}
void Importer::insertThreadFunc() {
    PicDatabase threadDb(dbFile, DbMode::Import);
    std::mutex conditionMutex;
    std::vector<ParsedPicture> picsToInsert;
    picsToInsert.reserve(1000);
    std::vector<std::vector<ParsedMetadata>> metadataVecsToInsert;
    metadataVecsToInsert.reserve(1000);

    // Collect files to import
    for (const auto& entry : std::filesystem::recursive_directory_iterator(importDirectory)) {
        if (!entry.is_regular_file() || threadDb.isFileImported(entry.path())) {
            continue;
        }
        files.push_back(entry.path());
    }
    supportedFileCount = files.size();
    Info() << "Total files to import: " << supportedFileCount.load();
    readyFlag.store(true, std::memory_order_release);

    threadDb.beginTransaction();
    while (!stopFlag.load() && importedCount < supportedFileCount.load(std::memory_order_relaxed)) {
        if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_relaxed));
        std::unique_lock<std::mutex> lock(conditionMutex);
        cv.wait(lock, [this]() { return !parsedPictureQueue.empty() || !metadataVecQueue.empty() || stopFlag.load(); });
        if (stopFlag.load()) break;
        if (!parsedPictureQueue.empty()) {
            if (parsedPicQueueMutex.try_lock()) {
                while (!parsedPictureQueue.empty()) {
                    picsToInsert.push_back(std::move(parsedPictureQueue.front()));
                    parsedPictureQueue.pop();
                }
                parsedPicQueueMutex.unlock();
                for (const auto& picInfo : picsToInsert) {
                    threadDb.insertPicture(picInfo);
                    importedCount++;
                }
                picsToInsert.clear();
            }
        }
        if (!metadataVecQueue.empty()) {
            if (parsedMetadataQueueMutex.try_lock()) {
                while (!metadataVecQueue.empty()) {
                    metadataVecsToInsert.push_back(std::move(metadataVecQueue.front()));
                    metadataVecQueue.pop();
                }
                parsedMetadataQueueMutex.unlock();
                for (const auto& metadataVec : metadataVecsToInsert) {
                    for (const auto& metadataInfo : metadataVec) {
                        if (metadataInfo.updateIfExists) {
                            threadDb.updateMetadata(metadataInfo);
                        } else {
                            threadDb.insertMetadata(metadataInfo);
                        }
                    }
                    importedCount++;
                }
                metadataVecsToInsert.clear();
            }
        }
    }
    Info() << "Finalizing import";
    if (stopFlag.load()) {
        Info() << "Import stopped by user, rolling back. " << "Directory: " << importDirectory;
        threadDb.rollbackTransaction();
        return;
    }
    threadDb.syncMetadataAndPicTables();
    threadDb.updatePlatformTagCounts();
    for (const auto& filePath : files) {
        threadDb.addImportedFile(filePath);
    }
    if (!threadDb.commitTransaction()) {
        Error() << "Import commit failed, rolling back. " << "Directory: " << importDirectory;
        threadDb.rollbackTransaction();
    }
    Info() << "Import completed. Directory: " << importDirectory;
    // progress equals to total is the signal of completion
    if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_seq_cst));
}
