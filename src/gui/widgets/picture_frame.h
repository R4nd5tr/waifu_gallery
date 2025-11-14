/*
 * Waifu Gallery - A Qt-based anime illustration gallery application.
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

#include "clickable_label.h"
#include "service/database.h"
#include "service/model.h"
#include <QDesktopServices>
#include <QFrame>
#include <QPixmap>
#include <QUrl>

namespace Ui {
class PictureFrame;
}

class PictureFrame : public QFrame {
    Q_OBJECT
public:
    explicit PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField = SearchField::None);
    ~PictureFrame();
    void setPixmap(const QPixmap& pixmap);

private:
    Ui::PictureFrame* ui;
    std::unordered_set<std::filesystem::path> imgPaths;
    QString illustratorURL;
    QString idURL;
    void openFileWithDefaultApp() {
        for (const auto& path : imgPaths) {
            try {
                QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path.string())));
                break;
            } catch (...) {
                continue;
            }
        }
    }
    void openFileLocation() {
        for (const auto& path : imgPaths) {
            try {
                std::wstring command = L"explorer /select,\"";
                std::wstring winPath = path.wstring();
                std::replace(winPath.begin(), winPath.end(), L'/', L'\\');
                command += winPath;
                command += L"\"";
                int result = _wsystem(command.c_str());
                if (result == -1) {
                    Info() << "Failed to open file location for path:" << path;
                    continue;
                }
            } catch (...) {
                continue;
            }
            break;
        }
    }
    void openIllustratorUrl() {
        if (illustratorURL.isEmpty()) return;
        QDesktopServices::openUrl(QUrl(illustratorURL));
    }
    void openIdUrl() {
        if (idURL.isEmpty()) return;
        QDesktopServices::openUrl(QUrl(idURL));
    }
};
