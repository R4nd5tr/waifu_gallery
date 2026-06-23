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

#include "picture_frame.h"
#include "../controllers/image_loader.h"
#include "ui_picture_frame.h"

QString getFileTypeStr(ImageFormat fileType) {
    switch (fileType) {
    case ImageFormat::JPG:
        return {"JPG"};
    case ImageFormat::PNG:
        return {"PNG"};
    case ImageFormat::GIF:
        return {"GIF"};
    case ImageFormat::WebP:
        return {"WebP"};
    default:
        return {"Unknown"};
    }
}
static size_t wrapIndex(int index, size_t count) {
    if (count == 0) return 0;
    int m = index % static_cast<int>(count);
    if (m < 0) m += static_cast<int>(count);
    return static_cast<size_t>(m);
}

constexpr int MAX_TITLE_LENGTH = 20;

const std::string PIXIV_BASE_URL = "https://www.pixiv.net/artworks/";
const std::string PIXIV_AUTHOR_URL = "https://www.pixiv.net/users/";
const std::string TWITTER_BASE_URL = "https://twitter.com/i/web/status/";
const std::string TWITTER_AUTHOR_URL = "https://twitter.com/";

constexpr size_t PRELOAD_PREVIEW_COUNT = 2;

// initialize

PictureFrame::PictureFrame(
    QWidget* parent, const PicItem* picItem, const MetadataItem* metadataItem, ImageLoader& imageLoader, SearchField searchField)
    : QFrame(parent), ui(new Ui::PictureFrame), picItem(picItem), metadataItem(metadataItem), imageLoader(imageLoader) {
    ui->setupUi(this);
    connectSignals();
    showInfo(searchField);
    showThumbnail();
}
PictureFrame::~PictureFrame() {
    delete ui;
}
void PictureFrame::connectSignals() {
    connect(ui->imageLabel, &ImageLabel::loadPreviewSignal, this, &PictureFrame::loadPreviewImage);
    connect(ui->imageLabel, &ImageLabel::showPreviewSignal, this, &PictureFrame::showPreview);

    connect(ui->resolutionLabel, &ClickableLabel::clicked, this, &PictureFrame::openFileWithDefaultApp);
    connect(ui->fileTypeAndSizeLabel, &ClickableLabel::clicked, this, &PictureFrame::openFileLocation);
    connect(ui->illustratorLabel, &ClickableLabel::clicked, this, &PictureFrame::openIllustratorUrl);
    connect(ui->idLabel, &ClickableLabel::clicked, this, &PictureFrame::openIdUrl);

    this->installEventFilter(this);
    ui->imageLabel->installEventFilter(this);
    previewer.installEventFilter(this);
}
void PictureFrame::reset() {
    ui->imageLabel->clear();
    ui->titleLabel->clear();
    ui->illustratorLabel->clear();
    ui->idLabel->clear();
    ui->resolutionLabel->clear();
    ui->fileTypeAndSizeLabel->clear();

    hidePreview();

    displayingPreview = false;
    previewingIndex = 0;
    picItem = nullptr;
    metadataItem = nullptr;

    released = true;
}

