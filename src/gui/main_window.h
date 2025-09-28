#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../service/database.h"
#include "../service/model.h"
#include "picture_frame.h"
#include "worker.h"
#include <QListWidgetItem>
#include <QMainWindow>
#include <QPixmap>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QWidget>

namespace Ui {
class MainWindow;
}

enum class SortBy { None, ID, Date, Size };

enum class SortOrder { Ascending, Descending };

enum class DisplayingItem { PicInfo, PixivInfo, TweetInfo };

const size_t MAX_PIC_CACHE = 1000; // max number of pictures in cache
const size_t LOAD_PIC_BATCH = 50;  // number of pictures to load each time

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

signals:
    void loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths); // load image in another thread
    void scanDirectory(const std::filesystem::path& directory);
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

public slots:
    void displayImage(uint64_t picId, const QPixmap& img);
    void handleSearchResults(const std::vector<PicInfo>& resultPics,
                             std::vector<std::pair<std::string, int>> availableTags,
                             std::vector<std::pair<std::string, int>> availablePixivTags,
                             std::vector<std::pair<std::string, int>> availableTwitterHashtags,
                             size_t requestId);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    // initialize
    Ui::MainWindow* ui;
    PicDatabase database;
    QThread* loaderWorkerThread = nullptr;
    QThread* databaseWorkerThread = nullptr;
    void initInterface();
    void fillComboBox();
    void initWorkerThreads();
    void connectSignalSlots();
    std::vector<std::pair<std::string, int>> generalTags;
    std::vector<std::pair<std::string, int>> characterTags;
    std::vector<std::pair<std::string, int>> pixivTags;
    std::vector<std::pair<std::string, int>> twitterHashtags;
    std::unordered_map<std::string, bool> isCharacterTag; // tag -> isCharacter (I don't like this, maybe split character tags and
                                                          // general tags in the database and data model?)
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
    bool showR18 = false;
    bool showR18g = false;
    bool showUnknowAI = true;
    bool showAI = true;
    bool showNonAI = true;
    uint maxHeight;
    uint minHeight;
    uint maxWidth;
    uint minWidth;
    SortBy sortBy = SortBy::None;
    SortOrder sortOrder = SortOrder::Descending;
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
    void updateShowR18(bool checked);
    void updateShowR18g(bool checked);
    void updateShowUnknowAI(bool checked);
    void updateShowAI(bool checked);
    void updateShowNonAI(bool checked);

    void updateMaxWidth(const QString& text);
    void updateMaxHeight(const QString& text);
    void updateMinWidth(const QString& text);
    void updateMinHeight(const QString& text);
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
    QTimer* textSearchTimer;

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
    QTimer* tagClickTimer;
    QTimer* tagSearchTimer;
    bool tagDoubleClicked = false;

    void handleWindowSizeChange();

    // searching
    bool selectedTagChanged = false;
    bool selectedPixivTagChanged = false;
    bool selectedTweetTagChanged = false;
    bool searchTextChanged = false;
    size_t searchRequestId = 0; // to identify latest search request
    void picSearch();

    // displaying   TODO: refactor display logic, make it accept picinfo, pixivinfo and tweetinfo
    void displayTags();
    void displayTags(const std::vector<std::pair<std::string, int>>& tags,
                     const std::vector<std::pair<std::string, int>>& pixivTags,
                     const std::vector<std::pair<std::string, int>>& twitterHashtags);

    uint displayIndex;
    std::vector<PicInfo> resultPics;
    std::vector<uint64_t> displayingPicIds;
    std::unordered_map<uint64_t, PictureFrame*>
        idToFrameMap; // remember to clear this when clearing resultPics vector, also use for clearing cache
    std::unordered_map<uint64_t, QPixmap> imageThumbCache;
    void refreshPicDisplay(); // clear widget, set displayIndex to 0, and call loadMorePics()
    bool isMatchFilter(const PicInfo& pic);
    void
    loadMorePics(); // check pictures match filter or not, add PictureFrame into widget and add id to displayingPicIds, load image
    void sortPics();           // sort resultPics vector
    void rearrangePicFrames(); // rearrange PictureFrame based on window size change, use displayingPicIds and idToFrameMap
    void clearAllPicFrames();
    void removePicFramesFromLayout();
};

#endif