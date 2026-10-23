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
#include "task_controller.h"
#include "utils/logger.h"
#include "utils/settings.h"
#include <QFileDialog>
#include <QMessageBox>

TaskController::TaskController(TaskUi ui, Importer& importer, Tagger& tagger, SearchController& searchController)
    : ui(ui), importer(importer), tagger(tagger), searchController(searchController) {
    connectSignalSlots();
}

void TaskController::connectSignalSlots() {
    ui.progressWidget->hide();
    connect(ui.importNewPicsAction, &QAction::triggered, this, &TaskController::handleImportNewPicsAction);
    connect(ui.importPowerfulPixivDownloaderAction,
            &QAction::triggered,
            this,
            &TaskController::handleImportPowerfulPixivDownloaderAction);
    connect(ui.importGallery_dlTwitterAction, &QAction::triggered, this, &TaskController::handleImportGallery_dlTwitterAction);
    connect(
        ui.importExistingDirectoriesAction, &QAction::triggered, this, &TaskController::handleImportExistingDirectoriesAction);
    connect(ui.startTaggingAction, &QAction::triggered, this, &TaskController::handleStartTaggingAction);
    connect(ui.cancelProgressButton, &QPushButton::clicked, this, &TaskController::cancelTask);
}
void TaskController::displayImportProgress(size_t progress, size_t total) {
    if (progress >= total) { // import complete
        finalizeImport(total);
        return;
    }
    // update progress bar and status
    ui.progressBar->setMaximum(static_cast<int>(total));
    ui.progressBar->setValue(static_cast<int>(progress));

    auto currentTime = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - taskStartTime).count();

    if (timeDiff > 0 && progress > 0) {
        double timeDiffSeconds = static_cast<double>(timeDiff) / 1000.0;
        double speed = static_cast<double>(progress) / timeDiffSeconds; // items per second
        size_t etaSeconds = static_cast<size_t>((total - progress) / speed);
        ui.progressStatusLabel->setText(QString("%1 / %2 | 速度：%3 文件每秒 | 剩余时间：%4 秒")
                                            .arg(progress)
                                            .arg(total)
                                            .arg(QString::number(speed, 'f', 2))
                                            .arg(etaSeconds));
    }
}
void TaskController::finalizeImport(size_t totalImported) {
    if (dirsToImport.empty()) { // single import task
        // record imported directory
        std::filesystem::path importedPath;
        ParserType parserType;
        std::tie(importedPath, parserType) = importer.getImportingDir();
        if (std::find(Settings::picDirectories.begin(), Settings::picDirectories.end(), std::pair{importedPath, parserType}) ==
            Settings::picDirectories.end()) {
            Settings::picDirectories.emplace_back(importedPath, parserType);
        }
        importer.finish();
    } else { // re-importing from multiple directories
        dirsToImport.pop_back();
        importer.finish();
        if (!dirsToImport.empty()) {
            ui.progressBar->setValue(0);
            taskStartTime = std::chrono::steady_clock::now();
            ui.progressStatusLabel->setText(QString("- / - | 速度：- 文件每秒 | 剩余时间：- 秒"));
            importer.startImportFromDirectory(dirsToImport.back().first, dirsToImport.back().second);
            Info() << "Re-importing pictures from directory: " << dirsToImport.back().first.string();
            return;
        }
    }

    // hide progress
    ui.progressWidget->hide();
    ui.progressLabel->setText("");
    ui.progressStatusLabel->setText("");
    ui.statusbar->showMessage(
        "图片导入完成，共扫描 " + QString::number(totalImported) + " 文件，用时 " +
        QString::number(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - taskStartTime).count()) +
        " 秒");

    searchController.loadTags(); // load new tags from database
    if (searchController.isSearchCriteriaEmpty()) {
        searchController.picSearch();
    }
    Info() << "Import completed. Total files imported: " << totalImported;

    if (Settings::autoTagAfterImport) {
        Info() << "Starting auto tagging after import...";
        handleStartTaggingAction();
    }
    return;
}
void TaskController::displayTaggingProgress(size_t progress, size_t total) {
    if (progress >= total) { // tagging complete
        finalizeTagging(total);
        return;
    }
    // update progress bar and status
    ui.progressBar->setMaximum(static_cast<int>(total));
    ui.progressBar->setValue(static_cast<int>(progress));

    auto currentTime = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - taskStartTime).count();

    if (timeDiff > 0 && progress > 0) {
        double timeDiffSeconds = static_cast<double>(timeDiff) / 1000.0;
        double speed = static_cast<double>(progress) / timeDiffSeconds; // items per second
        size_t etaSeconds = static_cast<size_t>((total - progress) / speed);
        ui.progressStatusLabel->setText(QString("%1 / %2 | 速度：%3 文件每秒 | 剩余时间：%4 秒")
                                            .arg(progress)
                                            .arg(total)
                                            .arg(QString::number(speed, 'f', 2))
                                            .arg(etaSeconds));
    }
}
void TaskController::finalizeTagging(size_t totalTagged) {
    tagger.finish();

    // hide progress
    ui.progressWidget->hide();
    ui.progressLabel->setText("");
    ui.progressStatusLabel->setText("");
    ui.statusbar->showMessage(
        "标签分类完成，共处理 " + QString::number(totalTagged) + " 文件，用时 " +
        QString::number(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - taskStartTime).count()) +
        " 秒");

    searchController.loadTags(); // load new tags from database
    Info() << "Tagging completed. Total pics tagged: " << totalTagged;
    return;
}
void TaskController::cancelTask() {
    if (!haveOngoingTask()) return; // no import task to cancel

    if (!tagger.finish()) {
        ui.statusbar->showMessage("正在取消标签分类任务，请稍候...");
        Info() << "Cancelling tagging task...";

        tagger.forceStop();

        ui.statusbar->showMessage("标签分类任务已取消。");
        Info() << "Tagging task cancelled.";

    } else if (!importer.finish()) {
        ui.statusbar->showMessage("正在取消导入任务，请稍候...");
        Info() << "Cancelling import task...";

        importer.forceStop();
        dirsToImport.clear();

        ui.statusbar->showMessage("导入任务已取消。");
        Info() << "Import task cancelled.";
    }
    ui.progressWidget->hide();
    ui.progressLabel->setText("");
    ui.progressStatusLabel->setText("");
}

