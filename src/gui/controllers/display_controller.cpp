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

DisplayController::DisplayController(Ui::MainWindow* ui, ImageLoader* imageLoader) : ui(ui), imageLoader(imageLoader) {}

DisplayController::~DisplayController() {
    if (displayItems) {
        delete displayItems;
        displayItems = nullptr;
    }
}

void DisplayController::setup() {
    picFramePool = std::make_unique<PicFramePool>(ui->picBrowseWidget, imageLoader);
}

void DisplayController::refreshDisplay() {
    this->displayingItemIndices.clear();
    this->scrollBarValue = 0;
    this->picIdToFrameIdxMap.clear();
    this->nextMatchSortedIndex = 0;
    if (displaying) {
        for (int i = startDisplayIndex; i < endDisplayIndex; i++) {
            picFramePool->release(picFrames[i]);
        }
    }
    std::fill(picFrames.begin(), picFrames.end(), nullptr);
    this->startDisplayIndex = 0;
    this->endDisplayIndex = 0;
    this->ui->picBrowseScrollArea->verticalScrollBar()->setValue(0);
    // update totalHeight to max possible height, will be updated when displaying items
    this->totalHeight = MARGIN * 2 + (PIC_FRAME_HEIGHT + SPACING) * (this->sortedItemIndices.size() / this->picsPerRow + 1);
    this->ui->picBrowseWidget->setMinimumHeight(this->totalHeight);
    displaying = false;
}

void DisplayController::setDisplayItems(DisplayItems* displayItems, SearchField searchField) {
    delete this->displayItems; // delete previous displayItems
    this->displayItems = displayItems;
    this->displayMode = this->displayItems->type;
    this->sortedItemIndices.resize(this->displayItems->type == DisplayItemType::Metadata
                                       ? this->displayItems->metadataItems.size()
                                       : this->displayItems->picItems.size());
    std::iota(this->sortedItemIndices.begin(), this->sortedItemIndices.end(), 0);
    this->picFrames.resize(this->sortedItemIndices.size(), nullptr);
    this->searchField = searchField;

    refreshDisplay();
    displayPicFrames();
}

void DisplayController::displayImage(uint64_t picId, LoadType loadType) {
    auto it = picIdToFrameIdxMap.find(picId);
    if (it != picIdToFrameIdxMap.end()) {
        int idx = it->second;
        if (idx < startDisplayIndex || idx >= endDisplayIndex) return; // not currently displayed
        PictureFrame* picFrame = picFrames[idx];
        if (picFrame) {
            picFrame->displayImage(picId, loadType);
        }
    }
}

Vec2 DisplayController::getPicFramePosition(int displayIndex) const {
    int availableWidth = totalWidth - 2 * MARGIN - (PIC_FRAME_WIDTH * picsPerRow);
    int row = displayIndex / picsPerRow;
    int col = displayIndex % picsPerRow;
    int x = MARGIN + col * (PIC_FRAME_WIDTH + (availableWidth / picsPerRow)) + std::min(col, availableWidth % picsPerRow);
    int y = MARGIN + row * (PIC_FRAME_HEIGHT + SPACING);
    return Vec2{x, y};
}

