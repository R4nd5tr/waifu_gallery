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
#include <QMessageBox>

QComboBox* createParserComboBox(ParserType type) {
    auto* comboBox = new QComboBox();
    comboBox->addItems(QStringList(PARSER_TYPE_STRS.begin(), PARSER_TYPE_STRS.end()));
    comboBox->setCurrentIndex(static_cast<int>(type));
    comboBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    return comboBox;
}
QTableWidgetItem* createDirItem(const QString& dir) {
    auto* item = new QTableWidgetItem(dir);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setToolTip(dir);
    return item;
}
SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    loadSettings();
    setupTableWidget();
    connect(ui->addPicDirButton, &QPushButton::clicked, this, &SettingsDialog::addPicDirectory);
    connect(ui->delPicDirButton, &QPushButton::clicked, this, &SettingsDialog::deletePicDirectory);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
}
SettingsDialog::~SettingsDialog() {
    delete ui;
}
void SettingsDialog::setupTableWidget() {
    QStringList headers;
    headers << "图片目录"
            << "解析器类型";
    ui->picDirsTable->setColumnCount(headers.size());
    ui->picDirsTable->setHorizontalHeaderLabels(headers);
    ui->picDirsTable->verticalHeader()->hide();
    ui->picDirsTable->setFrameShape(QFrame::NoFrame);
    ui->picDirsTable->setShowGrid(false);
    ui->picDirsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->picDirsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->picDirsTable->horizontalHeader()->setStretchLastSection(true);
    ui->picDirsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->picDirsTable->setWordWrap(false);
    ui->picDirsTable->setColumnWidth(0, 250);
}
void SettingsDialog::loadSettings() {
    importOnStartup = Settings::autoImportOnStartup;
    ui->importOnStartCheckBox->setChecked(importOnStartup);

    autoTagAfterImport = Settings::autoTagAfterImport;
    ui->autoTagAfterImportCheckBox->setChecked(autoTagAfterImport);
    autoTaggerDLLPath = Settings::autoTaggerDLLPath;
    ui->autoTaggerDLLPathLineEdit->setText(QString::fromStdString(autoTaggerDLLPath.string()));

    picDirectories = Settings::picDirectories;
    ui->picDirsTable->clear();
    ui->picDirsTable->setRowCount(static_cast<int>(picDirectories.size()));
    for (int i = 0; i < picDirectories.size(); i++) {
        ui->picDirsTable->setItem(i, 0, createDirItem(QString::fromStdString(picDirectories[i].first.string())));
        ui->picDirsTable->setCellWidget(i, 1, createParserComboBox(picDirectories[i].second));
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

    autoTagAfterImport = ui->autoTagAfterImportCheckBox->isChecked();
    Settings::autoTagAfterImport = autoTagAfterImport;

    autoTaggerDLLPath = ui->autoTaggerDLLPathLineEdit->text().toStdString();
    Settings::autoTaggerDLLPath = autoTaggerDLLPath;

    Settings::saveSettings();
    accept();
}
void SettingsDialog::addPicDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择图片目录");
    if (dir.isEmpty()) return;

    picDirectories.emplace_back(dir.toStdString(), ParserType::None);
    int newRow = ui->picDirsTable->rowCount();
    ui->picDirsTable->setRowCount(newRow + 1);
    ui->picDirsTable->setItem(newRow, 0, createDirItem(dir));
    ui->picDirsTable->setCellWidget(newRow, 1, createParserComboBox(ParserType::None));
}
void SettingsDialog::deletePicDirectory() {
    int currentRow = ui->picDirsTable->currentRow();
    if (currentRow < 0) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", "确定要删除选中的目录吗？", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) return;

    picDirectories.erase(picDirectories.begin() + currentRow);
    ui->picDirsTable->removeRow(currentRow);
}
