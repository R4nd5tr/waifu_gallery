#pragma once

#include "model.h"
#include <cstdint>
#include <string>
#include <vector>

enum class ParserType { None, Pixiv, Twitter };

PicInfo parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType = ParserType::None);

PixivInfo parsePixivMetadata(const std::filesystem::path& pixivMetadataFilePath);
std::vector<PixivInfo> parsePixivCsv(const std::filesystem::path& pixivCsvFilePath);
std::vector<PixivInfo> parsePixivJson(const std::filesystem::path& pixivJsonFilePath);

TweetInfo parseTweetJson(const std::filesystem::path& tweetJsonFilePath);