void DisplayController::displayPicFrames() {
    if (!displayItems) return; // no display items to display

    int viewportTopRow = std::max(0, (scrollBarValue - MARGIN)) / (PIC_FRAME_HEIGHT + SPACING);
    int viewportBottomRow = (std::max(0, (scrollBarValue + viewportHeight - MARGIN)) / (PIC_FRAME_HEIGHT + SPACING)) + 1;

    int newStartDisplayIndex = std::max(0, viewportTopRow - PRE_LOAD_ROWS) * picsPerRow;
    int newEndDisplayIndex = (viewportBottomRow + PRE_LOAD_ROWS) * picsPerRow;

    if (displaying && newStartDisplayIndex == startDisplayIndex && newEndDisplayIndex == endDisplayIndex) return; // no change

    if (newStartDisplayIndex < startDisplayIndex) { // load new frames at the top
        for (int i = std::min(startDisplayIndex - 1, newEndDisplayIndex - 1); i >= newStartDisplayIndex; i--) {
            int displayItemIdx = displayingItemIndices[i];
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
            PictureFrame* picFrame = picFramePool->acquire(picItem, metadataItem, searchField);
            picFrames[i] = picFrame;
            Vec2 pos = getPicFramePosition(i);
            picFrame->move(pos.x, pos.y);
        }
    } else if (newStartDisplayIndex > startDisplayIndex) { // release frames at the top
        for (int i = startDisplayIndex; i < std::min(newStartDisplayIndex, endDisplayIndex); i++) {
            picFramePool->release(picFrames[i]);
            picFrames[i] = nullptr;
        }
    }
    startDisplayIndex = newStartDisplayIndex;

    if (newEndDisplayIndex > endDisplayIndex) {
        for (int i = std::max(newStartDisplayIndex, endDisplayIndex); i < newEndDisplayIndex;
             i++) { // load new frames at the bottom
            if (i >= displayingItemIndices.size()) {
                while (i >= displayingItemIndices.size()) { // need to find next item that matches filter
                    while (nextMatchSortedIndex < sortedItemIndices.size()) {
                        if (displayMode == DisplayItemType::Pic &&
                            isMatchFilter(displayItems->picItems[sortedItemIndices[nextMatchSortedIndex]].info, filterCtx)) {
                            nextMatchSortedIndex += 1;
                            break;
                        } else if (displayMode == DisplayItemType::Metadata &&
                                   isMatchFilter(displayItems->metadataItems[sortedItemIndices[nextMatchSortedIndex]].metadata,
                                                 filterCtx)) {
                            nextMatchSortedIndex += 1;
                            break;
                        }
                        nextMatchSortedIndex += 1;
                    }
                    if (nextMatchSortedIndex >= sortedItemIndices.size()) break;

                    // nextMatchSortedIndex has been incremented,
                    // it is the index of the next item to check, so we need to use nextMatchSortedIndex - 1
                    displayingItemIndices.push_back(sortedItemIndices[nextMatchSortedIndex - 1]);
                }
                if (nextMatchSortedIndex >= sortedItemIndices.size()) { // no more items to display
                    // update totalHeight to reflect the actual number of items that match the filter
                    this->totalHeight =
                        MARGIN * 2 + (PIC_FRAME_HEIGHT + SPACING) * (displayingItemIndices.size() / this->picsPerRow + 1);
                    this->ui->picBrowseWidget->setMinimumHeight(this->totalHeight);

                    newEndDisplayIndex = displayingItemIndices.size();
                    break;
                }
            }

            int displayItemIdx = displayingItemIndices[i];
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
            PictureFrame* picFrame = picFramePool->acquire(picItem, metadataItem, searchField);
            if (i >= picFrames.size()) {
                picFrames.push_back(picFrame);
            } else {
                picFrames[i] = picFrame;
            }
            Vec2 pos = getPicFramePosition(i);
            picFrame->move(pos.x, pos.y);
        }
    } else if (newEndDisplayIndex < endDisplayIndex) { // release frames at the bottom
        for (int i = endDisplayIndex - 1; i >= std::max(newEndDisplayIndex, startDisplayIndex); i--) {
            picFramePool->release(picFrames[i]);
            picFrames[i] = nullptr;
        }
    }
    endDisplayIndex = newEndDisplayIndex;

    displaying = true;
}

void DisplayController::handleWindowResize() {
    this->totalWidth = ui->picBrowseScrollArea->width();
    this->viewportHeight = ui->picBrowseScrollArea->height();
    this->picsPerRow = std::max(0, ((totalWidth - 2 * MARGIN - PIC_FRAME_WIDTH) / (PIC_FRAME_WIDTH + SPACING))) + 1;
    if (displayItems) {
        displayPicFrames();
        for (int i = startDisplayIndex; i < endDisplayIndex; i++) {
            Vec2 pos = getPicFramePosition(i);
            picFrames[i]->move(pos.x, pos.y);
        }
    }
}
void DisplayController::handleScrollBarValueChanged(int value) {
    this->scrollBarValue = value;
    displayPicFrames();
}

void DisplayController::sortDisplayItems(const SortContext& sortContext) {
    if (!displayItems) return; // no display items to display

    if (displayMode == DisplayItemType::Pic) {
        std::sort(sortedItemIndices.begin(), sortedItemIndices.end(), [&](int a, int b) {
            return comparePicItems(displayItems->picItems[a], displayItems->picItems[b], sortContext);
        });
    } else if (displayMode == DisplayItemType::Metadata) {
        std::sort(sortedItemIndices.begin(), sortedItemIndices.end(), [&](int a, int b) {
            return compareMetadataItems(displayItems->metadataItems[a], displayItems->metadataItems[b], sortContext);
        });
    }
    refreshDisplay();
    displayPicFrames();
}
void DisplayController::setFilterContext(const FilterContext& filterContext) {
    this->filterCtx = filterContext;
    refreshDisplay();
    displayPicFrames();
}