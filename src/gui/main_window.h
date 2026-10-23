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
#include "controllers/display_controller.h"
#include "controllers/filter_sort_controller.h"
#include "controllers/search_controller.h"
#include "controllers/support/image_loader.h"
#include "controllers/task_controller.h"
#include "service/database.h"
#include "service/importer.h"
#include "service/model.h"
#include "service/tagger.h"
#include "settings_dialog.h"
#include <QCoreApplication>
#include <QMainWindow>
#include <memory>

namespace Ui {
class MainWindow;
}

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
class ImageLoadCompleteEvent : public QEvent {
public:
    static const QEvent::Type EventType;
    ImageLoadCompleteEvent(ImageLoadResult result) : QEvent(EventType), result(result) {}
    ImageLoadResult result;
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
    ImageLoadCompleteCallback imageLoadComplete = [this](const ImageLoadResult& result) {
        QCoreApplication::postEvent(this, new ImageLoadCompleteEvent(result));
    };

signals:
    void searchPics(const SearchContext& ctx, size_t requestId);

protected:
    bool firstShow_ = true;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    Ui::MainWindow* ui;
    PicDatabase database;
    ImageLoader imageLoader{imageLoadComplete}; // Blazing fast!!!
    Importer importer{reportImportProgress};    // Blazing fast!!!
    Tagger tagger{reportTaggingProgress};
    DisplayController displayController{imageLoader};
    FilterSortController filterSortController{displayController};
    SearchController searchController{displayController, filterSortController};
    TaskController taskController{importer, tagger, searchController};

    void connectSignalSlots();
    void initTagger();

    AboutDialog* aboutDialog = nullptr;
    SettingsDialog* settingsDialog = nullptr;
    void handleShowAboutAction();
    void handleShowSettingsAction();
};
