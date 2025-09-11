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

enum SortBy {
    NoSorting,
    Id,
    Date,
    Size
};

enum SortOrder {
    Increasing,
    Decreasing
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void loadImage(uint64_t id, PictureFrame* picFramePtr, std::filesystem::path filePath); // load image in another thread

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // initialize
    Ui::MainWindow *ui;
    PicDatabase database;
    QThread* loaderWorkerThread = nullptr;
    QThread* databaseWorkerThread = nullptr;
    void connectSignalSlots();
    void initInterface();
    void fillComboBox();
    void loadTags();
    
    // context
    uint widgetsPerRow;
    bool showPNG = true;
    bool showJPG = true;
    bool showGIF = true;
    bool showWEBP = true;
    uint maxHeight;
    uint minHeight;
    uint maxWidth;
    uint minWidth;
    SortBy sortBy;
    SortOrder sortOrder = Increasing;
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
    void updateSortBy(int index);
    void updateSortOrder(int index);
    void updateEnableRatioSort(bool checked);
    void updateRatioSlider(int value);
    void updateRatioSpinBox(double value);
    void updateSearchField(int index);
    void updateSearchText(const QString& text);
    void updateWidthHeightLimit();
    void handleWindowSizeChange();
    
    // searching
    bool selectedTagChanged = false;
    std::vector<PicInfo> lastTagSearchResult;
    bool selectedPixivTagChanged = false;
    std::vector<PicInfo> lastPixivTagSearchResult;
    bool selectedTweetTagChanged = false;
    std::vector<PicInfo> lastTweetTagSearchResult;
    bool searchTextChanged = false;
    std::vector<PicInfo> lastTextSearchResult;
    void picSearch();         // add search result into displayingPics vector

    // displaying
    uint displayIndex;
    std::vector<PicInfo> displayingPics;
    std::unordered_set<uint64_t> displayingPicIds; // For cache cleanup, images not in this set will be deleted from the cache.
    std::unordered_map<uint64_t, QPixmap> imageThumbCache;
    std::vector<PictureFrame*> displayingPicFrames;
    void sortPics();          // sort displayingPics vector
    void refreshPicDisplay(); // clear widget, set displayIndex to 0, and call loadMorePics()
    void loadMorePics();      // check pictures match filter or not, add PictureFrame into widget, load image
    void displayImage(uint64_t picId, PictureFrame* picFramePtr, QPixmap img);
    void rearrangePicFrames();// rearrange PictureFrame based on window size change
    
};

#endif