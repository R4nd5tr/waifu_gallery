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
#include "support/context.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTimer>

struct FilterSortUi {
    // filter checkboxes
    QCheckBox* unknowPlatformCheckBox;
    QCheckBox* pixivPlatformCheckBox;
    QCheckBox* twitterPlatformCheckBox;
    QCheckBox* jpgCheckBox;
    QCheckBox* pngCheckBox;
    QCheckBox* gifCheckBox;
    QCheckBox* webpCheckBox;
    QCheckBox* unknowRestrictCheckBox;
    QCheckBox* allAgeCheckBox;
    QCheckBox* sensitiveCheckBox;
    QCheckBox* questionableCheckBox;
    QCheckBox* r18CheckBox;
    QCheckBox* r18gCheckBox;
    QCheckBox* unknowAICheckBox;
    QCheckBox* aiCheckBox;
    QCheckBox* noAICheckBox;

    // resolution filter widgets
    QLineEdit* maxWidthEdit;
    QLineEdit* maxHeightEdit;
    QLineEdit* minWidthEdit;
    QLineEdit* minHeightEdit;
    QPushButton* clearResolutionFilterButton;

    // sort widgets
    QCheckBox* enableRatioCheckBox;
    QComboBox* sortComboBox;
    QComboBox* orderComboBox;
    QSlider* ratioSlider;
    QDoubleSpinBox* widthRatioSpinBox;
    QDoubleSpinBox* heightRatioSpinBox;
};

class FilterSortController : public QObject {
    Q_OBJECT
public:
    FilterSortController(DisplayController& displayController) : displayController(displayController) {}
    ~FilterSortController() = default;
    void setupUi(FilterSortUi filterSortUi) {
        ui = filterSortUi;
        connectSignalSlots();
        fillComboBoxes();
    }

    SortContext getSortContext() const { return sortCtx; }

private:
    FilterSortUi ui;
    DisplayController& displayController;
    FilterContext filterCtx;
    void connectSignalSlots();
    void fillComboBoxes();
    void updateShowUnknowPlatform(bool checked);
    void updateShowPixiv(bool checked);
    void updateShowTwitter(bool checked);
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
    QTimer resolutionTimer;
    void handleResolutionTimerTimeout();

    SortContext sortCtx;
    bool ratioSortEnabled = false;
    bool ratioSliderEditing = false;
    bool ratioSpinBoxEditing = false;
    int ratioSliderValue;
    double widthRatioSpinBoxValue;
    double heightRatioSpinBoxValue;
    void updateSortBy(int index);
    void updateSortOrder(int index);
    void updateEnableRatioSort(bool checked);
    void updateRatioSlider(int value);
    void updateRatioSpinBox(double value);
    QTimer ratioSortTimer;
    void handleRatioTimerTimeout();
};