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

#include "tagger.h"
#include "utils/logger.h"

PicTagger::PicTagger(const std::string& databaseFile) {
    databaseFileStr = databaseFile;
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
bool PicTagger::loadTaggerDLL(const std::filesystem::path& dllPath) {
    if (!taggerLoader.load(dllPath)) {
        Error() << "Failed to load AutoTagger DLL from path:" << dllPath.string();
        return false;
    }
    tagger = taggerLoader.getTagger();
    if (!tagger) {
        Error() << "Failed to create AutoTagger instance from DLL:" << dllPath.string();
        return false;
    }
    Info() << "Successfully loaded AutoTagger from DLL:" << dllPath.string();
    tagger->setLogCallback(logCallback);
    return true;
}
void PicTagger::loadTagSetToDatabase() {
    if (!tagger) {
        Error() << "No tagger loaded.";
        return;
    }

    PicDatabase db(databaseFileStr, DbMode::Normal);
    auto tagSet = tagger->getTagSet();
    auto name = tagger->getModelName();
    Info() << "Loading tag set for model:" << name;
    db.importTagSet(name, tagSet);
    Info() << "Loaded" << tagSet.size() << "tags to database.";
}
void PicTagger::startTagging() {
    if (!tagger) {
        Error() << "No tagger loaded, cannot start tagging.";
        return;
    }

    // start analyze thread
    analyzeThread = std::thread(&PicTagger::analyzeThreadFunc, this);

    // start preprocess workers
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        preprocessWorkers.emplace_back(&PicTagger::preprocessWorkerFunc, this);
    }
}
void PicTagger::forceStop() {
    stopFlag.store(true);
    readyCv.notify_all();
    preprocessedCv.notify_all();
}
void PicTagger::preprocessWorkerFunc() {
    size_t index;

    {
        std::unique_lock<std::mutex> lock(picFileMutex);
        readyCv.wait(lock, [this]() { return readyFlag.load(std::memory_order_acquire) || stopFlag.load(); });
    }

    while (index = nextIndex.fetch_add(1), index < picFilesForTagging.size() && !stopFlag.load()) {
        const auto& [picID, filePaths] = picFilesForTagging[index];
        bool preprocessed = false;
        for (const auto& filePath : filePaths) {
            if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) continue;

            std::vector<float> preprocessedData = tagger->preprocess(filePath);
            if (preprocessedData.empty()) {
                Error() << "Preprocessing failed for file:" << filePath.string();
                continue;
            }
            {
                std::unique_lock<std::mutex> lock(preprocessedMutex);
                preprocessedCv.wait(lock,
                                    [this]() { return preprocessedPic.size() < MAX_PREPROCESS_QUEUE_SIZE || stopFlag.load(); });
                preprocessedPic.emplace(picID, std::move(preprocessedData));
            }
            preprocessedCv.notify_one();
            preprocessed = true;
            break; // only need to process one valid file per picID
        }
        if (!preprocessed) {
            Warn() << "Failed to preprocess any file for picID:" << picID;
            totalSupported--;
        }
    }
}
void PicTagger::analyzeThreadFunc() {
    PicDatabase threadDb(databaseFileStr, DbMode::Import);
    picFilesForTagging = threadDb.getUntaggedPics();
    totalSupported = picFilesForTagging.size();
    Info() << "Total pictures to tag:" << totalSupported.load();
    readyFlag.store(true, std::memory_order_release);
    readyCv.notify_all();
    size_t analyzed = 0;

    threadDb.beginTransaction();
    while (analyzed < totalSupported.load(std::memory_order_relaxed) && !stopFlag.load()) {
        // fetch preprocessed data
        std::pair<uint64_t, std::vector<float>> item;
        {
            std::unique_lock<std::mutex> lock(preprocessedMutex);
            preprocessedCv.wait(lock, [this]() { return !preprocessedPic.empty() || stopFlag.load(); });
            if (stopFlag.load() && preprocessedPic.empty()) break;
            item = std::move(preprocessedPic.front());
            preprocessedPic.pop();
        }
        preprocessedCv.notify_one();

        // analyze
        uint64_t picID = item.first;
        std::vector<float>& imageData = item.second;

        PredictResult predictResult = tagger->predict(imageData);
        ImageTagResult tagResult = tagger->postprocess(predictResult);

        // update database
        std::vector<PicTag> picTags;
        for (size_t i = 0; i < tagResult.tagIndexes.size(); ++i) {
            picTags.emplace_back(PicTag{static_cast<uint32_t>(tagResult.tagIndexes[i]), tagResult.tagProbabilities[i]});
        }
        RestrictType restrictType = static_cast<RestrictType>(static_cast<int>(tagResult.restrictType));
        threadDb.updatePicTags(picID, picTags, restrictType, predictResult.featureHash);
        analyzed++;
        if (analyzed % 1000 == 0) {
            Info() << "Tagged pictures:" << analyzed << "/" << totalSupported.load();
        }
    }

    Info() << "Finalizing tagging, total analyzed:" << analyzed;
    if (stopFlag.load()) {
        Info() << "Tagging process was stopped by user.";
        threadDb.rollbackTransaction();
        finished.store(true);
        return;
    }

    threadDb.updateTagCounts();

    if (!threadDb.commitTransaction()) {
        Error() << "Failed to commit tagging transaction, rolling back.";
        threadDb.rollbackTransaction();
    }

    finished.store(true);
    Info() << "Tagging process completed.";
}
