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
#include "utils.h"
#include <QComboBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    loadSettings();
    connect(ui->addPicDirButton, &QPushButton::clicked, this, &SettingsDialog::addPicDirectory);
    connect(ui->delPicDirButton, &QPushButton::clicked, this, &SettingsDialog::deletePicDirectory);

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
    ui->picDirsTable->clear();
    for (int i = 0; i < picDirectories.size(); i++) {
        auto* dirItem = new QTableWidgetItem(QString::fromStdString(picDirectories[i].first.string()));
        auto* parserComboBox = new QComboBox();
        parserComboBox->addItems(QStringList(PARSER_TYPE_STRS.begin(), PARSER_TYPE_STRS.end()));
        parserComboBox->setCurrentIndex(static_cast<int>(picDirectories[i].second));
        dirItem->setFlags(dirItem->flags() | Qt::ItemIsEditable);
        ui->picDirsTable->setItem(i, 0, dirItem);
        ui->picDirsTable->setCellWidget(i, 1, parserComboBox);
    }
}
void SettingsDialog::saveSettings() {
    importOnStartup = ui->importOnStartCheckBox->isChecked();
    Settings::autoImportOnStartup = importOnStartup;

    picDirectories.clear();
    for (int i = 0; i < ui->picDirsTable->rowCount(); ++i) {
        picDirectories.emplace_back(
            ui->picDirsTable->item(i, 0)->text().toStdString(),
            static_cast<ParserType>(static_cast<QComboBox*>(ui->picDirsTable->cellWidget(i, 1))->currentIndex()));
    }
    Settings::picDirectories = picDirectories;
}
void SettingsDialog::addPicDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择图片目录");
    if (!dir.isEmpty()) {
        picDirectories.emplace_back(dir.toStdString(), ParserType::None);
        auto* item = new QTableWidgetItem(dir);
        auto* parserComboBox = new QComboBox();
        parserComboBox->addItems(QStringList(PARSER_TYPE_STRS.begin(), PARSER_TYPE_STRS.end()));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->picDirsTable->setItem(ui->picDirsTable->rowCount(), 0, item);
        ui->picDirsTable->setCellWidget(ui->picDirsTable->rowCount(), 1, parserComboBox);
    }
}
void SettingsDialog::deletePicDirectory() {
    auto* item = ui->picDirsTable->currentItem();
    if (item) {
        picDirectories.erase(picDirectories.begin() + ui->picDirsTable->row(item));
        delete item;
    }
}