void PictureFrame::showInfo(SearchField searchField) const {
    if (released) return;
    showPicInfo();

    ui->titleLabel->setResponsive(false);
    // show metadata if available, otherwise show filename and disable links
    if (metadataItem) { // display metadata if available, otherwise show filename and disable links
        switch (metadataItem->metadata.platformType) {
        case PlatformType::Pixiv:
            ui->titleLabel->setText(QString::fromStdString(metadataItem->metadata.title));
            ui->illustratorLabel->setText(QString::fromStdString(metadataItem->metadata.authorName));
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(metadataItem->metadata.id)));
            break;
        case PlatformType::Twitter: {
            QString description = QString::fromStdString(metadataItem->metadata.description).split('\n').first();
            if (description.length() > MAX_TITLE_LENGTH) {
                description = description.left(MAX_TITLE_LENGTH) + "...";
            }
            ui->titleLabel->setText(description);
            ui->illustratorLabel->setText(QString::fromStdString(metadataItem->metadata.authorNick));
            ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(metadataItem->metadata.authorName)));
            break;
        }
        default:
            ui->titleLabel->setText(QString::fromStdString(metadataItem->metadata.title));
            ui->idLabel->setText(QString::number(metadataItem->metadata.id));
            ui->illustratorLabel->setResponsive(false);
            ui->idLabel->setResponsive(false);
            break;
        }
    } else { // no metadata, show filename and disable links
        QString filename = QString::fromStdString(picItem->info.filePaths.begin()->filename().string());
        if (filename.length() > MAX_TITLE_LENGTH) {
            filename = filename.left(MAX_TITLE_LENGTH) + "...";
        }
        ui->titleLabel->setText(filename);
        ui->idLabel->setText("N/A");
        ui->illustratorLabel->setResponsive(false);
        ui->idLabel->setResponsive(false);
    }

    switch (searchField) { // highlight search result
    case SearchField::Title: {
        ui->titleLabel->setHighlighted(true);
        break;
    }
    case SearchField::AuthorID:
    case SearchField::AuthorName:
    case SearchField::AuthorNick: {
        ui->illustratorLabel->setHighlighted(true);
        break;
    }
    case SearchField::PlatformID: {
        ui->idLabel->setHighlighted(true);
        break;
    }
    default:
        break;
    }
}
void PictureFrame::showPicInfo() const {
    if (released) return;
    if (metadataItem && metadataItem->picCount > 1 && !displayingPreview) {
        ui->resolutionLabel->setText("");
        ui->fileTypeAndSizeLabel->setText(QString("%1 pics").arg(metadataItem->picCount));
        ui->resolutionLabel->setResponsive(false);
        ui->fileTypeAndSizeLabel->setResponsive(false);
    } else {
        showPicInfo(previewingIndex);
    }
}
void PictureFrame::showPicInfo(size_t index) const {
    if (released) return;
    if (metadataItem && index >= metadataItem->picCount) return;
    const PicInfo& currentPic = (picItem + index)->info;
    ui->resolutionLabel->setText(QString("%1x%2").arg(currentPic.width).arg(currentPic.height));
    ui->fileTypeAndSizeLabel->setText(QString("%1 | %2 MB")
                                          .arg(getFileTypeStr(currentPic.fileType))
                                          .arg(static_cast<double>(currentPic.size) / (1024 * 1024), 0, 'f', 2));
    ui->resolutionLabel->setResponsive(true);
    ui->fileTypeAndSizeLabel->setResponsive(true);
}
void PictureFrame::showThumbnail() const {
    if (released) return;
    if (!picItem) return;

    if (auto img = imageLoader.getImage(picItem->info, LoadType::Thumbnail)) {
        QPixmap pixmap = QPixmap::fromImage(*img);
        ui->imageLabel->setPixmap(pixmap);
    }
}

// asynchronous loaded image display

void PictureFrame::displayImage(uint64_t picId, LoadType loadType) {
    if (released) return;
    if (const auto img = imageLoader.getImage(picId, loadType)) {
        if (loadType == LoadType::Thumbnail) {
            QPixmap pixmap = QPixmap::fromImage(*img);
            ui->imageLabel->setPixmap(pixmap);
        } else if (loadType == LoadType::Preview && displayingPreview && (picItem + previewingIndex)->info.id == picId) {
            // only display if it's the preview image currently being previewed
            displayPreviewImage();
        }
    } else {
        Error() << "Failed to display image for PicID:" << picId;
    }
}

// previewer

