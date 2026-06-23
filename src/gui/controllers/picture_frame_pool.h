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
#include "../widgets/picture_frame.h"
#include "service/model.h"
#include <queue>
#include <unordered_set>

class PicFramePool {
public:
    PicFramePool(QWidget* parentWidget, ImageLoader* imageLoader) : parentWidget(parentWidget), imageLoader(imageLoader) {};
    ~PicFramePool() {
        while (!availableFrames.empty()) {
            delete availableFrames.front();
            availableFrames.pop();
        }
        for (auto* frame : occupiedFrames) {
            delete frame;
        }
    };
    PictureFrame* acquire(const PicItem* picItem, const MetadataItem* metadataItem, SearchField searchField) {
        PictureFrame* frame;
        if (!availableFrames.empty()) {
            frame = availableFrames.front();
            availableFrames.pop();
            frame->updateDisplayItem(picItem, metadataItem, searchField);
        } else {
            frame = new PictureFrame(parentWidget, picItem, metadataItem, *imageLoader, searchField);
        }
        occupiedFrames.insert(frame);
        frame->show();
        return frame;
    };
    void release(PictureFrame* frame) {
        if (occupiedFrames.find(frame) == occupiedFrames.end()) return;

        frame->reset();
        frame->hide();
        occupiedFrames.erase(frame);
        availableFrames.push(frame);
    };

private:
    std::unordered_set<PictureFrame*> occupiedFrames;
    std::queue<PictureFrame*> availableFrames;

    QWidget* parentWidget = nullptr;
    ImageLoader* imageLoader = nullptr;
};
