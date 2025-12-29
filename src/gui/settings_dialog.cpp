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

#include "settings_dialog.h"
#include "ui_settings_dialog.h"

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    loadSettings();
    connect(ui->addPicDirButton, &QPushButton::clicked, this, &SettingsDialog::addPicDirectory);
    connect(ui->delPicDirButton, &QPushButton::clicked, this, &SettingsDialog::deletePicDirectory);
    connect(ui->addPixivDirButton, &QPushButton::clicked, this, &SettingsDialog::addPixivDirectory);
    connect(ui->delPixivDirButton, &QPushButton::clicked, this, &SettingsDialog::deletePixivDirectory);
    connect(ui->addTwitterDirButton, &QPushButton::clicked, this, &SettingsDialog::addTwitterDirectory);
    connect(ui->delTwitterDirButton, &QPushButton::clicked, this, &SettingsDialog::deleteTwitterDirectory);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
}
SettingsDialog::~SettingsDialog() {
    delete ui;
}
void SettingsDialog::loadSettings() {
    importOnStartup = Settings::autoImportOnStartup;
    ui->importOnStartCheckBox->setChecked(importOnStartup);

    picDirectories = Settings::picDirectories;
    ui->picDirsList->clear();
    for (const auto& dir : picDirectories) {
        auto* item = new QListWidgetItem(QString::fromStdString(dir.string()));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->picDirsList->addItem(item);
    }

    pixivDirectories = Settings::pixivDirectories;
    ui->pixivPicDirsList->clear();
    for (const auto& dir : pixivDirectories) {
        auto* item = new QListWidgetItem(QString::fromStdString(dir.string()));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->pixivPicDirsList->addItem(item);
    }

    tweetDirectories = Settings::tweetDirectories;
    ui->twitterPicDirsList->clear();
    for (const auto& dir : tweetDirectories) {
        auto* item = new QListWidgetItem(QString::fromStdString(dir.string()));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->twitterPicDirsList->addItem(item);
    }
}
void SettingsDialog::saveSettings() {
    importOnStartup = ui->importOnStartCheckBox->isChecked();
    Settings::autoImportOnStartup = importOnStartup;

    picDirectories.clear();
    for (int i = 0; i < ui->picDirsList->count(); ++i) {
        picDirectories.emplace_back(ui->picDirsList->item(i)->text().toStdString());
    }
    Settings::picDirectories = picDirectories;

    pixivDirectories.clear();
    for (int i = 0; i < ui->pixivPicDirsList->count(); ++i) {
        pixivDirectories.emplace_back(ui->pixivPicDirsList->item(i)->text().toStdString());
    }
    Settings::pixivDirectories = pixivDirectories;

    tweetDirectories.clear();
    for (int i = 0; i < ui->twitterPicDirsList->count(); ++i) {
        tweetDirectories.emplace_back(ui->twitterPicDirsList->item(i)->text().toStdString());
    }
    Settings::tweetDirectories = tweetDirectories;
}
void SettingsDialog::addPicDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择图片目录");
    if (!dir.isEmpty()) {
        picDirectories.push_back(dir.toStdString());
        auto* item = new QListWidgetItem(dir);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->picDirsList->addItem(item);
    }
}
void SettingsDialog::deletePicDirectory() {
    auto* item = ui->picDirsList->currentItem();
    if (item) {
        picDirectories.erase(picDirectories.begin() + ui->picDirsList->row(item));
        delete item;
    }
}
void SettingsDialog::addPixivDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择Pixiv图片目录");
    if (!dir.isEmpty()) {
        pixivDirectories.push_back(dir.toStdString());
        auto* item = new QListWidgetItem(dir);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->pixivPicDirsList->addItem(item);
    }
}
void SettingsDialog::deletePixivDirectory() {
    auto* item = ui->pixivPicDirsList->currentItem();
    if (item) {
        pixivDirectories.erase(pixivDirectories.begin() + ui->pixivPicDirsList->row(item));
        delete item;
    }
}
void SettingsDialog::addTwitterDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择Twitter图片目录");
    if (!dir.isEmpty()) {
        tweetDirectories.push_back(dir.toStdString());
        auto* item = new QListWidgetItem(dir);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->twitterPicDirsList->addItem(item);
    }
}
void SettingsDialog::deleteTwitterDirectory() {
    auto* item = ui->twitterPicDirsList->currentItem();
    if (item) {
        tweetDirectories.erase(tweetDirectories.begin() + ui->twitterPicDirsList->row(item));
        delete item;
    }
}