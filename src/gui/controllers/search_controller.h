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
#include "display_controller.h"
#include "filter_sort_controller.h"
#include "service/model.h"
#include "support/context.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <chrono>
#include <string>
#include <vector>

struct SearchUi {
    QComboBox* searchPlatformComboBox;
    QComboBox* searchComboBox;
    QLineEdit* searchLineEdit;
    QListWidget* generalTagList;
    QListWidget* characterTagList;
    QListWidget* platformTagList;
    QPushButton* clearSearchTextButton;
    QStatusBar* statusbar;
    QScrollArea* selectedTagScrollArea;
    QWidget* selectedTagWidget;
    QTabWidget* tagListTabWidget;
    QHBoxLayout* selectedTagLayout;
    QPushButton* clearTextSearchButton;
    QLineEdit* searchTagTextEdit;
};

class SearchController : public QObject {
    Q_OBJECT
public:
    SearchController(SearchUi searchUi, DisplayController& displayController, FilterSortController& filterSortController)
        : ui(searchUi), displayController(displayController), filterSortController(filterSortController) {
        connectSignalSlots();
        fillComboBoxes();
        loadTags();
        displayTags();
    }
    ~SearchController() = default;

    void loadTags();
    void picSearch();
    bool isSearchCriteriaEmpty() const { return searchCtx.searchText.empty() && isSelectedTagsEmpty(); };
    void handleSearchResults(DisplayItems* displayItems,
                             const std::vector<TagCount>& availableTags,
                             const std::vector<PlatformTagCount>& availablePlatformTags,
                             size_t requestId);
signals:
    void searchPics(const SearchContext& ctx, size_t requestId);

private:
    SearchUi ui;
    DisplayController& displayController;
    FilterSortController& filterSortController;
    PicDatabase database;
    SearchContext searchCtx;
    QThread* searchWorkerThread = nullptr;
    std::vector<TagCount> allTags;
    std::vector<PlatformTagCount> allPlatformTags;
    void connectSignalSlots();
    void fillComboBoxes();
    // pic text search handlers
    void updateSearchPlatform(int index);
    void updateSearchField(int index);
    void updateSearchText(const QString& text);
    void clearSearchText();
    // pic tag search handlers
    void handleListWidgetItemSingleClick(QListWidgetItem* item);
    QListWidgetItem* lastClickedTagItem = nullptr;
    void addIncludedTags();
    void addExcludedTags(QListWidgetItem* item);
    void removeIncludedTags(QPushButton* button); // is there a better way to pass which tag to remove?
    void removeExcludedTags(QPushButton* button);
    void removeIncludedPlatformTags(QPushButton* button);
    void removeExcludedPlatformTags(QPushButton* button);
    QTimer tagClickTimer;  // use for double click detection
    QTimer tagSearchTimer; // use for debouncing removing selected tags
    bool tagDoubleClicked = false;
    bool isSelectedTagsEmpty() const {
        return searchCtx.includedTags.empty() && searchCtx.excludedTags.empty() && searchCtx.includedPlatformTags.empty() &&
               searchCtx.excludedPlatformTags.empty();
    };
    // tag search handler
    void tagSearch(const QString& text);
    // searching
    size_t searchRequestId = 0; // to identify latest search request
    void displayTags(const std::vector<TagCount>& tags = {}, const std::vector<PlatformTagCount>& platformTags = {});
};