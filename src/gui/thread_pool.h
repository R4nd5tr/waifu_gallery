#pragma once
#include "../service/model.h"
#include <QEvent>
#include <QImage>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

class MainWindow;

struct ImageLoadTask {
    uint64_t id;
    std::unordered_set<std::filesystem::path> filePaths;
};

class ImageLoadCompleteEvent : public QEvent {
public:
    static const QEvent::Type EventType;
    ImageLoadCompleteEvent(uint64_t id, QImage&& img) : QEvent(EventType), id(id), img(std::move(img)) {}
    uint64_t id;
    QImage img;
};

class ImageLoadThreadPool { // TODO: load full image functionality?
public:
    ImageLoadThreadPool(MainWindow* mainWindow, size_t numThreads = std::thread::hardware_concurrency());
    ~ImageLoadThreadPool();

    void loadImage(PicInfo picInfo);
    void clearTasks();

private:
    void stop();
    void workerFunction();
    MainWindow* mainWindow;
    std::vector<std::thread> workers;
    std::queue<ImageLoadTask> taskQueue;
    std::mutex mutex;
    std::condition_variable condVar;
    std::atomic<bool> stopFlag{false};
};