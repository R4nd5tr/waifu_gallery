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

#include "main_window.h"
#include "controllers/support/utils.h"
#include "service/database.h"
#include "ui_main_window.h"
#include "utils/settings.h"
#include <QFileDialog>
#include <QScrollBar>
#include <QString>

const QEvent::Type ImportProgressReportEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type TaggingProgressReportEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type ImageLoadCompleteEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());

// MainWindow implementation

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      displayController(DisplayUi{ui->picBrowseWidget, ui->picBrowseScrollArea->verticalScrollBar(), ui->picBrowseScrollArea},
                        imageLoader),
      filterSortController(FilterSortUi{ui->unknowPlatformCheckBox,
                                        ui->pixivPlatformCheckBox,
                                        ui->twitterPlatformCheckBox,
                                        ui->jpgCheckBox,
                                        ui->pngCheckBox,
                                        ui->gifCheckBox,
                                        ui->webpCheckBox,
                                        ui->unknowRestrictCheckBox,
                                        ui->allAgeCheckBox,
                                        ui->sensitiveCheckBox,
                                        ui->questionableCheckBox,
                                        ui->r18CheckBox,
                                        ui->r18gCheckBox,
                                        ui->unknowAICheckBox,
                                        ui->aiCheckBox,
                                        ui->noAICheckBox,
                                        ui->maxWidthEdit,
                                        ui->maxHeightEdit,
                                        ui->minWidthEdit,
                                        ui->minHeightEdit,
                                        ui->clearResolutionFilterButton,
                                        ui->enableRatioCheckBox,
                                        ui->sortComboBox,
                                        ui->orderComboBox,
                                        ui->ratioSlider,
                                        ui->widthRatioSpinBox,
                                        ui->heightRatioSpinBox},
                           displayController),
      searchController(SearchUi{ui->searchPlatformComboBox,
                                ui->searchComboBox,
                                ui->searchLineEdit,
                                ui->generalTagList,
                                ui->characterTagList,
                                ui->platformTagList,
                                ui->clearTextSearchButton,
                                ui->statusbar,
                                ui->selectedTagScrollArea,
                                ui->selectedTagWidget,
                                ui->tagListTabWidget,
                                ui->selectedTagLayout,
                                ui->clearTextSearchButton,
                                ui->searchTagTextEdit},
                       displayController,
                       filterSortController),
      taskController(TaskUi{ui->importNewPicsAction,
                            ui->importPowerfulPixivDownloaderAction,
                            ui->importGallery_dlTwitterAction,
                            ui->importExistingDirectoriesAction,
                            ui->startTaggingAction,
                            ui->progressWidget,
                            ui->progressBar,
                            ui->progressLabel,
                            ui->progressStatusLabel,
                            ui->statusbar,
                            ui->cancelProgressButton},
                     importer,
                     tagger,
                     searchController) {
    ui->setupUi(this);

    Settings::loadSettings();
    resize(Settings::windowWidth, Settings::windowHeight);
    if (Settings::autoImportOnStartup) taskController.handleImportExistingDirectoriesAction();

    initInterface();
    initWorkerThreads();
    connectSignalSlots();
    initTagger();
}
MainWindow::~MainWindow() {
    delete ui;
    searchWorkerThread->quit();
    searchWorkerThread->wait();
    Settings::saveSettings();
}

// Initialize functions

void MainWindow::initInterface() {
    ui->selectedTagScrollArea->hide();
    ui->progressWidget->hide();
}

void MainWindow::initWorkerThreads() {
    searchWorkerThread = new QThread(this);
    DatabaseWorker* searchWorker = new DatabaseWorker();
    searchWorker->moveToThread(searchWorkerThread);
    connect(this, &MainWindow::searchPics, searchWorker, &DatabaseWorker::searchPics);
    connect(searchWorker, &DatabaseWorker::searchComplete, &searchController, &SearchController::handleSearchResults);
    connect(searchWorkerThread, &QThread::finished, searchWorker, &QObject::deleteLater);
    searchWorkerThread->start();
}
void MainWindow::connectSignalSlots() {
    connect(ui->showAboutAction, &QAction::triggered, this, &MainWindow::handleShowAboutAction);
    connect(ui->showSettingsAction, &QAction::triggered, this, &MainWindow::handleShowSettingsAction);
}

void MainWindow::initTagger() {
    std::vector<std::filesystem::path> taggerPaths = tagger.getExistingTaggerDLLs();
    if (taggerPaths.empty()) {
        Warn() << "No tagger DLLs found. Tagger not initialized.";
        return;
    }
    tagger.loadTaggerDLL(taggerPaths[0]); // TODO: load the first available tagger DLL for now, implement selection later
    Info() << "Tagger initialized with DLL:" << taggerPaths[0];
    Settings::autoTaggerDLLPath = taggerPaths[0];
    if (database.getModelName() != tagger.getModelName()) {
        Info() << "Tagger model name differs from database record. Updating database tag set.";
        tagger.loadTagSetToDatabase();
    }
}

// Functions for window resizing and layout

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (firstShow_) {
        firstShow_ = false;
        displayController.handleWindowResize();
    }
}
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    Settings::setWidthHeight(width(), height());
    displayController.handleWindowResize();
}

// Custom event handling for image loading and import progress

bool MainWindow::event(QEvent* event) {
    if (event->type() == ImageLoadCompleteEvent::EventType) {
        auto* imageEvent = static_cast<ImageLoadCompleteEvent*>(event);
        displayController.displayImage(imageEvent->result.id, imageEvent->result.loadType);
        return true;
    } else if (event->type() == ImportProgressReportEvent::EventType) {
        auto* importProgressEvent = static_cast<ImportProgressReportEvent*>(event);
        taskController.displayImportProgress(importProgressEvent->progress, importProgressEvent->total);
        return true;
    } else if (event->type() == TaggingProgressReportEvent::EventType) {
        auto* taggingProgressEvent = static_cast<TaggingProgressReportEvent*>(event);
        taskController.displayTaggingProgress(taggingProgressEvent->progress, taggingProgressEvent->total);
        return true;
    }
    return QMainWindow::event(event);
}

// Handlers for menu actions

void MainWindow::handleShowAboutAction() {
    if (!aboutDialog) {
        aboutDialog = new AboutDialog(this);
        aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(aboutDialog, &QObject::destroyed, this, [this]() { aboutDialog = nullptr; });
    }
    aboutDialog->show();
    aboutDialog->raise();
    aboutDialog->activateWindow();
}
void MainWindow::handleShowSettingsAction() {
    if (!settingsDialog) {
        settingsDialog = new SettingsDialog(this);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(settingsDialog, &QObject::destroyed, this, [this]() { settingsDialog = nullptr; });
    }
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}
