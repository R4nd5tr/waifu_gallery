#ifndef WORKER_H
#define WORKER_H

#include "../service/database.h"
#include "picture_frame.h"
#include <filesystem>
#include <QObject>
#include <QPixmap>

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
    explicit LoaderWorker(QObject* parent = nullptr);
    ~LoaderWorker();
public slots:
    void loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths);
signals:
    void loadComplete(uint64_t id, const QPixmap& img);
private:
};

#endif