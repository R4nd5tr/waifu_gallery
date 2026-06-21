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

#include "image_cache.h"

void ImageCache::put(uint64_t id, std::unique_ptr<QImage> img) {
    if (capacity == 0 || img == nullptr) return;
    std::lock_guard<std::mutex> lock(mutex);
    auto it = idToIndexMap.find(id);
    if (it != idToIndexMap.end()) { // already in cache, do nothing
        return;
    } else { // not in cache, add new node
        if (size < capacity) {
            size_t index = size;
            size += 1;
            idToIndexMap[id] = index;
            indexToIdMap[index] = id;
            nodeArray[index] = {std::move(img), index, index}; // initialize prev and next to itself
            if (size == 1) {
                headIndex = tailIndex = index;
            } else { // insert at head
                nodeArray[index].next = headIndex;
                nodeArray[index].prev = tailIndex;
                nodeArray[headIndex].prev = index;
                nodeArray[tailIndex].next = index;
                headIndex = index;
            }
        } else { // cache is full, evict tail and add new node at head
            // replace image
            nodeArray[tailIndex].img = std::move(img);
            // update mappings
            idToIndexMap.erase(indexToIdMap[tailIndex]);
            idToIndexMap[id] = tailIndex;
            indexToIdMap[tailIndex] = id;
            // move to head
            headIndex = tailIndex;
            tailIndex = nodeArray[tailIndex].prev;
        }
    }
}

QImage* ImageCache::get(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = idToIndexMap.find(id);
    if (it == idToIndexMap.end()) return nullptr; // not found

    size_t index = it->second;
    if (index == tailIndex) {
        headIndex = tailIndex;
        tailIndex = nodeArray[index].prev;
    } else if (index != headIndex) { // move to head
        // remove from current position
        nodeArray[nodeArray[index].prev].next = nodeArray[index].next;
        nodeArray[nodeArray[index].next].prev = nodeArray[index].prev;
        // insert at head
        nodeArray[index].next = headIndex;
        nodeArray[index].prev = tailIndex;
        nodeArray[headIndex].prev = index;
        nodeArray[tailIndex].next = index;
        headIndex = index;
    }
    return nodeArray[index].img.get();
}
