#pragma once
#include <QDebug>
#include <filesystem>
#include <vector>

std::vector<std::filesystem::path> collectFiles(const std::filesystem::path& directory);