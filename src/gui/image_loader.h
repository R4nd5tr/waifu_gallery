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
#include "service/model.h"
#include <QEvent>
#include <QImage>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class MainWindow;

struct ImageLoadTask {
    uint64_t id;
    std::vector<std::filesystem::path> filePaths;
};

class ImageLoadCompleteEvent : public QEvent {
public:
    static const QEvent::Type EventType;
    ImageLoadCompleteEvent(uint64_t id, QImage&& img) : QEvent(EventType), id(id), img(std::move(img)) {}
    uint64_t id;
    QImage img;
};

class ImageLoader { // TODO: load full image functionality?
public:
    ImageLoader(MainWindow* mainWindow, size_t numThreads = std::thread::hardware_concurrency());
    ~ImageLoader();

    void loadImage(const PicInfo& picInfo);
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
};
