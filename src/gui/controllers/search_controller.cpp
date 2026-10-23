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

#include "search_controller.h"
#include "support/utils.h"

QString getTagString(const TagCount& tagCount) {
    return QString("%1 (%2)").arg(QString::fromStdString(tagCount.tag.tag)).arg(tagCount.count);
}
QString getTagString(const PlatformTagCount& tagCount) {
    return QString("[%1] %2 (%3)")
        .arg(platformTypeToString(tagCount.tag.platform))
        .arg(QString::fromStdString(tagCount.tag.tag))
        .arg(tagCount.count);
}
void SearchController::initWorkerThreads() {
    searchWorkerThread = new QThread(this);
    DatabaseWorker* searchWorker = new DatabaseWorker();
    searchWorker->moveToThread(searchWorkerThread);
    connect(this, &SearchController::searchPics, searchWorker, &DatabaseWorker::searchPics);
    connect(searchWorker, &DatabaseWorker::searchComplete, this, &SearchController::handleSearchResults);
    connect(searchWorkerThread, &QThread::finished, searchWorker, &QObject::deleteLater);
    searchWorkerThread->start();
}
void SearchController::connectSignalSlots() {
    ui.selectedTagScrollArea->hide();
    tagClickTimer.setSingleShot(true);
    tagSearchTimer.setSingleShot(true);
    // text search controls
    connect(ui.searchPlatformComboBox, &QComboBox::currentIndexChanged, this, &SearchController::updateSearchPlatform);
    connect(ui.searchComboBox, &QComboBox::currentIndexChanged, this, &SearchController::updateSearchField);
    connect(ui.searchLineEdit, &QLineEdit::textChanged, this, &SearchController::updateSearchText);
    connect(ui.searchLineEdit, &QLineEdit::returnPressed, this, &SearchController::picSearch);
    connect(ui.clearTextSearchButton, &QPushButton::clicked, this, &SearchController::clearSearchText);

    // tag list controls
    connect(ui.generalTagList, &QListWidget::itemClicked, this, &SearchController::handleListWidgetItemSingleClick);
    connect(ui.generalTagList, &QListWidget::itemDoubleClicked, this, &SearchController::addExcludedTags);
    connect(ui.characterTagList, &QListWidget::itemClicked, this, &SearchController::handleListWidgetItemSingleClick);
    connect(ui.characterTagList, &QListWidget::itemDoubleClicked, this, &SearchController::addExcludedTags);
    connect(ui.platformTagList, &QListWidget::itemClicked, this, &SearchController::handleListWidgetItemSingleClick);
    connect(ui.platformTagList, &QListWidget::itemDoubleClicked, this, &SearchController::addExcludedTags);
    connect(&tagClickTimer, &QTimer::timeout, this, &SearchController::addIncludedTags);
    connect(&tagSearchTimer, &QTimer::timeout, this, &SearchController::picSearch);

    // tag search box
    connect(ui.searchTagTextEdit, &QLineEdit::textChanged, this, &SearchController::tagSearch);
}
void SearchController::fillComboBoxes() {
    ui.searchPlatformComboBox->addItem("选择平台");
    ui.searchPlatformComboBox->addItem("Pixiv");
    ui.searchPlatformComboBox->addItem("Twitter");
    ui.searchPlatformComboBox->setCurrentIndex(0);

    ui.searchComboBox->addItem("选择搜索字段");
    ui.searchComboBox->addItem("平台ID");
    ui.searchComboBox->addItem("作者ID");
    ui.searchComboBox->addItem("作者名");
    ui.searchComboBox->addItem("作者别名");
    ui.searchComboBox->addItem("标题");
    ui.searchComboBox->setCurrentIndex(0);
}
void SearchController::displayTags(const std::vector<TagCount>& availableTags,
                                   const std::vector<PlatformTagCount>& availablePlatformTags) {
    ui.generalTagList->clear();
    ui.characterTagList->clear();
    ui.platformTagList->clear();

    std::vector<TagCount> tagCounts;
    std::vector<PlatformTagCount> platformTagCounts;

    if (availableTags.empty() && availablePlatformTags.empty()) { // only display all tags if no available tags are provided
        tagCounts = allTags;
        platformTagCounts = allPlatformTags;
    } else { // display available tags when either availableTags or availablePlatformTags is provided
        tagCounts = availableTags;
        platformTagCounts = availablePlatformTags;
    }

    QStringList generalTagNames;
    QStringList characterTagNames;
    for (const auto& tagCount : tagCounts) {
        if (tagCount.tag.isCharacter) { // determine if it's a character tag
            characterTagNames.append(getTagString(tagCount));
        } else {
            generalTagNames.append(getTagString(tagCount));
        }
    }
    ui.generalTagList->addItems(generalTagNames);
    ui.characterTagList->addItems(characterTagNames);
    size_t generalTagIndex = 0;
    size_t characterTagIndex = 0;
    for (const auto& tagCount : tagCounts) {
        if (tagCount.tag.isCharacter) { // determine if it's a character tag
            ui.characterTagList->item(characterTagIndex)->setData(Qt::UserRole, tagCount.tagId);
            characterTagIndex++;
        } else {
            ui.generalTagList->item(generalTagIndex)->setData(Qt::UserRole, tagCount.tagId);
            generalTagIndex++;
        }
    }

    QStringList platformTagNames;
    for (const auto& tagCount : platformTagCounts) {
        platformTagNames.append(getTagString(tagCount));
    }
    ui.platformTagList->addItems(platformTagNames);
    for (size_t i = 0; i < platformTagCounts.size(); i++) {
        ui.platformTagList->item(i)->setData(Qt::UserRole, platformTagCounts[i].tagId);
    }
}
void SearchController::loadTags() {
    allTags = database.getTagCounts();
    allPlatformTags = database.getPlatformTagCounts();
    Info() << "Loaded tags from database.";
}
void SearchController::updateSearchPlatform(int index) {
    searchCtx.searchPlatform = static_cast<PlatformType>(index);
}
void SearchController::updateSearchField(int index) {
    searchCtx.searchField = static_cast<SearchField>(index);
}
void SearchController::updateSearchText(const QString& text) {
    searchCtx.searchText = text.toStdString();
}
void SearchController::clearSearchText() {
    ui.searchLineEdit->clear();
    searchCtx.searchText = "";
    picSearch();
}
void SearchController::handleListWidgetItemSingleClick(QListWidgetItem* item) {
    tagClickTimer.start(DOUBLE_CLICK_DELAY);
    lastClickedTagItem = item;
}
void SearchController::addIncludedTags() {
    if (tagDoubleClicked) return;

    QListWidgetItem* item = lastClickedTagItem;
    ui.selectedTagScrollArea->show();

    QListWidget* listWidget = item->listWidget();
    uint32_t tagId = item->data(Qt::UserRole).toUInt();

    QPushButton* tagButton = new QPushButton(ui.selectedTagWidget);
    QPalette palette = tagButton->palette();
    palette.setColor(QPalette::Button, QColor(100, 200, 100));
    tagButton->setPalette(palette);
    tagButton->setProperty("tag", tagId);
    if (listWidget == ui.generalTagList || listWidget == ui.characterTagList) {
        if (searchCtx.includedTags.find(tagId) == searchCtx.includedTags.end()) {
            searchCtx.includedTags.insert(tagId);
            tagButton->setText(item->text().left(item->text().lastIndexOf(' ')));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeIncludedTags(tagButton); });
        }
    } else if (listWidget == ui.platformTagList) {
        if (searchCtx.includedPlatformTags.find(tagId) == searchCtx.includedPlatformTags.end()) {
            searchCtx.includedPlatformTags.insert(tagId);
            tagButton->setText(item->text().left(item->text().lastIndexOf(' ')));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeIncludedPlatformTags(tagButton); });
        }
    }
    ui.selectedTagLayout->insertWidget(0, tagButton);
    picSearch();
}
void SearchController::addExcludedTags(QListWidgetItem* item) {
    tagDoubleClicked = true;
    tagClickTimer.stop();

    ui.selectedTagScrollArea->show();

    QListWidget* listWidget = item->listWidget();
    uint32_t tagId = item->data(Qt::UserRole).toUInt();

    QPushButton* tagButton = new QPushButton(ui.selectedTagWidget);
    QPalette palette = tagButton->palette();
    palette.setColor(QPalette::Button, QColor(200, 100, 100));
    tagButton->setPalette(palette);
    tagButton->setProperty("tag", tagId);
    if (listWidget == ui.generalTagList || listWidget == ui.characterTagList) {
        if (searchCtx.excludedTags.find(tagId) == searchCtx.excludedTags.end()) {
            searchCtx.excludedTags.insert(tagId);
            tagButton->setText(item->text().left(item->text().lastIndexOf(' ')));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeExcludedTags(tagButton); });
        }
    } else if (listWidget == ui.platformTagList) {
        if (searchCtx.excludedPlatformTags.find(tagId) == searchCtx.excludedPlatformTags.end()) {
            searchCtx.excludedPlatformTags.insert(tagId);
            tagButton->setText(item->text().left(item->text().lastIndexOf(' ')));
            connect(tagButton, &QPushButton::clicked, this, [this, tagButton]() { removeExcludedPlatformTags(tagButton); });
        }
    }
    ui.selectedTagLayout->addWidget(tagButton);
    picSearch();
    tagDoubleClicked = false;
}
void SearchController::removeIncludedTags(QPushButton* button) {
    uint32_t tag = button->property("tag").toUInt();
    searchCtx.includedTags.erase(tag);
    ui.selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui.selectedTagScrollArea->hide();
    }
    tagSearchTimer.start(DEBOUNCE_DELAY);
}
void SearchController::removeExcludedTags(QPushButton* button) {
    uint32_t tag = button->property("tag").toUInt();
    searchCtx.excludedTags.erase(tag);
    ui.selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui.selectedTagScrollArea->hide();
    }
    tagSearchTimer.start(DEBOUNCE_DELAY);
}
void SearchController::removeIncludedPlatformTags(QPushButton* button) {
    uint32_t tag = button->property("tag").toUInt();
    searchCtx.includedPlatformTags.erase(tag);
    ui.selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui.selectedTagScrollArea->hide();
    }
    tagSearchTimer.start(DEBOUNCE_DELAY);
}
void SearchController::removeExcludedPlatformTags(QPushButton* button) {
    uint32_t tag = button->property("tag").toUInt();
    searchCtx.includedPlatformTags.erase(tag);
    ui.selectedTagLayout->removeWidget(button);
    button->deleteLater();
    if (isSelectedTagsEmpty()) {
        ui.selectedTagScrollArea->hide();
    }
    tagSearchTimer.start(DEBOUNCE_DELAY);
}
void SearchController::tagSearch(const QString& text) {
    QListWidget* currentListWidget = ui.tagListTabWidget->currentWidget()->findChild<QListWidget*>();
    for (int i = 0; i < currentListWidget->count(); i++) {
        QListWidgetItem* item = currentListWidget->item(i);
        if (item->text().contains(text, Qt::CaseInsensitive)) {
            item->setHidden(false);
        } else {
            item->setHidden(true);
        }
    }
}
// Main search function
void SearchController::picSearch() {
    if (isSearchCriteriaEmpty()) {
        displayTags();
        return;
    }
    ui.statusbar->showMessage("正在搜索...");
    searchRequestId++;
    emit searchPics(searchCtx, searchRequestId);
}
void SearchController::handleSearchResults(DisplayItems* displayItems,
                                           const std::vector<TagCount>& availableTags,
                                           const std::vector<PlatformTagCount>& availablePlatformTags,
                                           size_t requestId) {
    if (requestId != searchRequestId) return;
    displayController.setDisplayItems(displayItems, searchCtx.searchField);
    displayController.sortDisplayItems(filterSortController.getSortContext());
    displayTags(availableTags, availablePlatformTags);
    ui.statusbar->showMessage("搜索完成，共找到 " +
                              QString::number(displayItems->type == DisplayItemType::Pic ? displayItems->picItems.size()
                                                                                         : displayItems->metadataItems.size()) +
                              " 个结果");
}
