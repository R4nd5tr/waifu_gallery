#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "worker.h"
#include "picture_frame.h"
#include "../service/model.h"
#include "../service/database.h"
#include <QMainWindow>
#include <QThread>
#include <QPixmap>
#include <QWidget>

namespace Ui {
    class MainWindow;
}

enum class SortBy {
    None,
    ID,
    Date,
    Size
};

enum class SortOrder {
    Ascending,
    Descending
};

const size_t MAX_PIC_CACHE = 1000; // max number of pictures in cache
const size_t LOAD_PIC_BATCH = 50; // number of pictures to load each time

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths); // load image in another thread

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // initialize
    Ui::MainWindow *ui;
    PicDatabase database;
    QThread* loaderWorkerThread = nullptr;
    QThread* databaseWorkerThread = nullptr;
    void initInterface();
    void fillComboBox();
    void initWorkerThreads();
    void connectSignalSlots();
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

    void updateSortBy(int index);
    void updateSortOrder(int index);
    void updateEnableRatioSort(bool checked);
    void updateRatioSlider(int value);
    void updateRatioSpinBox(double value);

    void updateSearchField(int index);
    void updateSearchText(const QString& text);
    
    void updateIncludedTags(const QStringList& tags);
    void updateExcludedTags(const QStringList& tags);
    void updateIncludedPixivTags(const QStringList& tags);
    void updateExcludedPixivTags(const QStringList& tags);
    void updateIncludedTweetTags(const QStringList& tags);
    void updateExcludedTweetTags(const QStringList& tags);
    void handleWindowSizeChange();

    // searching
    bool selectedTagChanged = false;
    std::vector<uint64_t> lastTagSearchResult;
    bool selectedPixivTagChanged = false;
    std::vector<uint64_t> lastPixivTagSearchResult;
    bool selectedTweetTagChanged = false;
    std::vector<uint64_t> lastTweetTagSearchResult;
    bool searchTextChanged = false;
    std::vector<uint64_t> lastTextSearchResult;
    void picSearch(); // clear and add search result into resultPics vector(ONLY FUNCTION THAT CLEAR AND REFILL THIS VECTOR)
                      //                                                                                                   |                                                   
    // displaying                                                                                                          |
    uint displayIndex;//                                                                                                   |
    std::vector<PicInfo> resultPics;//  <-----------------------------------------------------------------------------------
    std::vector<uint64_t> displayingPicIds;
    std::unordered_map<uint64_t, PictureFrame*> idToFrameMap; // remember to clear this when clearing resultPics vector, also use for clearing cache
    std::unordered_map<uint64_t, QPixmap> imageThumbCache;
    void refreshPicDisplay(); // clear widget, set displayIndex to 0, and call loadMorePics()
    bool matchFilter(const PicInfo& pic);
    void loadMorePics();      // check pictures match filter or not, add PictureFrame into widget and add id to displayingPicIds, load image
    void displayImage(uint64_t picId, const QPixmap& img); // slot for signal loadComplete
    void sortPics();          // sort resultPics vector
    void rearrangePicFrames();// rearrange PictureFrame based on window size change, use displayingPicIds and idToFrameMap
    void clearAllPicFrames();
    void removePicFramesFromLayout();
    
};

#endif