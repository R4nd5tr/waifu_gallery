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

// TODO: click resolution label to open image in default viewer,
//       click file type and size label to open file location
//       click illustrator label to open illustrator's pixiv page
//       click id label to open image's pixiv/tweet page

PictureFrame::PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField)
    : QFrame(parent), ui(new Ui::PictureFrame) {
    ui->setupUi(this);
    ui->resolutionLabel->setText(QString("%1x%2").arg(picinfo.width).arg(picinfo.height));
    ui->fileTypeAndSizeLabel->setText(QString("%1 | %2 MB")
                                          .arg(getFileTypeStr(picinfo.fileType))
                                          .arg(static_cast<double>(picinfo.size) / (1024 * 1024), 0, 'f', 2));
    switch (searchField) { // highlight search result
    case SearchField::PixivID:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivInfo[0].pixivID)));
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->idLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivAuthorID:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivInfo[0].pixivID)));
            ui->titleLabel->setText(QString::number(picinfo.pixivInfo[0].authorID));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivAuthorName:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivInfo[0].pixivID)));
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::PixivTitle:
        if (picinfo.pixivInfo.size() > 0) {
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->titleLabel->setFont(QFont("", 10, QFont::Bold));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
        }
        break;
    case SearchField::TweetAuthorID:
        if (picinfo.tweetInfo.size() > 0) {
            ui->titleLabel->setText(QString::number(picinfo.tweetInfo[0].authorID));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.tweetInfo[0].authorNick));
            ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
            ui->idLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::TweetAuthorName:
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.tweetInfo[0].authorNick));
            ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
            ui->idLabel->setFont(QFont("", 10, QFont::Bold));
        }
        break;
    case SearchField::TweetAuthorNick:
        if (picinfo.tweetInfo.size() > 0) {
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.tweetInfo[0].authorNick));
            ui->illustratorLabel->setFont(QFont("", 10, QFont::Bold));
            ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
        }
        break;
    default:
        if (picinfo.pixivInfo.size() > 0) {
            ui->titleLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].title));
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.pixivInfo[0].authorName));
            ui->idLabel->setText(QString("pid: %1").arg(QString::number(picinfo.pixivIdIndices.begin()->first)));
        } else if (picinfo.tweetInfo.size() > 0) {
            QString description = QString::fromStdString(picinfo.tweetInfo[0].description).split('\n').first();
            if (description.length() > MAX_TITLE_LENGTH) {
                description = description.left(MAX_TITLE_LENGTH) + "...";
            }
            ui->titleLabel->setText(description);
            ui->illustratorLabel->setText(QString::fromStdString(picinfo.tweetInfo[0].authorNick));
            ui->idLabel->setText(QString("@%1").arg(QString::fromStdString(picinfo.tweetInfo[0].authorName)));
        } else {
            QString filename = QString::fromStdString(picinfo.filePaths.begin()->filename().string());
            if (filename.length() > MAX_TITLE_LENGTH) {
                filename = filename.left(MAX_TITLE_LENGTH) + "...";
            }
            ui->titleLabel->setText(filename);
            ui->idLabel->setText("N/A");
        }
        break;
    }
}
PictureFrame::~PictureFrame() {
    delete ui;
}
void PictureFrame::setPixmap(const QPixmap& pixmap) {
    ui->imageLabel->setPixmap(pixmap.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}