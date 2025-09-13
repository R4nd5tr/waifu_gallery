#include "../service/database.h"
#include "worker.h"
#include <QImageReader>

DatabaseWorker::DatabaseWorker(const QString &dbFile, QObject *parent) {
    database = PicDatabase(dbFile);
}
DatabaseWorker::~DatabaseWorker() {}

LoaderWorker::LoaderWorker(QObject* parent) : QObject(parent) {}
LoaderWorker::~LoaderWorker() {}
void LoaderWorker::loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths) {
    QString filePathStr;
    for (const auto& filePath : filePaths) {
        filePathStr = QString::fromUtf8(filePath.u8string().c_str());
        if (!std::filesystem::exists(filePath)) {
            qWarning() << "File does not exist:" << filePathStr;
            continue;
        }
        break;
    }
    QImageReader reader(filePathStr);
    if (!reader.canRead()) {
        qWarning() << "Cannot read image format:" << filePathStr;
        return;
    }
    reader.setAutoTransform(true);

    QImage img;
    QSize originalSize = reader.size();
    if (!originalSize.isValid()) originalSize = QSize(1, 1);

    int maxWidth = 232;
    int maxHeight = 218;

    QSize scaledSize = originalSize;
    scaledSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);

    reader.setScaledSize(scaledSize);
    img = reader.read();

    if (!img.isNull()) {
        emit loadComplete(id, QPixmap::fromImage(img));
    } else {
        qWarning() << "Failed to load image from" << filePathStr;
    }
}