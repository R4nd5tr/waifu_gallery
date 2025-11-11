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

#include "main_window.h"
#include "service/database.h"
#include "ui_main_window.h"
#include <QFileDialog>
#include <QScrollBar>
#include <QString>

const QEvent::Type ImportProgressReportEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initInterface();
    initWorkerThreads();
    connectSignalSlots();
    loadTags();
    displayTags();
    noMetadataPics = database.getNoMetadataPics();
}
MainWindow::~MainWindow() {
    delete ui;
    searchWorkerThread->quit();
    searchWorkerThread->wait();
}
void MainWindow::initInterface() {
    ui->selectedTagScrollArea->hide();
    ui->progressWidget->hide();
    fillComboBox();
}
void MainWindow::fillComboBox() {
    ui->searchComboBox->addItem("选择搜索字段");
    ui->searchComboBox->addItem("图片ID");
    ui->searchComboBox->addItem("pixiv作品ID");
    ui->searchComboBox->addItem("pixiv作者ID");
    ui->searchComboBox->addItem("pixiv作者名");
    ui->searchComboBox->addItem("pixiv标题");
    ui->searchComboBox->addItem("推特ID");
    ui->searchComboBox->addItem("推特作者ID");
    ui->searchComboBox->addItem("推特作者名");
    ui->searchComboBox->addItem("推特作者别名");
    ui->searchComboBox->setCurrentIndex(0);

    ui->sortComboBox->addItem("无");
    ui->sortComboBox->addItem("图片ID");
    ui->sortComboBox->addItem("下载日期");
    ui->sortComboBox->addItem("编辑日期");
    ui->sortComboBox->addItem("大小");
    ui->sortComboBox->addItem("文件名");
    ui->sortComboBox->addItem("宽度");
    ui->sortComboBox->addItem("高度");
    ui->sortComboBox->setCurrentIndex(0);

    ui->orderComboBox->addItem("升序");
    ui->orderComboBox->addItem("降序");
    ui->orderComboBox->setCurrentIndex(0);
}
void MainWindow::initWorkerThreads() {
    searchWorkerThread = new QThread(this);
    DatabaseWorker* searchWorker = new DatabaseWorker();
    searchWorker->moveToThread(searchWorkerThread);
    connect(this, &MainWindow::searchPics, searchWorker, &DatabaseWorker::searchPics);
    connect(searchWorker, &DatabaseWorker::searchComplete, this, &MainWindow::handleSearchResults);
    connect(searchWorkerThread, &QThread::finished, searchWorker, &QObject::deleteLater);
    connect(this, &MainWindow::reloadWorkerDatabase, searchWorker, &DatabaseWorker::reloadDatabase);
    searchWorkerThread->start();
}
void MainWindow::connectSignalSlots() {
    // debounce timers
    resolutionTimer = new QTimer(this);
    ratioSortTimer = new QTimer(this);
    tagClickTimer = new QTimer(this);
    tagSearchTimer = new QTimer(this);
    resolutionTimer->setSingleShot(true);
    ratioSortTimer->setSingleShot(true);
    tagClickTimer->setSingleShot(true);
    tagSearchTimer->setSingleShot(true);

    // filter checkboxes
    connect(ui->jpgCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowJPG);
    connect(ui->pngCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowPNG);
    connect(ui->gifCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowGIF);
    connect(ui->webpCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowWEBP);
    connect(ui->unknowRestrictCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowUnknowRestrict);
    connect(ui->allAgeCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowAllAge);
    connect(ui->sensitiveCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowSensitive);
    connect(ui->questionableCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowQuestionable);
    connect(ui->r18CheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowR18);
    connect(ui->r18gCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowR18g);
    connect(ui->unknowAICheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowUnknowAI);
    connect(ui->aiCheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowAI);
    connect(ui->noAICheckBox, &QCheckBox::toggled, this, &MainWindow::updateShowNonAI);

    // resolution filters
    connect(ui->maxHeightEdit, &QLineEdit::textChanged, this, &MainWindow::updateMaxHeight);
    connect(ui->minHeightEdit, &QLineEdit::textChanged, this, &MainWindow::updateMinHeight);
    connect(ui->maxWidthEdit, &QLineEdit::textChanged, this, &MainWindow::updateMaxWidth);
    connect(ui->minWidthEdit, &QLineEdit::textChanged, this, &MainWindow::updateMinWidth);
    connect(ui->maxHeightEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer->start(DEBOUNCE_DELAY); });
    connect(ui->minHeightEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer->start(DEBOUNCE_DELAY); });
    connect(ui->maxWidthEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer->start(DEBOUNCE_DELAY); });
    connect(ui->minWidthEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer->start(DEBOUNCE_DELAY); });
    connect(ui->clearResolutionFilterButton, &QPushButton::clicked, this, &MainWindow::clearResolutionFilters);
    connect(resolutionTimer, &QTimer::timeout, this, &MainWindow::handleResolutionTimerTimeout);

    // sorting controls
    connect(ui->sortComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortBy);
    connect(ui->orderComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSortOrder);
    connect(ui->enableRatioCheckBox, &QCheckBox::toggled, this, &MainWindow::updateEnableRatioSort);
    connect(ui->ratioSlider, &QSlider::valueChanged, this, &MainWindow::updateRatioSlider);
    connect(ui->ratioSlider, &QSlider::valueChanged, this, [this]() { ratioSortTimer->start(SLIDER_DEBOUNCE_DELAY); });
    connect(ui->widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(
        ui->widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() { ratioSortTimer->start(SLIDER_DEBOUNCE_DELAY); });
    connect(ui->heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &MainWindow::updateRatioSpinBox);
    connect(
        ui->heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() { ratioSortTimer->start(SLIDER_DEBOUNCE_DELAY); });
    connect(ratioSortTimer, &QTimer::timeout, this, &MainWindow::handleRatioTimerTimeout);

    // text search controls
    connect(ui->searchComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSearchField);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchText);
    connect(ui->searchLineEdit, &QLineEdit::returnPressed, this, &MainWindow::picSearch);

    // tag list controls
    connect(ui->generalTagList, &QListWidget::itemClicked, this, &MainWindow::handleListWidgetItemSingleClick);
    connect(ui->generalTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->characterTagList, &QListWidget::itemClicked, this, &MainWindow::handleListWidgetItemSingleClick);
    connect(ui->characterTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->pixivTagList, &QListWidget::itemClicked, this, &MainWindow::handleListWidgetItemSingleClick);
    connect(ui->pixivTagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(ui->twitterHashtagList, &QListWidget::itemClicked, this, &MainWindow::handleListWidgetItemSingleClick);
    connect(ui->twitterHashtagList, &QListWidget::itemDoubleClicked, this, &MainWindow::addExcludedTags);
    connect(tagClickTimer, &QTimer::timeout, this, &MainWindow::addIncludedTags);
    connect(tagSearchTimer, &QTimer::timeout, this, &MainWindow::picSearch);

    // tag search box
    connect(ui->searchTagTextEdit, &QLineEdit::textChanged, this, &MainWindow::tagSearch);

    // scroll area scrollbar
    connect(
        ui->picBrowseScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::handlescrollBarValueChange);

    // menu actions
    connect(ui->addNewPicsAction, &QAction::triggered, this, &MainWindow::handleAddNewPicsAction);
    connect(ui->addPowerfulPixivDownloaderAction, &QAction::triggered, this, &MainWindow::handleAddPowerfulPixivDownloaderAction);
    connect(ui->addGallery_dlTwitterAction, &QAction::triggered, this, &MainWindow::handleAddGallery_dlTwitterAction);
}
QString getTagString(std::string tag, int count) {
    return QString("%1 (%2)").arg(QString::fromStdString(tag)).arg(count);
}
void MainWindow::loadTags() {
    allTags = database.getTags();
    allPixivTags = database.getPixivTags();
    allTwitterHashtags = database.getTwitterHashtags();
    Info() << "Loaded tags from database.";
}
void MainWindow::displayTags(const std::vector<std::tuple<std::string, int, bool>>& availableTags,
                             const std::vector<std::pair<std::string, int>>& availablePixivTags,
                             const std::vector<std::pair<std::string, int>>& availableTwitterHashtags) {
    ui->generalTagList->clear();
    ui->characterTagList->clear();
    ui->pixivTagList->clear();
    ui->twitterHashtagList->clear();

    std::vector<std::tuple<std::string, int, bool>> tags;
    std::vector<std::pair<std::string, int>> pixivTags;
    std::vector<std::pair<std::string, int>> twitterHashtags;

    if (availableTags.size() > 0) {
        tags = availableTags;
    } else {
        tags = allTags;
    }
    if (availablePixivTags.size() > 0) {
        pixivTags = availablePixivTags;
    } else {
        pixivTags = allPixivTags;
    }
    if (availableTwitterHashtags.size() > 0) {
        twitterHashtags = availableTwitterHashtags;
    } else {
        twitterHashtags = allTwitterHashtags;
    }

    QStringList generalTagNames;
    QStringList characterTagNames;
    for (const auto& tag : tags) {
        if (std::get<2>(tag)) { // determine if it's a character tag
            characterTagNames.append(getTagString(std::get<0>(tag), std::get<1>(tag)));
        } else {
            generalTagNames.append(getTagString(std::get<0>(tag), std::get<1>(tag)));
        }
    }
    ui->generalTagList->addItems(generalTagNames);
    ui->characterTagList->addItems(characterTagNames);
    size_t generalTagIndex = 0;
    size_t characterTagIndex = 0;
    for (const auto& tag : tags) {
        if (std::get<2>(tag)) { // determine if it's a character tag
            ui->characterTagList->item(characterTagIndex)->setData(Qt::UserRole, QString::fromStdString(std::get<0>(tag)));
            characterTagIndex++;
        } else {
            ui->generalTagList->item(generalTagIndex)->setData(Qt::UserRole, QString::fromStdString(std::get<0>(tag)));
            generalTagIndex++;
        }
    }

    QStringList pixivTagNames;
    for (const auto& tag : pixivTags) {
        pixivTagNames.append(getTagString(tag.first, tag.second));
    }
    ui->pixivTagList->addItems(pixivTagNames);
    for (size_t i = 0; i < pixivTags.size(); i++) {
        ui->pixivTagList->item(i)->setData(Qt::UserRole, QString::fromStdString(pixivTags[i].first));
    }

    QStringList twitterHashtagNames;
    for (const auto& tag : twitterHashtags) {
        twitterHashtagNames.append(getTagString(tag.first, tag.second));
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
void MainWindow::updateShowSensitive(bool checked) {
    showSensitive = checked;
    refreshPicDisplay();
}
void MainWindow::updateShowQuestionable(bool checked) {
    showQuestionable = checked;
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
    if (ok) {
        maxWidth = value;
    } else {
        maxWidth = std::numeric_limits<uint>::max();
    }
}
void MainWindow::updateMinWidth(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        minWidth = value;
    } else {
        minWidth = 0;
    }
}
void MainWindow::updateMaxHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        maxHeight = value;
    } else {
        maxHeight = std::numeric_limits<uint>::max();
    }
}
void MainWindow::updateMinHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        minHeight = value;
    } else {
        minHeight = 0;
    }
}
void MainWindow::clearResolutionFilters() {
    ui->maxWidthEdit->clear();
    ui->minWidthEdit->clear();
    ui->maxHeightEdit->clear();
    ui->minHeightEdit->clear();
    maxWidth = std::numeric_limits<uint>::max();
    minWidth = 0;
    maxHeight = std::numeric_limits<uint>::max();
    minHeight = 0;
    refreshPicDisplay();
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
        return static_cast<int>(((1 / (ratio)) - 1) * 20);
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
}
void MainWindow::handleRatioTimerTimeout() {
    if (ratioSortEnabled) {
        sortPics();
        refreshPicDisplay();
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
                return a.size < b.size;
            } else {
                return a.size > b.size;
            }
        case SortBy::DownloadDate:
            if (sortOrder == SortOrder::Ascending) {
                return a.downloadTime < b.downloadTime;
            } else {
                return a.downloadTime > b.downloadTime;
            }
        case SortBy::EditDate:
            if (sortOrder == SortOrder::Ascending) {
                return a.editTime < b.editTime;
            } else {
                return a.editTime > b.editTime;
            }
        case SortBy::Filename:
            if (sortOrder == SortOrder::Ascending) {
                return a.filePaths.begin()->filename().string() < b.filePaths.begin()->filename().string();
            } else {
                return a.filePaths.begin()->filename().string() > b.filePaths.begin()->filename().string();
            }
        case SortBy::Width:
            if (sortOrder == SortOrder::Ascending) {
                return a.width < b.width;
            } else {
                return a.width > b.width;
            }
        case SortBy::Height:
            if (sortOrder == SortOrder::Ascending) {
                return a.height < b.height;
            } else {
                return a.height > b.height;
            }
        default:
            return false;
        }
    };
    std::sort(resultPics.begin(), resultPics.end(), comparator);
}
void MainWindow::updateSearchField(int index) {
    searchField = static_cast<SearchField>(index);
}
void MainWindow::updateSearchText(const QString& text) {
    searchText = text.toStdString();
    searchTextChanged = true;
}
void MainWindow::handleListWidgetItemSingleClick(QListWidgetItem* item) {
    tagClickTimer->start(DOUBLE_CLICK_DELAY);
    lastClickedTagItem = item;
}
void MainWindow::addIncludedTags() {
    if (tagDoubleClicked) return;

    QListWidgetItem* item = lastClickedTagItem;
    ui->selectedTagScrollArea->show();

    QListWidget* listWidget = item->listWidget();
    std::string tag = item->data(Qt::UserRole).toString().toStdString();

    QPushButton* tagButton = new QPushButton(this);
    QPalette palette = tagButton->palette();
    palette.setColor(QPalette::Button, QColor(100, 200, 100));
    tagButton->setPalette(palette);
    tagButton->setProperty("tag", QString::fromStdString(tag));
    if (listWidget == ui->generalTagList || listWidget == ui->characterTagList) {
        if (includedTags.find(tag) == includedTags.end()) {
            includedTags.insert(tag);
            selectedTagChanged = true;
            tagButton->setText(QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeIncludedTags(tagButton); });
        }
    } else if (listWidget == ui->pixivTagList) {
        if (includedPixivTags.find(tag) == includedPixivTags.end()) {
            includedPixivTags.insert(tag);
            selectedPixivTagChanged = true;
            tagButton->setText(QString("pixiv: ") + QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeIncludedPixivTags(tagButton); });
        }
    } else if (listWidget == ui->twitterHashtagList) {
        if (includedTweetTags.find(tag) == includedTweetTags.end()) {
            includedTweetTags.insert(tag);
            selectedTweetTagChanged = true;
            tagButton->setText(QString("twitter: ") + QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeIncludedTweetTags(tagButton); });
        }
    }
    ui->selectedTagLayout->insertWidget(0, tagButton);
    picSearch();
}
void MainWindow::addExcludedTags(QListWidgetItem* item) {
    tagDoubleClicked = true;
    tagClickTimer->stop();

    ui->selectedTagScrollArea->show();

    QListWidget* listWidget = item->listWidget();
    std::string tag = item->data(Qt::UserRole).toString().toStdString();

    QPushButton* tagButton = new QPushButton(this);
    QPalette palette = tagButton->palette();
    palette.setColor(QPalette::Button, QColor(200, 100, 100));
    tagButton->setPalette(palette);
    tagButton->setProperty("tag", QString::fromStdString(tag));
    if (listWidget == ui->generalTagList || listWidget == ui->characterTagList) {
        if (excludedTags.find(tag) == excludedTags.end()) {
            excludedTags.insert(tag);
            selectedTagChanged = true;
            tagButton->setText(QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeExcludedTags(tagButton); });
        }
    } else if (listWidget == ui->pixivTagList) {
        if (excludedPixivTags.find(tag) == excludedPixivTags.end()) {
            excludedPixivTags.insert(tag);
            selectedPixivTagChanged = true;
            tagButton->setText(QString("pixiv: ") + QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeExcludedPixivTags(tagButton); });
        }
    } else if (listWidget == ui->twitterHashtagList) {
        if (excludedTweetTags.find(tag) == excludedTweetTags.end()) {
            excludedTweetTags.insert(tag);
            selectedTweetTagChanged = true;
            tagButton->setText(QString("twitter: ") + QString::fromStdString(tag));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeExcludedTweetTags(tagButton); });
        }
    }
    ui->selectedTagLayout->addWidget(tagButton);
    picSearch();
    tagDoubleClicked = false;
}
void MainWindow::removeIncludedTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    includedTags.erase(tag);
    selectedTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::removeExcludedTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    excludedTags.erase(tag);
    selectedTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::removeIncludedPixivTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    includedPixivTags.erase(tag);
    selectedPixivTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::removeExcludedPixivTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    excludedPixivTags.erase(tag);
    selectedPixivTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::removeIncludedTweetTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    includedTweetTags.erase(tag);
    selectedTweetTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::removeExcludedTweetTags(QPushButton* button) {
    std::string tag = button->property("tag").toString().toStdString();
    excludedTweetTags.erase(tag);
    selectedTweetTagChanged = true;
    ui->selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui->selectedTagScrollArea->hide();
    }
    tagSearchTimer->start(DEBOUNCE_DELAY);
}
void MainWindow::tagSearch(const QString& text) {
    QListWidget* currentListWidget = ui->tagListTabWidget->currentWidget()->findChild<QListWidget*>();
    for (int i = 0; i < currentListWidget->count(); i++) {
        QListWidgetItem* item = currentListWidget->item(i);
        if (item->text().contains(text, Qt::CaseInsensitive)) {
            item->setHidden(false);
        } else {
            item->setHidden(true);
        }
    }
}
void MainWindow::handleWindowSizeChange() {
    int width = ui->picBrowseWidget->width();
    widgetsPerRow = width / 270;
    if (widgetsPerRow < 1) widgetsPerRow = 1;
    while (ui->picDisplayLayout->count() > 0) {
        QLayoutItem* item = ui->picDisplayLayout->takeAt(0);
        if (item) {
            ui->picDisplayLayout->removeItem(item);
        }
        delete item;
    }
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
void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (firstShow_) {
        firstShow_ = false;
        handleWindowSizeChange();
        picSearch(); // display no metadata pics initially
    }
}
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    handleWindowSizeChange();
}
void MainWindow::handlescrollBarValueChange(int value) {
    if (ui->picBrowseScrollArea->verticalScrollBar()->maximum() - value < 100) {
        loadMorePics();
    }
}
void MainWindow::picSearch() {
    imageLoadThreadPool.clearTasks();
    clearAllPicFrames();
    if (isSearchCriteriaEmpty()) {
        displayTags();
        resultPics = noMetadataPics;
        sortPics();
        loadMorePics();
        ui->statusbar->showMessage("显示无元数据图片，共 " + QString::number(resultPics.size()) + " 张图片");
        return;
    }
    ui->statusbar->showMessage("正在搜索...");
    searchRequestId++;
    emit searchPics(includedTags,
                    excludedTags,
                    includedPixivTags,
                    excludedPixivTags,
                    includedTweetTags,
                    excludedTweetTags,
                    searchText,
                    searchField,
                    selectedTagChanged,
                    selectedPixivTagChanged,
                    selectedTweetTagChanged,
                    searchTextChanged,
                    searchRequestId);
}
void MainWindow::handleSearchResults(const std::vector<PicInfo>& pics,
                                     std::vector<std::tuple<std::string, int, bool>> availableTags,
                                     std::vector<std::pair<std::string, int>> availablePixivTags,
                                     std::vector<std::pair<std::string, int>> availableTweetTags,
                                     size_t requestId) {
    if (requestId != searchRequestId) return;
    resultPics = std::move(pics);
    selectedTagChanged = false;
    selectedPixivTagChanged = false;
    selectedTweetTagChanged = false;
    searchTextChanged = false;
    displayTags(availableTags, availablePixivTags, availableTweetTags);
    ui->statusbar->showMessage("搜索完成，找到 " + QString::number(resultPics.size()) + " 张图片");
    sortPics();
    loadMorePics();
}
void MainWindow::refreshPicDisplay() {
    removePicFramesFromLayout();
    loadMorePics();
}
bool MainWindow::isMatchFilter(const PicInfo& pic) {
    if (!showJPG && pic.fileType == ImageFormat::JPG) return false;
    if (!showPNG && pic.fileType == ImageFormat::PNG) return false;
    if (!showGIF && pic.fileType == ImageFormat::GIF) return false;
    if (!showWEBP && pic.fileType == ImageFormat::WebP) return false;
    if (!showUnknowRestrict && pic.xRestrict == RestrictType::Unknown) return false;
    if (!showAllAge && pic.xRestrict == RestrictType::AllAges) return false;
    if (!showSensitive && pic.xRestrict == RestrictType::Sensitive) return false;
    if (!showQuestionable && pic.xRestrict == RestrictType::Questionable) return false;
    if (!showR18 && pic.xRestrict == RestrictType::R18) return false;
    if (!showR18g && pic.xRestrict == RestrictType::R18G) return false;
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

        PictureFrame* picFrame = nullptr;
        auto frameIt = idToFrameMap.find(pic.id);
        if (frameIt != idToFrameMap.end()) {
            picFrame = frameIt->second;
            ui->picDisplayLayout->addWidget(picFrame, currentRow, currentColumn);
        } else {
            if (searchText.empty() || searchField == SearchField::None) { // create PictureFrame
                picFrame = new PictureFrame(this, pic);
            } else {
                picFrame = new PictureFrame(this, pic, searchField);
            }
            idToFrameMap[pic.id] = picFrame;

            if (imageThumbCache.find(pic.id) == imageThumbCache.end()) { // load image thumbnail
                imageLoadThreadPool.loadImage(pic);
            } else {
                picFrame->setPixmap(imageThumbCache[pic.id]);
            }

            ui->picDisplayLayout->addWidget(picFrame, currentRow, currentColumn); // add to layout and display
        }

        picsLoaded++;
        currentColumn++;
        if (currentColumn >= widgetsPerRow) {
            currentColumn = 0;
            currentRow++;
        }
    }
}
bool MainWindow::event(QEvent* event) {
    if (event->type() == ImageLoadCompleteEvent::EventType) {
        auto* imageEvent = static_cast<ImageLoadCompleteEvent*>(event);
        displayImage(imageEvent->id, std::move(imageEvent->img));
        return true;
    }
    if (event->type() == ImportProgressReportEvent::EventType) {
        auto* importProgressEvent = static_cast<ImportProgressReportEvent*>(event);
        displayImportProgress(importProgressEvent->progress, importProgressEvent->total);
        return true;
    }
    return QMainWindow::event(event);
}
void MainWindow::displayImage(uint64_t picId, QImage&& img) {
    size_t cacheLimit = MAX_PIC_CACHE;
    if (resultPics.size() > MAX_PIC_CACHE) {
        cacheLimit = resultPics.size();
    }
    if (imageThumbCache.size() >= cacheLimit) {
        for (auto iter = imageThumbCache.begin(); iter != imageThumbCache.end();) {
            if (idToFrameMap.find(iter->first) == idToFrameMap.end()) {
                iter = imageThumbCache.erase(iter);
            } else {
                ++iter;
            }
        }
    }
    QPixmap pixmap = QPixmap::fromImage(std::move(img));
    imageThumbCache[picId] = pixmap;
    auto frameIt = idToFrameMap.find(picId);
    if (frameIt != idToFrameMap.end()) {
        frameIt->second->setPixmap(pixmap);
    }
}
void MainWindow::displayImportProgress(size_t progress, size_t total) {
    if (progress >= total) { // import complete
        delete importer;
        importer = nullptr;
        ui->progressWidget->hide();
        ui->progressLabel->setText("");
        ui->progressStatusLabel->setText("");
        ui->statusbar->showMessage(
            "图片导入完成，共扫描 " + QString::number(total) + " 文件，用时 " +
            QString::number(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - ImportStartTime).count()) +
            " 秒");
        database.reloadDatabase();
        emit reloadWorkerDatabase();
        loadTags(); // load new tags from database
        if (isSearchCriteriaEmpty()) {
            noMetadataPics = database.getNoMetadataPics();
            picSearch();
        }
        Info() << "Import completed. Total files imported: " << total;
        return;
    }
    // update progress bar and status
    ui->progressBar->setMaximum(static_cast<int>(total));
    ui->progressBar->setValue(static_cast<int>(progress));

    auto currentTime = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - ImportStartTime).count();

    if (timeDiff > 0 && progress > 0) {
        double timeDiffSeconds = static_cast<double>(timeDiff) / 1000.0;
        double speed = static_cast<double>(progress) / timeDiffSeconds; // items per second
        size_t etaSeconds = static_cast<size_t>((total - progress) / speed);
        ui->progressStatusLabel->setText(QString("%1 / %2 | 速度：%3 文件每秒 | 剩余时间：%4 秒")
                                             .arg(progress)
                                             .arg(total)
                                             .arg(QString::number(speed, 'f', 2))
                                             .arg(etaSeconds));
    }
}
void MainWindow::clearAllPicFrames() { // clear all PictureFrames and free memory
    while (ui->picDisplayLayout->count() > 0) {
        QLayoutItem* item = ui->picDisplayLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    for (auto& [id, frame] : idToFrameMap) {
        delete frame;
    }
    idToFrameMap.clear();

    displayIndex = 0;
    displayingPicIds.clear();
    currentColumn = 0;
    currentRow = 0;
    ui->picBrowseScrollArea->verticalScrollBar()->setValue(0);
}
void MainWindow::removePicFramesFromLayout() { // remove all PictureFrames from layout but keep them in memory
    while (ui->picDisplayLayout->count() > 0) {
        QLayoutItem* item = ui->picDisplayLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
        delete item;
    }
    displayIndex = 0;
    displayingPicIds.clear();
    currentColumn = 0;
    currentRow = 0;
    ui->picBrowseScrollArea->verticalScrollBar()->setValue(0);
}
void MainWindow::handleAddNewPicsAction() {
    if (importer) {
        ui->statusbar->showMessage("已有导入任务正在进行中，请稍后再试。");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "选择图片文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui->statusbar->showMessage("正在扫描文件夹...");
    importer = new MultiThreadedImporter(std::filesystem::path(dir.toStdString()), reportImportProgress);
    ui->progressWidget->show();
    ui->progressBar->setValue(0);
    ui->progressLabel->setText("正在导入图片...");
    Info() << "Started importing pictures from directory: " << dir.toStdString();
    ImportStartTime = std::chrono::steady_clock::now();
}
void MainWindow::handleAddPowerfulPixivDownloaderAction() {
    if (importer) {
        ui->statusbar->showMessage("已有导入任务正在进行中，请稍后再试。");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择 Powerful Pixiv Downloader 下载文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui->statusbar->showMessage("正在扫描 Powerful Pixiv Downloader 下载文件夹...");
    importer = new MultiThreadedImporter(std::filesystem::path(dir.toStdString()), reportImportProgress, ParserType::Pixiv);
    ui->progressWidget->show();
    ui->progressBar->setValue(0);
    ui->progressLabel->setText("正在导入Pixiv图片...");
    Info() << "Started importing pixiv pictures from directory: " << dir.toStdString();
    ImportStartTime = std::chrono::steady_clock::now();
}
void MainWindow::handleAddGallery_dlTwitterAction() {
    if (importer) {
        ui->statusbar->showMessage("已有导入任务正在进行中，请稍后再试。");
        return;
    }
    QString dir =
        QFileDialog::getExistingDirectory(this, "选择 gallery-dl Twitter 下载文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui->statusbar->showMessage("正在扫描 gallery-dl Twitter 下载文件夹...");
    importer = new MultiThreadedImporter(std::filesystem::path(dir.toStdString()), reportImportProgress, ParserType::Twitter);
    ui->progressWidget->show();
    ui->progressBar->setValue(0);
    ui->progressLabel->setText("正在导入Twitter图片...");
    Info() << "Started importing twitter pictures from directory: " << dir.toStdString();
    ImportStartTime = std::chrono::steady_clock::now();
}
