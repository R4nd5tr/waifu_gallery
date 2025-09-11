#ifndef WORKER_H
#define WORKER_H

#include "../service/database.h"
#include "picture_frame.h"
#include <filesystem>
#include <QObject>
#include <QPixmap>

struct ImageData {
    uint64_t id;
    QPixmap img;
};

class DatabaseWorker : public QObject {
    Q_OBJECT
public:
    explicit DatabaseWorker(const QString &dbFile, QObject *parent = nullptr);
    ~DatabaseWorker();
public slots:
    void scanDirectory(std::filesystem::path directory);
signals:
private:
    PicDatabase database;
};

class LoaderWorker : public QObject {
    Q_OBJECT
public:
    explicit LoaderWorker();
    ~LoaderWorker();
public slots:
    void loadImage(uint64_t id, PictureFrame* picFramePtr, std::filesystem::path filePath);
signals:
    void loadComplete(uint64_t id, PictureFrame* picFramePtr, QPixmap img);
private:
};

#endif