bool PictureFrame::eventFilter(QObject* watched, QEvent* event) {
    if ((watched == ui->imageLabel || watched == &previewer || watched == this) && event->type() == QEvent::Wheel) {
        if (!displayingPreview || !metadataItem || metadataItem->picCount <= 1) {
            return QFrame::eventFilter(watched, event);
        }

        auto* wheelEvent = dynamic_cast<QWheelEvent*>(event);

        if (wheelEvent->angleDelta().y() > 0) {
            previewingIndex = wrapIndex(static_cast<int>(previewingIndex) - 1, metadataItem->picCount);
        } else {
            previewingIndex = wrapIndex(static_cast<int>(previewingIndex) + 1, metadataItem->picCount);
        }
        displayPreviewImage();
        showPicInfo(previewingIndex);

        wheelEvent->accept();
        return true;
    }

    return QFrame::eventFilter(watched, event);
}
void PictureFrame::loadPreviewImage() const {
    if (released || !picItem || !metadataItem) return;

    const size_t count = metadataItem ? metadataItem->picCount : 0;
    const size_t center = wrapIndex(static_cast<int>(previewingIndex), count);

    imageLoader.getImage((picItem + center)->info, LoadType::Preview);

    for (size_t offset = 1; offset <= PRELOAD_PREVIEW_COUNT; ++offset) {
        const size_t left = wrapIndex(static_cast<int>(center) - static_cast<int>(offset), count);
        const size_t right = wrapIndex(static_cast<int>(center) + static_cast<int>(offset), count);

        imageLoader.getImage((picItem + left)->info, LoadType::Preview);
        if (right != left) {
            imageLoader.getImage((picItem + right)->info, LoadType::Preview);
        }
    }
}
void PictureFrame::displayPreviewImage(size_t index) {
    if (released || !picItem) return;
    if (auto img = imageLoader.getImage((picItem + index)->info, LoadType::Preview)) { // cache hit
        QPixmap pixmap = QPixmap::fromImage(*img);
        previewer.setPixmap(pixmap);
        const QPoint topLeft = mapToGlobal(QPoint(0, 0));
        const int x = topLeft.x() - previewer.width();

        QWidget* win = window();
        const QRect winRect = win ? win->frameGeometry() : QRect(topLeft, size());
        const int widgetCenterY = topLeft.y() + height() / 2;
        const bool upperHalf = widgetCenterY < winRect.center().y();

        const int yPos = upperHalf ? topLeft.y() : (topLeft.y() + height() - previewer.height());

        previewer.move(x, yPos);
        showPicInfo(index);
    }
    // cache miss, the preview image will be loaded asynchronously and
    // displayed when it's ready through displayImage function
}
void PictureFrame::displayPreviewImage() {
    displayPreviewImage(previewingIndex);
}
void PictureFrame::showPreview() {
    if (released) return;
    displayPreviewImage();
    previewer.show();
    displayingPreview = true;
}
void PictureFrame::hidePreview() {
    if (released) return;
    previewer.hide();
    displayingPreview = false;
}

// shortcuts

void PictureFrame::openFileWithDefaultApp() const {
    if (released) return;
    for (const auto& path : picItem->info.filePaths) {
        try {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path.string())));
            break;
        } catch (...) {
            continue;
        }
    }
}
void PictureFrame::openFileLocation() const {
    if (released) return;
    for (const auto& path : picItem->info.filePaths) {
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
void PictureFrame::openIllustratorUrl() const {
    if (released) return;
    if (!metadataItem) return;
    if (metadataItem->metadata.platformType == PlatformType::Pixiv) {
        QDesktopServices::openUrl(
            QUrl(QString::fromStdString(PIXIV_AUTHOR_URL + std::to_string(metadataItem->metadata.authorID))));
    } else if (metadataItem->metadata.platformType == PlatformType::Twitter) {
        QDesktopServices::openUrl(
            QUrl(QString::fromStdString(TWITTER_AUTHOR_URL + std::to_string(metadataItem->metadata.authorID))));
    }
}
void PictureFrame::openIdUrl() const {
    if (released) return;
    if (!metadataItem) return;
    if (metadataItem->metadata.platformType == PlatformType::Pixiv) {
        QDesktopServices::openUrl(QUrl(QString::fromStdString(PIXIV_BASE_URL + std::to_string(metadataItem->metadata.id))));
    } else if (metadataItem->metadata.platformType == PlatformType::Twitter) {
        QDesktopServices::openUrl(QUrl(QString::fromStdString(TWITTER_BASE_URL + std::to_string(metadataItem->metadata.id))));
    }
}
