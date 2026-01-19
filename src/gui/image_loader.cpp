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

#include "image_loader.h"
#include "main_window.h"
#include "utils/logger.h"
#include <QCoreApplication>
#include <QImageReader>

const size_t IMAGE_RESOLUTION_LIMIT = 256;

const QEvent::Type ImageLoadCompleteEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());

ImageLoader::ImageLoader(MainWindow* mainWindow, size_t numThreads) : mainWindow(mainWindow) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ImageLoader::workerFunction, this);
    }
}
ImageLoader::~ImageLoader() {
    stop();
}
void ImageLoader::loadImage(const PicInfo& picInfo) {
    if (picInfo.filePaths.empty()) {
        Warn() << "No file paths available for PicInfo ID:" << picInfo.id;
        return;
    }
    ImageLoadTask task{picInfo.id, picInfo.filePaths};
    std::lock_guard<std::mutex> lock(mutex);
    taskQueue.push(task);
    condVar.notify_one();
}
void ImageLoader::clearTasks() {
    std::lock_guard<std::mutex> lock(mutex);
    while (!taskQueue.empty()) {
        taskQueue.pop();
    }
}
void ImageLoader::stop() {
    stopFlag.store(true, std::memory_order_release);
    clearTasks();
    condVar.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
}
void ImageLoader::workerFunction() {
    while (true) {
        ImageLoadTask task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (stopFlag.load(std::memory_order_acquire)) return;
            condVar.wait(lock, [this]() { return !taskQueue.empty() || stopFlag.load(std::memory_order_acquire); });
            if (stopFlag.load(std::memory_order_acquire) && taskQueue.empty()) return;
            task = taskQueue.front();
            taskQueue.pop();
        }

        QString filePathStr;
        for (const auto& filePath : task.filePaths) {
            if (!std::filesystem::exists(filePath)) {
                Warn() << "File does not exist:" << filePath;
                continue;
            }
            filePathStr = QString::fromUtf8(filePath.u8string().c_str());
            break;
        }
        QImageReader reader(filePathStr);
        if (!reader.canRead()) {
            Warn() << "Cannot read image format:" << filePathStr;
            continue;
        }
        reader.setAutoTransform(true);

        QImage img;
        QSize originalSize = reader.size();
        if (!originalSize.isValid()) originalSize = QSize(1, 1);
        if (originalSize.width() > IMAGE_RESOLUTION_LIMIT || originalSize.height() > IMAGE_RESOLUTION_LIMIT) {
            reader.setScaledSize(originalSize.scaled(IMAGE_RESOLUTION_LIMIT, IMAGE_RESOLUTION_LIMIT, Qt::KeepAspectRatio));
        }
        if (!reader.read(&img)) {
            Warn() << "Failed to read image:" << filePathStr << ", Error:" << reader.errorString();
            continue;
        }
        QCoreApplication::postEvent(mainWindow, new ImageLoadCompleteEvent(task.id, std::move(img)));
    }
}