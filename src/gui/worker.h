#pragma once

#include "../service/database.h"
#include "picture_frame.h"
#include <QObject>
#include <QPixmap>
#include <filesystem>

class DatabaseWorker : public QObject { // database operations in another thread
    Q_OBJECT
public:
    explicit DatabaseWorker(QObject* parent = nullptr);
    ~DatabaseWorker();

    void importFilesFromDirectory(std::filesystem::path directory,
                                  ParserType parserType = ParserType::None); // TODO: multithreading!
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
    void reportProgress(int current, int total, double speed);
    void searchComplete(const std::vector<PicInfo>& resultPics,
                        std::vector<std::tuple<std::string, int, bool>> availableTags,
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
    void loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths); // TODO: thread pool
signals:
    void loadComplete(uint64_t id, const QPixmap& img);

private:
};
