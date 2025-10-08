#pragma once
#include "../service/model.h"
#include "main_window.h"
#include <QEvent>
#include <QImage>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

template <typename T>

class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(item);
        condVar.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        condVar.wait(lock, [this]() { return !queue.empty(); });
        T item = queue.front();
        queue.pop();
        return item;
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable condVar;
};

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

class ImageLoadThreadPool {
public:
    ImageLoadThreadPool(MainWindow* mainWindow, size_t numThreads = std::thread::hardware_concurrency());
    ~ImageLoadThreadPool();

    void loadImage(PicInfo picInfo);
    void clearTasks();
    void stop();

private:
    void enqueueTask(const ImageLoadTask& task);
    void workerFunction();
    MainWindow* mainWindow;
    std::vector<std::thread> workers;
    ThreadSafeQueue<ImageLoadTask> taskQueue;
};