#ifndef PARSER_H
#define PARSER_H

#include "model.h"
#include <QByteArray>
#include <cstdint>
#include <string>
#include <vector>

enum class ParserType { None, Pixiv, Twitter };

PicInfo parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType = ParserType::None);

PixivInfo parsePixivMetadata(const std::filesystem::path& pixivMetadataFilePath);
std::vector<PixivInfo> parsePixivCsv(const std::filesystem::path& pixivCsvFilePath);

QByteArray readJsonFile(const std::filesystem::path& jsonFilePath);
std::vector<PixivInfo> parsePixivJson(const QByteArray& data);
TweetInfo parseTweetJson(const QByteArray& data);

uint64_t calcFileHash(const std::filesystem::path& filePath);

#endif