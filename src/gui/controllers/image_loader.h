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
#include "image_cache.h"
#include "service/model.h"
#include <QEvent>
#include <QImage>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

class MainWindow;

enum class LoadType { Thumbnail, Preview };

const size_t THUMBNAIL_CACHE_CAPACITY = 200;
const size_t PREVIEW_CACHE_CAPACITY = 50;

struct ImageLoadTask {
    LoadType loadType;
    uint64_t id;
    std::vector<std::filesystem::path> filePaths;
};

struct ImageLoadResult {
    LoadType loadType;
    uint64_t id;
};

class ImageLoadCompleteEvent : public QEvent {
public:
    static const QEvent::Type EventType;
    ImageLoadCompleteEvent(ImageLoadResult result) : QEvent(EventType), result(result) {}
    ImageLoadResult result;
};

class ImageLoader {
public:
    ImageLoader(MainWindow* mainWindow,
                size_t numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) / 2));
    ~ImageLoader();

    QImage* getImage(const PicInfo& picInfo, LoadType loadType);
    QImage* getImage(uint64_t picId, LoadType loadType);
    void clearTasks();

private:
    void stop();
    void workerFunction();
    MainWindow* mainWindow;
    std::vector<std::thread> workers;
    std::queue<ImageLoadTask> taskQueue;
    std::mutex mutex;
    std::condition_variable condVar;
    std::atomic<bool> stopFlag{false};

    std::unordered_set<uint64_t> loadingThumbnailIds;
    std::unordered_set<uint64_t> loadingPreviewIds;

    ImageCache thumbnailCache{THUMBNAIL_CACHE_CAPACITY};
    ImageCache previewCache{PREVIEW_CACHE_CAPACITY};
};
