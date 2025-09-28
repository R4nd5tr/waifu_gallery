#ifndef WORKER_H
#define WORKER_H

#include "../service/database.h"
#include "picture_frame.h"
#include <filesystem>
#include <QObject>
#include <QPixmap>

class DatabaseWorker : public QObject { //TODO: implement
    Q_OBJECT
public:
    explicit DatabaseWorker(const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseWorker();
public slots:
    void scanDirectory(std::filesystem::path directory);
    void searchPics(const std::unordered_set<std::string>& includedTags,
                    const std::unordered_set<std::string>& excludedTags,
                    const std::unordered_set<std::string>& includedPixivTags,
                    const std::unordered_set<std::string>& excludedPixivTags,
                    const std::unordered_set<std::string>& includedTweetTags,
                    const std::unordered_set<std::string>& excludedTweetTags,
                    const std::string& searchText,
                    SearchField searchField,
                    bool selectedTagChanged,
                    bool selectedPixivTagChanged,
                    bool selectedTweetTagChanged,
                    bool searchTextChanged,
                    size_t requestId);
signals:
    void scanComplete();
    void searchComplete(const std::vector<PicInfo>& resultPics,
        std::vector<std::pair<std::string, int>> availableTags,
        std::vector<std::pair<std::string, int>> availablePixivTags,
        std::vector<std::pair<std::string, int>> availableTwitterHashtags,
        size_t requestId);
private:
    PicDatabase database;

    std::vector<uint64_t> lastTagSearchResult;
    std::vector<uint64_t> lastPixivTagSearchResult;
    std::vector<uint64_t> lastTweetTagSearchResult;
    std::vector<uint64_t> lastTextSearchResult;
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