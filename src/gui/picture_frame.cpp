#include "picture_frame.h"
#include "ui_picture_frame.h"


PictureFrame::PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField)
    : QFrame(parent), ui(new Ui::PictureFrame) {
    ui->setupUi(this);
    ui->titleLabel->hide();
    ui->resolutionLabel->setText(QString("%1x%2").arg(picinfo.width).arg(picinfo.height));
    ui->fileTypeAndSizeLabel->setText(
        QString("%1 | %2 MB")
            .arg(QString::fromStdString(picinfo.fileType))
            .arg(static_cast<double>(picinfo.size) / (1024 * 1024), 0, 'f', 2)
    );
    switch (searchField) {
    case SearchField::None:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->show();
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
        }
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
        }
        break;
    case SearchField::PixivID:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->show();
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->idLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivUserID:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->show();
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivUserName:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->show();
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivTitle:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->show();
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->titleLabel->setFont(QFont("", 10, QFont::Bold));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
        }
        break;
    case SearchField::TweetUserID:
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorNick)));
            ui->idLabel->setText(QString("tid: %1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::TweetUserName:
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorNick)));
            ui->idLabel->setText(QString("tid: %1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::TweetUserNick:
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorNick)));
            ui->idLabel->setText(QString("tid: %1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
            ui->idLabel->setFont(QFont("", 10, QFont::Bold));
        }
    }
}
PictureFrame::~PictureFrame() {
    delete ui;
}
void PictureFrame::setPixmap(const QPixmap& pixmap) {
    ui->imageLabel->setPixmap(pixmap.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}