/*
 * Waifu Gallery - A Qt-based image gallery application.
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
#include "service/database.h"
#include "service/model.h"
#include "thread_pool.h"
#include "widgets/picture_frame.h"
#include "worker.h"
#include <QCoreApplication>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPixmap>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <chrono>

namespace Ui {
class MainWindow;
}

enum class SortBy { None, ID, DownloadDate, EditDate, Size, Filename, Width, Height };

enum class SortOrder { Ascending, Descending };

enum class DisplayingItem { PicInfo, PixivInfo, TweetInfo };

const size_t MAX_PIC_CACHE = 1000;    // max number of pictures in cache
const size_t LOAD_PIC_BATCH = 50;     // number of pictures to load each time
const int DEBOUNCE_DELAY = 300;       // ms for debouncing text input
const int SLIDER_DEBOUNCE_DELAY = 50; // ms for debouncing slider input
const int DOUBLE_CLICK_DELAY = 200;   // ms for double click detection

class ImportProgressReportEvent : public QEvent {
public:
    static const QEvent::Type EventType;
    ImportProgressReportEvent(size_t progress, size_t total) : QEvent(EventType), progress(progress), total(total) {}
    size_t progress;
    size_t total;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    ImportProgressCallback reportImportProgress = [this](size_t progress, size_t total) {
        QCoreApplication::postEvent(this, new ImportProgressReportEvent(progress, total));
    };

signals:
    void searchPics(const std::unordered_set<std::string>& includedTags,
                    const std::unordered_set<std::string>& excludedTags,
                    const std::unordered_set<std::string>& includedPixivTags,
                    const std::unordered_set<std::string>& excludedPixivTags,
                    const std::unordered_set<std::string>& includedTweetTags,
                    const std::unordered_set<std::string>& excludedTweetTags,
                    const std::string& searchText,
                    SearchField searchField,
                    bool selectedTagChanged,
                    bool selectedPixivTagChanged,
                    bool selectedTweetTagChanged,
                    bool searchTextChanged,
                    size_t requestId);
    void reloadWorkerDatabase();

protected:
    bool firstShow_ = true;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    // initialize
    Ui::MainWindow* ui;
    PicDatabase database;                          // Normal mode by default, for tag loading and user changes
    ImageLoadThreadPool imageLoadThreadPool{this}; // Blazing fast!!!
    QThread* searchWorkerThread = nullptr;
    MultiThreadedImporter* importer = nullptr; // only one importer at a time, Blazing fast!!!
    void initInterface();
    void fillComboBox();
    void initWorkerThreads();
    void connectSignalSlots();
    std::vector<std::tuple<std::string, int, bool>> allTags; // (tag, count, isCharacter)
    std::vector<std::pair<std::string, int>> allPixivTags;
    std::vector<std::pair<std::string, int>> allTwitterHashtags;
    void loadTags();

    // context
    uint widgetsPerRow;
    uint currentColumn;
    uint currentRow;
    bool showPNG = true;
    bool showJPG = true;
    bool showGIF = true;
    bool showWEBP = true;
    bool showUnknowRestrict = true;
    bool showAllAge = true;
    bool showSensitive = false;
    bool showQuestionable = false;
    bool showR18 = false;
    bool showR18g = false;
    bool showUnknowAI = true;
    bool showAI = true;
    bool showNonAI = true;
    uint maxHeight = std::numeric_limits<uint>::max();
    uint minHeight = 0;
    uint maxWidth = std::numeric_limits<uint>::max();
    uint minWidth = 0;
    SortBy sortBy = SortBy::None;
    SortOrder sortOrder = SortOrder::Ascending;
    int ratioSliderValue;
    double widthRatioSpinBoxValue;
    double heightRatioSpinBoxValue;
    double ratio;
    bool ratioSortEnabled = false;
    bool ratioSliderEditing = false;
    bool ratioSpinBoxEditing = false;
    SearchField searchField;
    std::string searchText;
    std::unordered_set<std::string> includedTags;
    std::unordered_set<std::string> excludedTags;
    std::unordered_set<std::string> includedPixivTags;
    std::unordered_set<std::string> excludedPixivTags;
    std::unordered_set<std::string> includedTweetTags;
    std::unordered_set<std::string> excludedTweetTags;
    void updateShowPNG(bool checked);
    void updateShowJPG(bool checked);
    void updateShowGIF(bool checked);
    void updateShowWEBP(bool checked);
    void updateShowUnknowRestrict(bool checked);
    void updateShowAllAge(bool checked);
    void updateShowSensitive(bool checked);
    void updateShowQuestionable(bool checked);
    void updateShowR18(bool checked);
    void updateShowR18g(bool checked);
    void updateShowUnknowAI(bool checked);
    void updateShowAI(bool checked);
    void updateShowNonAI(bool checked);

    void updateMaxWidth(const QString& text);
    void updateMaxHeight(const QString& text);
    void updateMinWidth(const QString& text);
    void updateMinHeight(const QString& text);
    void clearResolutionFilters();
    QTimer* resolutionTimer;
    void handleResolutionTimerTimeout();

    void updateSortBy(int index);
    void updateSortOrder(int index);
    void updateEnableRatioSort(bool checked);
    void updateRatioSlider(int value);
    void updateRatioSpinBox(double value);
    QTimer* ratioSortTimer;
    void handleRatioTimerTimeout();

    void updateSearchField(int index);
    void updateSearchText(const QString& text);

    void handleListWidgetItemSingleClick(QListWidgetItem* item);
    QListWidgetItem* lastClickedTagItem = nullptr;
    void addIncludedTags();
    void addExcludedTags(QListWidgetItem* item);
    void removeIncludedTags(QPushButton* button); // is there a better way to pass which tag to remove?
    void removeExcludedTags(QPushButton* button);
    void removeIncludedPixivTags(QPushButton* button);
    void removeExcludedPixivTags(QPushButton* button);
    void removeIncludedTweetTags(QPushButton* button);
    void removeExcludedTweetTags(QPushButton* button);
    QTimer* tagClickTimer;  // use for double click detection
    QTimer* tagSearchTimer; // use for debouncing removing selected tags
    bool tagDoubleClicked = false;
    bool isSelectedTagsEmpty() {
        return includedTags.empty() && excludedTags.empty() && includedPixivTags.empty() && excludedPixivTags.empty() &&
               includedTweetTags.empty() && excludedTweetTags.empty();
    };

    void handleWindowSizeChange();

    void handlescrollBarValueChange(int value);

    void tagSearch(const QString& text);

    void handleAddNewPicsAction();
    void handleAddPowerfulPixivDownloaderAction(); // specify parser type
    void handleAddGallery_dlTwitterAction();       // specify parser type
    void displayImportProgress(size_t progress, size_t total);
    std::chrono::steady_clock::time_point ImportStartTime;

    // searching
    bool selectedTagChanged = false;
    bool selectedPixivTagChanged = false;
    bool selectedTweetTagChanged = false;
    bool searchTextChanged = false;
    size_t searchRequestId = 0; // to identify latest search request
    void picSearch();
    void handleSearchResults(const std::vector<PicInfo>& resultPics,
                             std::vector<std::tuple<std::string, int, bool>> availableTags,
                             std::vector<std::pair<std::string, int>> availablePixivTags,
                             std::vector<std::pair<std::string, int>> availableTwitterHashtags,
                             size_t requestId);
    void displayTags(const std::vector<std::tuple<std::string, int, bool>>& tags = {},
                     const std::vector<std::pair<std::string, int>>& pixivTags = {},
                     const std::vector<std::pair<std::string, int>>& twitterHashtags = {});
    bool isSearchCriteriaEmpty() { return searchText.empty() && isSelectedTagsEmpty(); };

    // displaying   TODO: refactor display logic, make it accept picinfo, pixivinfo and tweetinfo
    uint displayIndex;
    std::vector<PicInfo> noMetadataPics; // pics without metadata, displayed when no filters are set
    std::vector<PicInfo> resultPics;
    std::vector<uint64_t> displayingPicIds;                   // use for rearranging layout when window resized
    std::unordered_map<uint64_t, PictureFrame*> idToFrameMap; // cache created frames, also use for clearing cache
    std::unordered_map<uint64_t, QPixmap> imageThumbCache;    // TODO: optimize memory usage?
    void refreshPicDisplay();                                 // clear widget, set displayIndex to 0, and call loadMorePics()
    bool isMatchFilter(const PicInfo& pic);
    void loadMorePics();
    void sortPics(); // sort resultPics vector
    void clearAllPicFrames();
    void removePicFramesFromLayout();
    void displayImage(uint64_t picId, QImage&& img);
};
