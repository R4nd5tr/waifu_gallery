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
#include "support/context.h"
#include "support/image_loader.h"
#include "support/picture_frame_pool.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>
#include <unordered_map>
#include <vector>

struct DisplayUi {
    QWidget* picBrowseWidget;
    QScrollBar* picBrowseScrollBar;
    QScrollArea* picBrowseScrollArea;
};
const int MARGIN = 20;
const int SPACING = 10;

const int PIC_FRAME_WIDTH = 250; // same as the one defined in picture_frame.ui, keep in sync
const int PIC_FRAME_HEIGHT = 325;

const int PRE_LOAD_ROWS = 3;

struct Vec2 {
    int x;
    int y;
};

class DisplayController : public QObject {
    Q_OBJECT
public:
    DisplayController(DisplayUi ui, ImageLoader& imageLoader);
    ~DisplayController();

    void setDisplayItems(DisplayItems* displayItems, SearchField searchField);

    void handleWindowResize();
    void handleScrollBarValueChanged(int value);

    void sortDisplayItems(const SortContext& sortContext);
    void setFilterContext(const FilterContext& filterContext);

    void displayImage(uint64_t picId, LoadType loadType); // asynchronous loaded image will be displayed through this function

private:
    DisplayUi ui;
    ImageLoader& imageLoader;
    PicFramePool picFramePool;

    DisplayItemType displayMode = DisplayItemType::Pic;
    DisplayItems* displayItems = nullptr;
    std::vector<int> sortedItemIndices; // index of displayItems in sorted order
    int nextMatchSortedIndex = 0;

    bool displaying = false;                // when displaying, everything below should be valid
    std::vector<int> displayingItemIndices; // indices of displayItems filtered and currently being displayed
    std::vector<PictureFrame*> picFrames;   // displaying PictureFrames, corresponds to displayingIndices
    int startDisplayIndex = 0;              // [start, end) is the range of displaying PictureFrames in picFrames
    int endDisplayIndex = 0;                // and indices in displayingItemIndices currently being displayed
    std::unordered_map<uint64_t, int> picIdToFrameIdxMap;

    FilterContext filterCtx;
    SearchField searchField = SearchField::None;

    int totalWidth = 0;
    int totalHeight = 0;
    int viewportHeight = 0;
    int scrollBarValue = 0;
    int picsPerRow = 1;

    int lookingAt = 0;
    bool resizing = false;

    void connectSignalSlots();

    Vec2 getPicFramePosition(int displayIndex) const;
    void displayPicFrames();
    void clearDisplay();
    bool fillFilteredItemUntil(int count);
};
