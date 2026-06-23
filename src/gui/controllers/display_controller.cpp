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

#include "display_controller.h"
#include "ui_main_window.h"
#include <QScrollBar>

DisplayController::DisplayController(Ui::MainWindow* ui, ImageLoader* imageLoader)
    : ui(ui), picFramePool(ui->picBrowseWidget, imageLoader) {
    this->totalWidth = ui->picBrowseScrollArea->width();
    this->totalHeight = ui->picBrowseWidget->height();
    this->viewportHeight = ui->picBrowseScrollArea->height();
    this->picsPerRow = ((totalWidth - 2 * MARGIN - PIC_FRAME_WIDTH) / (PIC_FRAME_WIDTH + SPACING)) + 1;
}

DisplayController::~DisplayController() {}

void DisplayController::setDisplayItems(std::unique_ptr<DisplayItems>& displayItems, SearchField searchField) {
    this->displayItems = std::move(displayItems);
    this->displayMode = this->displayItems->type;
    this->sortedItemIndices.resize(this->displayItems->metadataItems.size()
                                       ? this->displayItems->type == DisplayItemType::Metadata
                                       : this->displayItems->picItems.size());
    std::iota(this->sortedItemIndices.begin(), this->sortedItemIndices.end(), 0);
    this->searchField = searchField;

    // reset state
    this->displayingItemIndices.clear();
    this->scrollBarValue = 0;
    this->picIdToFrameIdxMap.clear();
    for (int i = startDisplayIndex; i < endDisplayIndex; i++) {
        picFramePool.release(picFrames[i]);
    }
    this->picFrames.clear();
    this->startDisplayIndex = 0;
    this->endDisplayIndex = 0;
    this->ui->picBrowseScrollArea->verticalScrollBar()->setValue(0);

    // update totalHeight
    this->totalHeight = MARGIN * 2 + (PIC_FRAME_HEIGHT + SPACING) * (this->displayingItemIndices.size() / this->picsPerRow + 1);
    this->ui->picBrowseWidget->setMinimumHeight(this->totalHeight);
}

Vec2 DisplayController::getPicFramePosition(size_t displayIndex) const {
    int availableWidth = totalWidth - 2 * MARGIN - PIC_FRAME_WIDTH;
    size_t row = displayIndex / picsPerRow;
    size_t col = displayIndex % picsPerRow;
    int x = MARGIN + col * (PIC_FRAME_WIDTH + availableWidth / picsPerRow);
    int y = MARGIN + row * (PIC_FRAME_HEIGHT + SPACING);
    return Vec2{x, y};
}

void DisplayController::displayPicFrames() {
    int viewportTopRow = std::max(0, (scrollBarValue - MARGIN)) / (PIC_FRAME_HEIGHT + SPACING);
    int viewportBottomRow = (std::max(0, (scrollBarValue + viewportHeight - MARGIN)) / (PIC_FRAME_HEIGHT + SPACING)) + 1;

    size_t newStartDisplayIndex = std::max(0, viewportTopRow - PRE_LOAD_ROWS) * picsPerRow;
    size_t newEndDisplayIndex = (viewportBottomRow + PRE_LOAD_ROWS) * picsPerRow;

    if (newStartDisplayIndex == startDisplayIndex && newEndDisplayIndex == endDisplayIndex) return; // no change

    assert(newStartDisplayIndex <= newEndDisplayIndex);
    assert(newStartDisplayIndex < endDisplayIndex || newEndDisplayIndex > startDisplayIndex);

    if (newStartDisplayIndex < startDisplayIndex) { // need to load new frames at the top
        for (size_t i = startDisplayIndex - 1; i >= newStartDisplayIndex; i--) {
            size_t displayItemIdx = displayingItemIndices[i];
            PicItem* picItem = nullptr;
            MetadataItem* metadataItem = nullptr;
            switch (displayMode) {
            case DisplayItemType::Pic:
                picItem = &displayItems->picItems[displayItemIdx];
                if (picItem->metadataCount > 0) {
                    metadataItem = &displayItems->metadataItems[picItem->metadataStartIndex];
                }
                break;
            case DisplayItemType::Metadata:
                metadataItem = &displayItems->metadataItems[displayItemIdx];
                picItem = &displayItems->picItems[metadataItem->picStartIndex];
                break;
            }
            PictureFrame* picFrame = picFramePool.acquire(picItem, metadataItem, searchField);
            picFrames[i] = picFrame;
            Vec2 pos = getPicFramePosition(i);
            picFrame->move(pos.x, pos.y);
        }
        startDisplayIndex = newStartDisplayIndex;
    } else if (newStartDisplayIndex > startDisplayIndex) { // need to release frames at the top
        for (size_t i = startDisplayIndex; i < newStartDisplayIndex; i++) {
            picFramePool.release(picFrames[i]);
            picFrames[i] = nullptr;
        }
        startDisplayIndex = newStartDisplayIndex;
    }

    if (newEndDisplayIndex > endDisplayIndex) { // need to load new frames at the bottom
        for (size_t i = endDisplayIndex + 1; i <= newEndDisplayIndex; i++) {
            if (i >= displayingItemIndices.size()) { // need to find next filtered item
                filteredIndex += 1;
                while (filteredIndex < sortedItemIndices.size()) {
                    if (displayMode == DisplayItemType::Pic) {
                        if (isMatchFilter(displayItems->picItems[sortedItemIndices[filteredIndex]].info, filterCtx)) break;
                    } else if (displayMode == DisplayItemType::Metadata) {
                        if (isMatchFilter(displayItems->metadataItems[sortedItemIndices[filteredIndex]].metadata, filterCtx))
                            break;
                    }
                    filteredIndex += 1;
                }

                if (filteredIndex >= sortedItemIndices.size()) break;
                displayingItemIndices.push_back(sortedItemIndices[filteredIndex]);
            }

            size_t displayItemIdx = displayingItemIndices[i];
            PicItem* picItem = nullptr;
            MetadataItem* metadataItem = nullptr;
            switch (displayMode) {
            case DisplayItemType::Pic:
                picItem = &displayItems->picItems[displayItemIdx];
                if (picItem->metadataCount > 0) {
                    metadataItem = &displayItems->metadataItems[picItem->metadataStartIndex];
                }
                picIdToFrameIdxMap[picItem->info.id] = i;
                break;
            case DisplayItemType::Metadata:
                metadataItem = &displayItems->metadataItems[displayItemIdx];
                if (metadataItem->picCount > 0) {
                    picItem = &displayItems->picItems[metadataItem->picStartIndex];
                    for (int j = metadataItem->picStartIndex; j < metadataItem->picStartIndex + metadataItem->picCount; j++) {
                        picIdToFrameIdxMap[displayItems->picItems[j].info.id] = i;
                    }
                }
                break;
            }
            PictureFrame* picFrame = picFramePool.acquire(picItem, metadataItem, searchField);
            if (i >= picFrames.size()) {
                picFrames.push_back(picFrame);
            } else {
                picFrames[i] = picFrame;
            }
            Vec2 pos = getPicFramePosition(i);
            picFrame->move(pos.x, pos.y);
        }
        endDisplayIndex = newEndDisplayIndex;
    } else if (newEndDisplayIndex < endDisplayIndex) { // need to release frames at the bottom
        for (size_t i = endDisplayIndex; i > newEndDisplayIndex; i--) {
            picFramePool.release(picFrames[i]);
            picFrames[i] = nullptr;
        }
        endDisplayIndex = newEndDisplayIndex;
    }
}
