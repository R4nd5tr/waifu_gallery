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
#include <QImage>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

struct ImageCacheNode {
    std::unique_ptr<QImage> img;
    size_t prev;
    size_t next;
};

class ImageCache {
public:
    ImageCache(size_t capacity) : capacity(capacity) {
        indexToIdMap = std::make_unique<uint64_t[]>(capacity);
        nodeArray = std::make_unique<ImageCacheNode[]>(capacity);
    }
    ~ImageCache() { clear(); }

    void put(uint64_t id, std::unique_ptr<QImage> img);
    QImage* get(uint64_t id);

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        for (size_t i = 0; i < size; ++i) {
            nodeArray[i].img.reset();
        }
        size = 0;
        idToIndexMap.clear();
        headIndex = tailIndex = 0;
    };

private:
    size_t capacity;
    size_t size = 0;
    std::unordered_map<uint64_t, size_t> idToIndexMap;
    std::unique_ptr<uint64_t[]> indexToIdMap;
    std::unique_ptr<ImageCacheNode[]> nodeArray;
    size_t headIndex = 0;
    size_t tailIndex = 0;

    std::mutex mutex;
};
