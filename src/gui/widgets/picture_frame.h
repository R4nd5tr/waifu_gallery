#pragma once

#include "../../service/database.h"
#include "../../service/model.h"
#include "clickable_label.h"
#include <QDesktopServices>
#include <QFrame>
#include <QPixmap>
#include <QUrl>
#define NOMINMAX
#include <windows.h>

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
