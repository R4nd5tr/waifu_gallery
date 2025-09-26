#include "main_window.h"
#include "ui_main_window.h"
#include "../service/database.h"
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    database = PicDatabase(QString("main_thread"));
    initInterface();
    initWorkerThreads();
    connectSignalSlots();

}
MainWindow::~MainWindow() {
    delete ui;
    loaderWorkerThread->quit();
    loaderWorkerThread->wait();
}
void MainWindow::initInterface() {
    ui->selectedTagScrollArea->hide();
    fillComboBox();
}
void MainWindow::fillComboBox() {
    ui->searchComboBox->addItem("选择搜索字段");
    ui->searchComboBox->addItem("图片ID");
    ui->searchComboBox->addItem("pixiv作品ID");
    ui->searchComboBox->addItem("pixiv作者ID");
    ui->searchComboBox->addItem("pixiv作者名");
    ui->searchComboBox->addItem("pixiv标题");
    ui->searchComboBox->addItem("pixiv描述");
    ui->searchComboBox->addItem("推特ID");
    ui->searchComboBox->addItem("推特作者ID");
    ui->searchComboBox->addItem("推特作者名");
    ui->searchComboBox->addItem("推特作者别名");
    ui->searchComboBox->addItem("推特描述");
    ui->searchComboBox->setCurrentIndex(0);

    ui->sortComboBox->addItem("无");
    ui->sortComboBox->addItem("图片ID");
    ui->sortComboBox->addItem("日期");
    ui->sortComboBox->addItem("大小");
    ui->sortComboBox->setCurrentIndex(0);

    ui->orderComboBox->addItem("递增");
    ui->orderComboBox->addItem("递减");
    ui->orderComboBox->setCurrentIndex(0);
}
void MainWindow::initWorkerThreads() {
    loaderWorkerThread = new QThread(this);
    LoaderWorker* loaderWorker = new LoaderWorker(this);
    loaderWorker->moveToThread(loaderWorkerThread);
    connect(this, &MainWindow::loadImage, loaderWorker, &LoaderWorker::loadImage);
    connect(loaderWorker, &LoaderWorker::loadComplete, this, &MainWindow::displayImage);
    connect(loaderWorkerThread, &QThread::finished, loaderWorker, &QObject::deleteLater);
    loaderWorkerThread->start();

    databaseWorkerThread = new QThread(this);
    DatabaseWorker* databaseWorker = new DatabaseWorker(QString("database_thread"), this);
    databaseWorker->moveToThread(databaseWorkerThread);
    connect(this, &MainWindow::scanDirectory, databaseWorker, &DatabaseWorker::scanDirectory);
    connect(this, &MainWindow::searchPics, databaseWorker, &DatabaseWorker::searchPics);
    connect(databaseWorker, &DatabaseWorker::searchComplete, this, &MainWindow::handleSearchResults);
    connect(databaseWorkerThread, &QThread::finished, databaseWorker, &QObject::deleteLater);
    databaseWorkerThread->start();
}
void MainWindow::connectSignalSlots() {
    resolutionTimer = new QTimer(this);
    ratioSortTimer = new QTimer(this);
    textSearchTimer = new QTimer(this);
    tagClickTimer = new QTimer(this);
    resolutionTimer->setSingleShot(true);
    ratioSortTimer->setSingleShot(true);
    textSearchTimer->setSingleShot(true);
    tagClickTimer->setSingleShot(true);

    connect(ui->jpgCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowJPG);
    connect(ui->pngCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowPNG);
    connect(ui->gifCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowGIF);
    connect(ui->webpCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowWEBP);
    connect(ui->unknowRestrictCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowUnknowRestrict);
    connect(ui->allAgeCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowAllAge);
    connect(ui->r18CheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowR18);
    connect(ui->r18gCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowR18g);
    connect(ui->unknowAICheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowUnknowAI);
    connect(ui->aiCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowAI);
    connect(ui->noAICheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowNonAI);
    
    connect(ui->maxHeightEdit, &QLineEdit::textChanged, this, &MainWindow::updateMaxHeight);
    connect(ui->maxHeightEdit, &QLineEdit::textChanged, this, [this]() {resolutionTimer->start(300);});
    connect(ui->minHeightEdit, &QLineEdit::textChanged, this, &MainWindow::updateMinHeight);
    connect(ui->minHeightEdit, &QLineEdit::textChanged, this, [this]() {resolutionTimer->start(300);});
    connect(ui->maxWidthEdit, &QLineEdit::textChanged, this, &MainWindow::updateMaxWidth);
    connect(ui->maxWidthEdit, &QLineEdit::textChanged, this, [this]() {resolutionTimer->start(300);});
    connect(ui->minWidthEdit, &QLineEdit::textChanged, this, &MainWindow::updateMinWidth);
    connect(ui->minWidthEdit, &QLineEdit::textChanged, this, [this]() {resolutionTimer->start(300);});
    connect(resolutionTimer, &QTimer::timeout, this, &MainWindow::handleResolutionTimerTimeout);
    
    connect(ui->sortComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortBy);
    connect(ui->orderComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortOrder);
    connect(ui->enableRatioCheckBox, &QCheckBox::toggled, this, &MainWindow::updateEnableRatioSort);
    connect(ui->ratioSlider, &QSlider::valueChanged, this, &MainWindow::updateRatioSlider);
    connect(ui->ratioSlider, &QSlider::valueChanged, this, [this]() {ratioSortTimer->start(300);});
    connect(ui->widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(ui->widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {ratioSortTimer->start(300);});
    connect(ui->heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(ui->heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {ratioSortTimer->start(300);});
    connect(ratioSortTimer, &QTimer::timeout, this, &MainWindow::handleRatioTimerTimeout);
    
    connect(ui->searchComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSearchField);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchText);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, [this]() {textSearchTimer->start(300);});
    connect(textSearchTimer, &QTimer::timeout, this, &MainWindow::picSearch);

    connect(ui->generalTagList, &QListWidget::itemClicked, this, &MainWindow::addIncludedTags);
    connect(ui->generalTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->characterTagList, &QListWidget::itemClicked, this, &MainWindow::addIncludedTags);
    connect(ui->characterTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->pixivTagList, &QListWidget::itemClicked, this, &MainWindow::addIncludedTags);
    connect(ui->pixivTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->twitterHashtagList, &QListWidget::itemClicked, this, &MainWindow::addIncludedTags);
    connect(ui->twitterHashtagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
}
    
void MainWindow::loadTags() {
    generalTags = database.getGeneralTags();
    characterTags = database.getCharacterTags();
    pixivTags = database.getPixivTags();
    twitterHashtags = database.getTwitterHashtags();
    ui->generalTagList->clear();
    ui->characterTagList->clear();
    ui->pixivTagList->clear();
    ui->twitterHashtagList->clear();
    QStringList generalTagNames;
    for (const auto& tag : generalTags) {
        generalTagNames.append(QString("%1 (%2)").arg(QString::fromStdString(tag.first)).arg(tag.second));
    }
    ui->generalTagList->addItems(generalTagNames);
    for (size_t i = 0; i < generalTags.size(); i++) {
        ui->generalTagList->item(i)->setData(Qt::UserRole, QString::fromStdString(generalTags[i].first));
    }
    

    QStringList characterTagNames;
    for (const auto& tag : characterTags) {
        characterTagNames.append(QString("%1 (%2)").arg(QString::fromStdString(tag.first)).arg(tag.second));
    }
    ui->characterTagList->addItems(characterTagNames);
    for (size_t i = 0; i < characterTags.size(); i++) {
        ui->characterTagList->item(i)->setData(Qt::UserRole, QString::fromStdString(characterTags[i].first));
    }

    QStringList pixivTagNames;
    for (const auto& tag : pixivTags) {
        pixivTagNames.append(QString("%1 (%2)").arg(QString::fromStdString(tag.first)).arg(tag.second));
    }
    ui->pixivTagList->addItems(pixivTagNames);
    for (size_t i = 0; i < pixivTags.size(); i++) {
        ui->pixivTagList->item(i)->setData(Qt::UserRole, QString::fromStdString(pixivTags[i].first));
    }

    QStringList twitterHashtagNames;
    for (const auto& tag : twitterHashtags) {
        twitterHashtagNames.append(QString("%1 (%2)").arg(QString::fromStdString(tag.first)).arg(tag.second));
    }
    ui->twitterHashtagList->addItems(twitterHashtagNames);
    for (size_t i = 0; i < twitterHashtags.size(); i++) {
        ui->twitterHashtagList->item(i)->setData(Qt::UserRole, QString::fromStdString(twitterHashtags[i].first));
    }
}
void MainWindow::updateShowJPG(bool checked) {
    showJPG = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowPNG(bool checked) {
    showPNG = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowGIF(bool checked) {
    showGIF = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowWEBP(bool checked) {
    showWEBP = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowUnknowRestrict(bool checked) {
    showUnknowRestrict = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowAllAge(bool checked) {
    showAllAge = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowR18(bool checked) {
    showR18 = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowR18g(bool checked) {
    showR18g = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowUnknowAI(bool checked) {
    showUnknowAI = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowAI(bool checked) {
    showAI = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowNonAI(bool checked) {
    showNonAI = checked;
    refreshPicDisplay();
}
void MainWindow::updateMaxWidth(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) maxWidth = value;
}
void MainWindow::updateMinWidth(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) minWidth = value;
}
void MainWindow::updateMaxHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) maxHeight = value;
}
void MainWindow::updateMinHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) minHeight = value;
}
void MainWindow::handleResolutionTimerTimeout() {
    refreshPicDisplay();
}
void MainWindow::updateSortBy(int index) {
    sortBy = static_cast<SortBy>(index);
    sortPics();
    refreshPicDisplay();
}
void MainWindow::updateSortOrder(int index) {
    sortOrder = static_cast<SortOrder>(index);
    sortPics();
    refreshPicDisplay();
}
void MainWindow::updateEnableRatioSort(bool checked) {
    ratioSortEnabled = checked;
    if (ratioSortEnabled) {
        ui->sortComboBox->setDisabled(true);
        ui->orderComboBox->setDisabled(true);
    } else {
        ui->sortComboBox->setEnabled(true);
        ui->orderComboBox->setEnabled(true);
    }
    sortPics();
    refreshPicDisplay();
}
int ratioToSliderValue(double ratio) {
    if (ratio <= 0) {
        return 40;
    } else if (ratio >= 3) {
        return -40;
    }
    if (ratio < 1.0) {
        return static_cast<int>(((1/(ratio)) - 1) * 20);
    } else {
        return static_cast<int>((1 - ratio) * 20);
    }
}
void MainWindow::updateRatioSlider(int value) {
    if (ratioSpinBoxEditing) return;
    ratioSliderEditing = true;
    ratioSliderValue = value;
    if (value >= 0) {
        double temp = 1.0 + (value / 20.0);
        ratio = 1.0 / temp;
        widthRatioSpinBoxValue = 1.0;
        heightRatioSpinBoxValue = temp;
    } else {
        double temp = 1.0 - (value / 20.0);
        ratio = temp;
        widthRatioSpinBoxValue = temp;
        heightRatioSpinBoxValue = 1.0;
    }
    ui->widthRatioSpinBox->setValue(widthRatioSpinBoxValue);
    ui->heightRatioSpinBox->setValue(heightRatioSpinBoxValue);
    ratioSliderEditing = false;
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
    }
}
void MainWindow::updateRatioSpinBox(double value) {
    if (ratioSliderEditing) return;
    ratioSpinBoxEditing = true;
    widthRatioSpinBoxValue = ui->widthRatioSpinBox->value();
    heightRatioSpinBoxValue = ui->heightRatioSpinBox->value();
    if (widthRatioSpinBoxValue == 0) {
        ratio = 0;
    } else if (heightRatioSpinBoxValue == 0) {
        ratio = std::numeric_limits<double>::infinity();
    } else {
        ratio = widthRatioSpinBoxValue / heightRatioSpinBoxValue;
    }
    ui->ratioSlider->setValue(ratioToSliderValue(ratio));
    ratioSpinBoxEditing = false;
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
    }
}
void MainWindow::handleRatioTimerTimeout() {
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
    }
}
void MainWindow::updateSearchField(int index) {
    searchField = static_cast<SearchField>(index);
}
void MainWindow::updateSearchText(const QString& text) {
    searchText = text.toStdString();
    searchTextChanged = true;
}
void MainWindow::addIncludedTags(QListWidgetItem* item) {
    tagClickTimer->start(400);
    if (tagDoubleClicked) return;
    QListWidget* listWidget = item->listWidget();
    std::string tag = item->data(Qt::UserRole).toString().toStdString();
    if (listWidget == ui->generalTagList || listWidget == ui->characterTagList) {
        if (includedTags.find(tag) == includedTags.end()) {
            includedTags.insert(tag);
            selectedTagChanged = true;
        }
    } else if (listWidget == ui->pixivTagList) {
        if (includedPixivTags.find(tag) == includedPixivTags.end()) {
            includedPixivTags.insert(tag);
            selectedPixivTagChanged = true;
        }
    } else if (listWidget == ui->twitterHashtagList) {
        if (includedTweetTags.find(tag) == includedTweetTags.end()) {
            includedTweetTags.insert(tag);
            selectedTweetTagChanged = true;
        }
    }
}
void MainWindow::addExcludedTags(QListWidgetItem* item) {
    tagDoubleClicked = true;
    tagClickTimer->stop();
    QListWidget* listWidget = item->listWidget();
    std::string tag = item->data(Qt::UserRole).toString().toStdString();
    if (listWidget == ui->generalTagList || listWidget == ui->characterTagList) {
        if (excludedTags.find(tag) == excludedTags.end()) {
            excludedTags.insert(tag);
            selectedTagChanged = true;
        }
    } else if (listWidget == ui->pixivTagList) {
        if (excludedPixivTags.find(tag) == excludedPixivTags.end()) {
            excludedPixivTags.insert(tag);
            selectedPixivTagChanged = true;
        }
    } else if (listWidget == ui->twitterHashtagList) {
        if (excludedTweetTags.find(tag) == excludedTweetTags.end()) {
            excludedTweetTags.insert(tag);
            selectedTweetTagChanged = true;
        }
    }
    tagDoubleClicked = false;
}
void MainWindow::handleWindowSizeChange() {
    int width = ui->picBrowseWidget->width();
    widgetsPerRow = width / 250;
    if (widgetsPerRow < 1) widgetsPerRow = 1;
    rearrangePicFrames();
}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    handleWindowSizeChange();
}
void MainWindow::picSearch() {
    clearAllPicFrames();
    ui->statusbar->showMessage("正在搜索...");
    searchRequestId++;
    emit searchPics(includedTags, excludedTags,
                    includedPixivTags, excludedPixivTags,
                    includedTweetTags, excludedTweetTags,
                    searchText, searchField,
                    selectedTagChanged, selectedPixivTagChanged, selectedTweetTagChanged, searchTextChanged,
                    searchRequestId);
}
void MainWindow::handleSearchResults(std::vector<PicInfo>&& pics, size_t requestId) {
    if (requestId != searchRequestId) return;
    resultPics = std::move(pics);
    ui->statusbar->showMessage("搜索完成，找到 " + QString::number(resultPics.size()) + " 张图片");
    sortPics();
    refreshPicDisplay();
}
void MainWindow::refreshPicDisplay() {
    removePicFramesFromLayout();
    displayIndex = 0;
    displayingPicIds.clear();
    currentColumn = 0;
    currentRow = 0;
    loadMorePics();
}
bool MainWindow::isMatchFilter(const PicInfo& pic) {
    if (!showJPG && pic.fileType == "jpg") return false;
    if (!showPNG && pic.fileType == "png") return false;
    if (!showGIF && pic.fileType == "gif") return false;
    if (!showWEBP && pic.fileType == "webp") return false;
    if (!showUnknowRestrict && pic.xRestrict == XRestrictType::Unknown) return false;
    if (!showAllAge && pic.xRestrict == XRestrictType::AllAges) return false;
    if (!showR18 && pic.xRestrict == XRestrictType::R18) return false;
    if (!showR18g && pic.xRestrict == XRestrictType::R18G) return false;
    if (!showUnknowAI && pic.aiType == AIType::Unknown) return false;
    if (!showAI && pic.aiType == AIType::AI) return false;
    if (!showNonAI && pic.aiType == AIType::NotAI) return false;
    if (maxWidth != 0 && pic.width > maxWidth) return false;
    if (minWidth != 0 && pic.width < minWidth) return false;
    if (maxHeight != 0 && pic.height > maxHeight) return false;
    if (minHeight != 0 && pic.height < minHeight) return false;
    return true;
}
void MainWindow::loadMorePics() {
    int picsLoaded = 0;
    while (displayIndex < resultPics.size() && picsLoaded < LOAD_PIC_BATCH) {
        const PicInfo& pic = resultPics[displayIndex];
        displayIndex++;
        if (!isMatchFilter(pic)) continue;
        displayingPicIds.push_back(pic.id);
        auto frame = idToFrameMap.find(pic.id);
        if (frame != idToFrameMap.end()) {
            ui->picDisplayLayout->addWidget(frame->second, currentRow, currentColumn);
        } else {
            PictureFrame* picFrame;
            if (searchText.empty() || searchField == SearchField::None) {
                picFrame = new PictureFrame(this, pic);
            } else {
                picFrame = new PictureFrame(this, pic, searchField);
            }
            idToFrameMap[pic.id] = picFrame;
            ui->picDisplayLayout->addWidget(picFrame, currentRow, currentColumn);
            auto imgIt = imageThumbCache.find(pic.id);
            if (imgIt != imageThumbCache.end()) {
                picFrame->setPixmap(imgIt->second);
            } else {
                emit loadImage(pic.id, pic.filePaths);
            }
        }
        picsLoaded++;
        currentColumn++;
        if (currentColumn >= widgetsPerRow) {
            currentColumn = 0;
            currentRow++;
        }
    }
}
void MainWindow::displayImage(uint64_t picId, QPixmap&& img) {
    if (imageThumbCache.size() >= MAX_PIC_CACHE) {
        for (auto it = imageThumbCache.begin(); it != imageThumbCache.end(); ) {
            if (idToFrameMap.find(it->first) == idToFrameMap.end()) {
                it = imageThumbCache.erase(it);
            } else {
                ++it;
            }
        }
    }
    imageThumbCache[picId] = img;
    auto frameIt = idToFrameMap.find(picId);
    if (frameIt != idToFrameMap.end()) {
        frameIt->second->setPixmap(img);
    }
}
void MainWindow::sortPics() {
    auto comparator = [this](const PicInfo& a, const PicInfo& b) {
        if (ratioSortEnabled) {
            double ratioA = (a.height == 0) ? std::numeric_limits<double>::infinity() : static_cast<double>(a.width) / a.height;
            double ratioB = (b.height == 0) ? std::numeric_limits<double>::infinity() : static_cast<double>(b.width) / b.height;
            double diffA = std::abs(ratioA - ratio);
            double diffB = std::abs(ratioB - ratio);
            if (diffA != diffB) {
                return diffA < diffB;
            }
            return false; // if ratios are equally close, sort by ID to ensure consistent order
        }
        switch (sortBy) {
            case SortBy::None:
                return false;
            case SortBy::ID:
                if (sortOrder == SortOrder::Ascending) {
                    return a.id < b.id;
                } else {
                    return a.id > b.id;
                }
            case SortBy::Size:
                if (sortOrder == SortOrder::Ascending) {
                    return (a.size) < (b.size);
                } else {
                    return (a.size) > (b.size);
                }
            default:
                return false;
        }
    };
    std::sort(resultPics.begin(), resultPics.end(), comparator);
}
void MainWindow::rearrangePicFrames() {
    removePicFramesFromLayout();
    currentColumn = 0;
    currentRow = 0;
    for (const auto& picId : displayingPicIds) {
        auto frameIt = idToFrameMap.find(picId);
        if (frameIt != idToFrameMap.end()) {
            ui->picDisplayLayout->addWidget(frameIt->second, currentRow, currentColumn);
            currentColumn++;
            if (currentColumn >= widgetsPerRow) {
                currentColumn = 0;
                currentRow++;
            }
        }
    }
}
void MainWindow::clearAllPicFrames() {
    QLayoutItem* child;
    while ((child = ui->picDisplayLayout->takeAt(0)) != nullptr) {
        delete child;
    }

    for (auto& [id, frame] : idToFrameMap) {
        delete frame;
    }
    idToFrameMap.clear();
}
void MainWindow::removePicFramesFromLayout() {    
    QLayoutItem* child;
    while ((child = ui->picDisplayLayout->takeAt(0)) != nullptr) {
        delete child;
    }
}