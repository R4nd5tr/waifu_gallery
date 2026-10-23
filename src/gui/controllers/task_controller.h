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
#include "search_controller.h"
#include "service/importer.h"
#include "service/tagger.h"
#include <QAction>
#include <QLabel>
#include <QObject>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QWidget>
#include <chrono>
#include <filesystem>
#include <vector>

struct TaskUi {
    QAction* importNewPicsAction;
    QAction* importPowerfulPixivDownloaderAction;
    QAction* importGallery_dlTwitterAction;
    QAction* importExistingDirectoriesAction;
    QAction* startTaggingAction;
    QWidget* progressWidget;
    QProgressBar* progressBar;
    QLabel* progressLabel;
    QLabel* progressStatusLabel;
    QStatusBar* statusbar;
    QPushButton* cancelProgressButton;
};

class TaskController : public QObject {
    Q_OBJECT
public:
    explicit TaskController(TaskUi ui, Importer& importer, Tagger& tagger, SearchController& searchController);

    void displayImportProgress(size_t progress, size_t total);
    void displayTaggingProgress(size_t progress, size_t total);
    void handleImportExistingDirectoriesAction();
signals:

private:
    TaskUi ui;
    Importer& importer;
    Tagger& tagger;
    SearchController& searchController;

    std::chrono::steady_clock::time_point taskStartTime;
    std::vector<std::pair<std::filesystem::path, ParserType>> dirsToImport; // paths to import with specified parser types

    void connectSignalSlots();

    void handleImportNewPicsAction();
    void handleImportPowerfulPixivDownloaderAction(); // specify parser type pixiv
    void handleImportGallery_dlTwitterAction();       // specify parser type twitter
    void handleStartTaggingAction();

    bool haveOngoingTask() { return !(importer.finish() && tagger.finish()); }

    void cancelTask();

    void finalizeImport(size_t totalImported);
    void finalizeTagging(size_t totalTagged);
};