void TaskController::handleImportNewPicsAction() {
    if (haveOngoingTask()) {
        ui.statusbar->showMessage("已有任务正在进行中，请稍后再试。");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(nullptr, "选择图片文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui.statusbar->showMessage("正在扫描文件夹...");
    importer.startImportFromDirectory(std::filesystem::path(dir.toStdString()));
    ui.progressWidget->show();
    ui.progressBar->setValue(0);
    ui.progressLabel->setText("正在导入图片...");
    Info() << "Started importing pictures from directory: " << dir.toStdString();
    taskStartTime = std::chrono::steady_clock::now();
}
void TaskController::handleImportPowerfulPixivDownloaderAction() {
    if (haveOngoingTask()) {
        ui.statusbar->showMessage("已有任务正在进行中，请稍后再试。");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(
        nullptr, "选择 Powerful Pixiv Downloader 下载文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui.statusbar->showMessage("正在扫描 Powerful Pixiv Downloader 下载文件夹...");
    importer.startImportFromDirectory(std::filesystem::path(dir.toStdString()), ParserType::PowerfulPixivDownloader);
    ui.progressWidget->show();
    ui.progressBar->setValue(0);
    ui.progressLabel->setText("正在导入Pixiv图片...");
    Info() << "Started importing pixiv pictures from directory: " << dir.toStdString();
    taskStartTime = std::chrono::steady_clock::now();
}
void TaskController::handleImportGallery_dlTwitterAction() {
    if (haveOngoingTask()) {
        ui.statusbar->showMessage("已有任务正在进行中，请稍后再试。");
        return;
    }
    QString dir =
        QFileDialog::getExistingDirectory(nullptr, "选择 gallery-dl Twitter 下载文件夹", QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;
    ui.statusbar->showMessage("正在扫描 gallery-dl Twitter 下载文件夹...");
    importer.startImportFromDirectory(std::filesystem::path(dir.toStdString()), ParserType::GallerydlTwitter);
    ui.progressWidget->show();
    ui.progressBar->setValue(0);
    ui.progressLabel->setText("正在导入Twitter图片...");
    Info() << "Started importing twitter pictures from directory: " << dir.toStdString();
    taskStartTime = std::chrono::steady_clock::now();
}
void TaskController::handleImportExistingDirectoriesAction() {
    if (haveOngoingTask()) {
        ui.statusbar->showMessage("已有任务正在进行中，请稍后再试。");
        return;
    }
    ui.statusbar->showMessage("正在重新扫描已导入的图片文件夹...");
    Info() << "Started re-importing pictures from existing directories.";
    for (const auto& dir : Settings::picDirectories) {
        dirsToImport.emplace_back(dir);
    }
    if (dirsToImport.empty()) {
        ui.statusbar->showMessage("没有已导入的图片文件夹可供重新扫描。");
        Info() << "No existing directories to re-import.";
        return;
    }
    importer.startImportFromDirectory(dirsToImport.back().first, dirsToImport.back().second);
    Info() << "Re-importing pictures from directory: " << dirsToImport.back().first.string();
    ui.progressWidget->show();
    ui.progressBar->setValue(0);
    ui.progressLabel->setText("正在重新扫描已导入文件夹...");
    taskStartTime = std::chrono::steady_clock::now();
}
void TaskController::handleStartTaggingAction() {
    if (haveOngoingTask()) {
        ui.statusbar->showMessage("已有任务正在进行中，请稍后再试。");
        return;
    }
    if (!tagger.taggerLoaded()) {
        Error() << "No tagger loaded. Cannot start tagging.";
        ui.statusbar->showMessage("未找到可用的标签分类器，无法开始标签分类。");
        return;
    }
    ui.statusbar->showMessage("正在对图片进行标签分类...");
    tagger.startTagging();
    ui.progressWidget->show();
    ui.progressBar->setValue(0);
    QString progressLabelText = "正在对图片进行标签分类...";
    if (tagger.gpuAvailable()) {
        progressLabelText += "（GPU 加速）";
    } else {
        progressLabelText += "（CPU）";
    }
    ui.progressLabel->setText(progressLabelText);
    Info() << "Started tagging pictures with tagger.";
    taskStartTime = std::chrono::steady_clock::now();
}
