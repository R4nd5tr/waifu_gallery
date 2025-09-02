#ifndef PARSER_H
#define PARSER_H

#include "model.h"
#include <string>
#include <vector>
#include <cstdint>
#include <QByteArray>

enum class JsonType { Unknown, Pixiv, Tweet };

PicInfo parsePicture(const std::filesystem::path& pictureFilePath);

PixivInfo parsePixivMetadata(const std::filesystem::path& pixivMetadataFilePath);
std::vector<PixivInfo> parsePixivCsv(const std::filesystem::path& pixivCsvFilePath);

QByteArray readJsonFile (const std::filesystem::path& jsonFilePath);
JsonType detectJsonType(const QByteArray& data);
PixivInfo parsePixivJson(const QByteArray& data);
TweetInfo parseTweetJson(const QByteArray& data);

uint64_t calcFileHash(const std::filesystem::path& filePath);

#endif