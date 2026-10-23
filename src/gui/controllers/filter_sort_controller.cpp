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

#include "filter_sort_controller.h"

void FilterSortController::connectSignalSlots() {
    resolutionTimer.setSingleShot(true);
    ratioSortTimer.setSingleShot(true);
    // filter checkboxes
    connect(ui.unknowPlatformCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowUnknowPlatform);
    connect(ui.pixivPlatformCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowPixiv);
    connect(ui.twitterPlatformCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowTwitter);
    connect(ui.jpgCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowJPG);
    connect(ui.pngCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowPNG);
    connect(ui.gifCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowGIF);
    connect(ui.webpCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowWEBP);
    connect(ui.unknowRestrictCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowUnknowRestrict);
    connect(ui.allAgeCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowAllAge);
    connect(ui.sensitiveCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowSensitive);
    connect(ui.questionableCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowQuestionable);
    connect(ui.r18CheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowR18);
    connect(ui.r18gCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowR18g);
    connect(ui.unknowAICheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowUnknowAI);
    connect(ui.aiCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowAI);
    connect(ui.noAICheckBox, &QCheckBox::toggled, this, &FilterSortController::updateShowNonAI);

    // resolution filters
    connect(ui.maxHeightEdit, &QLineEdit::textChanged, this, &FilterSortController::updateMaxHeight);
    connect(ui.minHeightEdit, &QLineEdit::textChanged, this, &FilterSortController::updateMinHeight);
    connect(ui.maxWidthEdit, &QLineEdit::textChanged, this, &FilterSortController::updateMaxWidth);
    connect(ui.minWidthEdit, &QLineEdit::textChanged, this, &FilterSortController::updateMinWidth);
    connect(ui.maxHeightEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer.start(DEBOUNCE_DELAY); });
    connect(ui.minHeightEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer.start(DEBOUNCE_DELAY); });
    connect(ui.maxWidthEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer.start(DEBOUNCE_DELAY); });
    connect(ui.minWidthEdit, &QLineEdit::textChanged, this, [this]() { resolutionTimer.start(DEBOUNCE_DELAY); });
    connect(ui.clearResolutionFilterButton, &QPushButton::clicked, this, &FilterSortController::clearResolutionFilters);
    connect(&resolutionTimer, &QTimer::timeout, this, &FilterSortController::handleResolutionTimerTimeout);

    // sorting controls
    connect(ui.sortComboBox, &QComboBox::currentIndexChanged, this, &FilterSortController::updateSortBy);
    connect(ui.orderComboBox, &QComboBox::currentIndexChanged, this, &FilterSortController::updateSortOrder);
    connect(ui.enableRatioCheckBox, &QCheckBox::toggled, this, &FilterSortController::updateEnableRatioSort);
    connect(ui.ratioSlider, &QSlider::valueChanged, this, &FilterSortController::updateRatioSlider);
    connect(ui.ratioSlider, &QSlider::valueChanged, this, [this]() { ratioSortTimer.start(SLIDER_DEBOUNCE_DELAY); });
    connect(ui.widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &FilterSortController::updateRatioSpinBox);
    connect(ui.widthRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() { ratioSortTimer.start(SLIDER_DEBOUNCE_DELAY); });
    connect(ui.heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, &FilterSortController::updateRatioSpinBox);
    connect(
        ui.heightRatioSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() { ratioSortTimer.start(SLIDER_DEBOUNCE_DELAY); });
    connect(&ratioSortTimer, &QTimer::timeout, this, &FilterSortController::handleRatioTimerTimeout);
}
void FilterSortController::fillComboBoxes() {
    ui.sortComboBox->addItem("无");
    ui.sortComboBox->addItem("图片ID");
    ui.sortComboBox->addItem("下载日期");
    ui.sortComboBox->addItem("编辑日期");
    ui.sortComboBox->addItem("大小");
    ui.sortComboBox->addItem("文件名");
    ui.sortComboBox->addItem("宽度");
    ui.sortComboBox->addItem("高度");
    ui.sortComboBox->setCurrentIndex(0);

    ui.orderComboBox->addItem("升序");
    ui.orderComboBox->addItem("降序");
    ui.orderComboBox->setCurrentIndex(0);
}
void FilterSortController::updateShowUnknowPlatform(bool checked) {
    filterCtx.showUnknowPlatform = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowPixiv(bool checked) {
    filterCtx.showPixiv = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowTwitter(bool checked) {
    filterCtx.showTwitter = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowJPG(bool checked) {
    filterCtx.showJPG = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowPNG(bool checked) {
    filterCtx.showPNG = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowGIF(bool checked) {
    filterCtx.showGIF = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowWEBP(bool checked) {
    filterCtx.showWEBP = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowUnknowRestrict(bool checked) {
    filterCtx.showUnknowRestrict = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowAllAge(bool checked) {
    filterCtx.showAllAge = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowSensitive(bool checked) {
    filterCtx.showSensitive = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowQuestionable(bool checked) {
    filterCtx.showQuestionable = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowR18(bool checked) {
    filterCtx.showR18 = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowR18g(bool checked) {
    filterCtx.showR18G = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowUnknowAI(bool checked) {
    filterCtx.showUnknowAI = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowAI(bool checked) {
    filterCtx.showAI = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateShowNonAI(bool checked) {
    filterCtx.showNonAI = checked;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateMaxWidth(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        filterCtx.maxWidth = value;
    } else {
        filterCtx.maxWidth = std::numeric_limits<uint>::max();
    }
}
void FilterSortController::updateMinWidth(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        filterCtx.minWidth = value;
    } else {
        filterCtx.minWidth = 0;
    }
}
void FilterSortController::updateMaxHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        filterCtx.maxHeight = value;
    } else {
        filterCtx.maxHeight = std::numeric_limits<uint>::max();
    }
}
void FilterSortController::updateMinHeight(const QString& text) {
    bool ok;
    uint value = text.toUInt(&ok);
    if (ok) {
        filterCtx.minHeight = value;
    } else {
        filterCtx.minHeight = 0;
    }
}
void FilterSortController::clearResolutionFilters() {
    ui.maxWidthEdit->clear();
    ui.minWidthEdit->clear();
    ui.maxHeightEdit->clear();
    ui.minHeightEdit->clear();
    filterCtx.maxWidth = std::numeric_limits<uint>::max();
    filterCtx.minWidth = 0;
    filterCtx.maxHeight = std::numeric_limits<uint>::max();
    filterCtx.minHeight = 0;
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::handleResolutionTimerTimeout() {
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateSortBy(int index) {
    sortCtx.sortBy = static_cast<SortBy>(index);
    displayController.sortDisplayItems(sortCtx);
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateSortOrder(int index) {
    sortCtx.sortOrder = static_cast<SortOrder>(index);
    displayController.sortDisplayItems(sortCtx);
    displayController.setFilterContext(filterCtx);
}
void FilterSortController::updateEnableRatioSort(bool checked) {
    ratioSortEnabled = checked;
    if (ratioSortEnabled) {
        ui.sortComboBox->setDisabled(true);
        ui.orderComboBox->setDisabled(true);
        sortCtx.sortBy = SortBy::Ratio;
    } else {
        ui.sortComboBox->setEnabled(true);
        ui.orderComboBox->setEnabled(true);
        sortCtx.sortBy = static_cast<SortBy>(ui.sortComboBox->currentIndex());
    }
    displayController.sortDisplayItems(sortCtx);
    displayController.setFilterContext(filterCtx);
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
void FilterSortController::updateRatioSlider(int value) {
    if (ratioSpinBoxEditing) return;
    ratioSliderEditing = true;
    ratioSliderValue = value;
    if (value >= 0) {
        double temp = 1.0 + (value / 20.0);
        sortCtx.ratio = 1.0 / temp;
        widthRatioSpinBoxValue = 1.0;
        heightRatioSpinBoxValue = temp;
    } else {
        double temp = 1.0 - (value / 20.0);
        sortCtx.ratio = temp;
        widthRatioSpinBoxValue = temp;
        heightRatioSpinBoxValue = 1.0;
    }
    ui.widthRatioSpinBox->setValue(widthRatioSpinBoxValue);
    ui.heightRatioSpinBox->setValue(heightRatioSpinBoxValue);
    ratioSliderEditing = false;
}
void FilterSortController::updateRatioSpinBox(double value) {
    if (ratioSliderEditing) return;
    ratioSpinBoxEditing = true;
    widthRatioSpinBoxValue = ui.widthRatioSpinBox->value();
    heightRatioSpinBoxValue = ui.heightRatioSpinBox->value();
    if (widthRatioSpinBoxValue == 0) {
        sortCtx.ratio = 0;
    } else if (heightRatioSpinBoxValue == 0) {
        sortCtx.ratio = std::numeric_limits<double>::infinity();
    } else {
        sortCtx.ratio = widthRatioSpinBoxValue / heightRatioSpinBoxValue;
    }
    ui.ratioSlider->setValue(ratioToSliderValue(sortCtx.ratio));
    ratioSpinBoxEditing = false;
}
void FilterSortController::handleRatioTimerTimeout() {
    if (ratioSortEnabled) {
        displayController.sortDisplayItems(sortCtx);
        displayController.setFilterContext(filterCtx);
    }
}
