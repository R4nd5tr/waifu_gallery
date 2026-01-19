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
#include "about_dialog.h"
#include "image_loader.h"
#include "service/database.h"
#include "service/importer.h"
#include "service/model.h"
#include "service/tagger.h"
#include "settings_dialog.h"
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

const size_t MAX_PIC_CACHE = 1000;    // max number of pictures in cache
const int DEBOUNCE_DELAY = 300;       // ms for debouncing text input
const int SLIDER_DEBOUNCE_DELAY = 50; // ms for debouncing slider input
const int DOUBLE_CLICK_DELAY = 200;   // ms for double click detection

class ImportProgressReportEvent : public QEvent {
public:
    ImportProgressReportEvent(size_t progress, size_t total) : QEvent(EventType), progress(progress), total(total) {}
    static const QEvent::Type EventType;
    size_t progress;
    size_t total;
};
class TaggingProgressReportEvent : public QEvent {
public:
    TaggingProgressReportEvent(size_t progress, size_t total) : QEvent(EventType), progress(progress), total(total) {}
    static const QEvent::Type EventType;
    size_t progress;
    size_t total;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    ProgressCallback reportImportProgress = [this](size_t progress, size_t total) {
        QCoreApplication::postEvent(this, new ImportProgressReportEvent(progress, total));
    };
    ProgressCallback reportTaggingProgress = [this](size_t progress, size_t total) {
        QCoreApplication::postEvent(this, new TaggingProgressReportEvent(progress, total));
    };

signals:
    void searchPics(const std::unordered_set<uint32_t>& includedTags,
                    const std::unordered_set<uint32_t>& excludedTags,
                    const std::unordered_set<uint32_t>& includedPlatformTags,
                    const std::unordered_set<uint32_t>& excludedPlatformTags,
                    PlatformType platform,
                    SearchField searchField,
                    const std::string& searchText,
                    size_t requestId);

protected:
    bool firstShow_ = true;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    // initialize
    Ui::MainWindow* ui;
    PicDatabase database;
    QThread* searchWorkerThread = nullptr;
    ImageLoader imageLoadThreadPool{this};   // Blazing fast!!!
    Importer importer{reportImportProgress}; // Blazing fast!!!
    Tagger tagger{reportTaggingProgress};
    void initInterface();
    void fillComboBox();
    void initWorkerThreads();
    void connectSignalSlots();
    std::vector<TagCount> allTags;
    std::vector<PlatformTagCount> allPlatformTags;
    void loadTags();
    void initTagger();

    // context
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
    double ratio = 1.0f;
    bool ratioSortEnabled = false;
    bool ratioSliderEditing = false;
    bool ratioSpinBoxEditing = false;
    PlatformType searchPlatform = PlatformType::Unknown;
    SearchField searchField = SearchField::None;
    std::string searchText;
    std::unordered_set<uint32_t> includedTags;
    std::unordered_set<uint32_t> excludedTags;
    std::unordered_set<uint32_t> includedPlatformTags;
    std::unordered_set<uint32_t> excludedPlatformTags;
    // filter update handlers
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
    // resolution filter handlers
    void updateMaxWidth(const QString& text);
    void updateMaxHeight(const QString& text);
    void updateMinWidth(const QString& text);
    void updateMinHeight(const QString& text);
    void clearResolutionFilters();
    QTimer* resolutionTimer;
    void handleResolutionTimerTimeout();
    // sort handlers
    void updateSortBy(int index);
    void updateSortOrder(int index);
    void updateEnableRatioSort(bool checked);
    void updateRatioSlider(int value);
    void updateRatioSpinBox(double value);
    QTimer* ratioSortTimer;
    void handleRatioTimerTimeout();
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
    QTimer* tagClickTimer;  // use for double click detection
    QTimer* tagSearchTimer; // use for debouncing removing selected tags
    bool tagDoubleClicked = false;
    bool isSelectedTagsEmpty() {
        return includedTags.empty() && excludedTags.empty() && includedPlatformTags.empty() && excludedPlatformTags.empty();
    };
    // tag search handler
    void tagSearch(const QString& text);

    // Action handlers
    std::chrono::steady_clock::time_point taskStartTime;
    std::vector<std::pair<std::filesystem::path, ParserType>> dirsToImport; // paths to import with specified parser types
    void handleImportNewPicsAction();
    void handleImportPowerfulPixivDownloaderAction(); // specify parser type pixiv
    void handleImportGallery_dlTwitterAction();       // specify parser type twitter
    void handleImportExistingDirectoriesAction();
    void handleStartTaggingAction();

    AboutDialog* aboutDialog = nullptr;
    void handleShowAboutAction();
    SettingsDialog* settingsDialog = nullptr;
    void handleShowSettingsAction();

    // task progress
    void displayImportProgress(size_t progress, size_t total);
    void finalizeImport(size_t totalImported);
    void displayTaggingProgress(size_t progress, size_t total);
    void finalizeTagging(size_t totalTagged);
    void cancelTask();

    bool haveOngoingTask() { return !(importer.finish() && tagger.finish()); };

    // searching
    size_t searchRequestId = 0; // to identify latest search request
    void picSearch();
    void handleSearchResults(const std::vector<PicInfo>& pics,
                             std::vector<TagCount> availableTags,
                             std::vector<PlatformTagCount> availablePlatformTags,
                             size_t requestId);
    void displayTags(const std::vector<TagCount>& tags = {}, const std::vector<PlatformTagCount>& platformTags = {});
    bool isSearchCriteriaEmpty() { return searchText.empty() && isSelectedTagsEmpty(); };

    // displaying   TODO: refactor display logic, make it accept picinfo and metadata directly
    uint picsPerRow;
    uint currentColumn;
    uint currentRow;
    void handleWindowSizeChange();
    void displayMorePicOnScroll(int value);

    uint displayIndex;
    std::vector<PicInfo> noMetadataPics; // pics without metadata, displayed when no filters are set
    std::vector<PicInfo> resultPics;
    std::vector<uint64_t> displayingPicIds;                   // use for rearranging layout when window resized
    std::unordered_map<uint64_t, PictureFrame*> idToFrameMap; // cache created frames, also use for clearing cache
    std::unordered_map<uint64_t, QPixmap> imageThumbCache;    // TODO: optimize memory usage? implement LRU cache?
    void refreshPicDisplay();                                 // clear widget, set displayIndex to 0, and call displayMorePics()
    bool isMatchFilter(const PicInfo& pic);
    void displayMorePics(uint rows = 5); // display more pics in the scroll area
    void sortPics();                     // sort resultPics vector
    void clearAllPicFrames();
    void removePicFramesFromLayout();
    void displayImage(uint64_t picId, QImage&& img);
};
