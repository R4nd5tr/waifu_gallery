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
#include "utils/settings.h"
#include <QDialog>
#include <QFileDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();
    void loadSettings();
    void saveSettings();

    void addPicDirectory();
    void deletePicDirectory();
    void addPixivDirectory();
    void deletePixivDirectory();
    void addTwitterDirectory();
    void deleteTwitterDirectory();

private:
    Ui::SettingsDialog* ui;
    bool importOnStartup;
    std::vector<std::filesystem::path> picDirectories;
    std::vector<std::filesystem::path> pixivDirectories;
    std::vector<std::filesystem::path> tweetDirectories;
};