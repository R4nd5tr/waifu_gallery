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

#include "picture_frame.h"
#include "ui_picture_frame.h"

QString getFileTypeStr(ImageFormat fileType) {
    switch (fileType) {
    case ImageFormat::JPG:
        return QString("JPG");
    case ImageFormat::PNG:
        return QString("PNG");
    case ImageFormat::GIF:
        return QString("GIF");
    case ImageFormat::WebP:
        return QString("WebP");
    default:
        return QString("Unknown");
    }
}
const int MAX_TITLE_LENGTH = 20;
const std::string PIXIV_BASE_URL = "https://www.pixiv.net/artworks/";
const std::string PIXIV_AUTHOR_URL = "https://www.pixiv.net/users/";
const std::string TWITTER_BASE_URL = "https://twitter.com/i/web/status/";
const std::string TWITTER_AUTHOR_URL = "https://twitter.com/";
const QFont HIGHLIGHT_FONT("", 10, QFont::Bold);

PictureFrame::PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField)
    : QFrame(parent), ui(new Ui::PictureFrame) {
    ui->setupUi(this);
    ui->resolutionLabel->setText(QString("%1x%2").arg(picinfo.width).arg(picinfo.height));
    ui->fileTypeAndSizeLabel->setText(QString("%1 | %2 MB")
                                          .arg(getFileTypeStr(picinfo.fileType))
                                          .arg(static_cast<double>(picinfo.size) / (1024 * 1024), 0, 'f', 2));
    imgPaths = picinfo.filePaths;

    ui->titleLabel->setResponsive(false);

    connect(ui->resolutionLabel, &ClickableLabel::clicked, this, &PictureFrame::openFileWithDefaultApp);
    connect(ui->fileTypeAndSizeLabel, &ClickableLabel::clicked, this, &PictureFrame::openFileLocation);
    connect(ui->illustratorLabel, &ClickableLabel::clicked, this, &PictureFrame::openIllustratorUrl);
    connect(ui->idLabel, &ClickableLabel::clicked, this, &PictureFrame::openIdUrl);

    if (picinfo.pixivInfo.size() > 0) { // display pixiv info if available
        ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
        ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
        ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));

        illustratorURL = QString::fromStdString(PIXIV_AUTHOR_URL + std::to_string(picinfo.pixivInfo[0].authorID));
        idURL = QString::fromStdString(PIXIV_BASE_URL + std::to_string(picinfo.pixivIdIndices.begin()->first));
    } else if (picinfo.tweetInfo.size() > 0) { // display tweet info if available
        QString description = QString::fromStdString(picinfo.tweetInfo[0].description).split('\n').first();
        if (description.length() > MAX_TITLE_LENGTH) {
            description = description.left(MAX_TITLE_LENGTH) + "...";
        }
        ui->titleLabel->setText(description);
        ui->illustratorLabel->setText(QString::fromStdString(picinfo.tweetInfo[0].authorNick));
        ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));

        illustratorURL = QString::fromStdString(TWITTER_AUTHOR_URL + picinfo.tweetInfo[0].authorName);
        idURL = QString::fromStdString(TWITTER_BASE_URL + std::to_string(picinfo.tweetInfo[0].tweetID));
    } else { // fallback to file name
        QString filename = QString::fromStdString(picinfo.filePaths.begin()->filename().string());
        if (filename.length() > MAX_TITLE_LENGTH) {
            filename = filename.left(MAX_TITLE_LENGTH) + "...";
        }
        ui->titleLabel->setText(filename);
        ui->idLabel->setText("N/A");
        ui->illustratorLabel->setResponsive(false);
        ui->idLabel->setResponsive(false);

        illustratorURL = "";
        idURL = "";
    }
    switch (searchField) { // highlight search result
    case SearchField::PixivTitle:
        ui->titleLabel->setFont(HIGHLIGHT_FONT);
        break;
    case SearchField::PixivAuthorID:
    case SearchField::PixivAuthorName:
    case SearchField::TweetAuthorNick:
        ui->illustratorLabel->setFont(HIGHLIGHT_FONT);
        break;
    case SearchField::PixivID:
    case SearchField::TweetAuthorID:
    case SearchField::TweetAuthorName:
        ui->idLabel->setFont(HIGHLIGHT_FONT);
        break;
    default:
        break;
    }
}
PictureFrame::~PictureFrame() {
    delete ui;
}
void PictureFrame::setPixmap(const QPixmap& pixmap) {
    ui->imageLabel->setPixmap(pixmap.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}