# Waifu Gallery

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Qt6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![SQLite3](https://img.shields.io/badge/SQLite-3-lightgrey.svg)](https://sqlite.org/)

一个功能强大的二次元插画管理系统，提供高级标签搜索、筛选和排序功能，让图片管理变得轻松简单。

## 项目背景

平时经常浏览 Pixiv、Twitter 等平台，看到喜欢的插画就会保存下来。随着收藏的图片越来越多，管理和查找变得不太方便，很难快速找到想要的图片。因此，我开发了这个管理工具来解决这个问题。

## 项目简介

Waifu Gallery 是基于 Qt6 开发的桌面应用程序，专门为管理二次元插画设计。支持多种图片格式，能够高效管理大量图片，并提供实用的搜索、筛选和排序功能，帮助你更好地整理和浏览插画收藏。

## 主要功能

### 格式支持
- 支持常见图片格式：JPG、PNG、GIF、WebP
- 兼容 [powerful pixiv downloader](https://github.com/xuejianxianzun/PixivBatchDownloader) 下载的图片、抓取结果和元数据
- 兼容 [gallery-dl](https://github.com/mikf/gallery-dl) 下载的 Twitter 图片与元数据

### 搜索功能
- 支持图片标签（开发中）、Pixiv 标签和 Twitter 标签搜索
- 支持多标签组合搜索，包含和排除特定标签
- 提供作者名称、作品标题，推文ID，图片ID等字段的全文搜索

### 筛选与排序
- 可按限制等级、文件类型、分辨率进行筛选
- 支持按文件大小、创建时间、宽高比等条件排序

## 使用指南

1. **导入图片**：点击`文件-导入`按钮，选择包含图片的文件夹
2. **标签搜索**：单击左侧标签栏中的标签可包含该标签，双击则可排除，点击已选中的标签可取消选择
3. **筛选图片**：使用侧边栏的筛选条件和排序选项，快速找到目标图片

## 开发与构建

这个项目还在持续开发中，还没有发布正式版本。如果你想参与开发或自行构建项目，请按照以下步骤操作：

### 环境要求

- 编译器：mingw-w64
- Qt6 框架
- vcpkg 包管理器
- CMake 构建工具

### 获取源码

```bash
git clone https://github.com/R4nd5tr/waifu_gallery.git
cd waifu_gallery
```

### 构建项目

修改 `CMakePresets.json.example` 文件中的路径为你的环境路径，并重命名为 `CMakePresets.json`。

然后在项目根目录下运行以下命令：
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
编译结果将在`/bin`目录下。

使用Qt6的`windeployqt`工具将必要的Qt库复制到输出目录：
```bash
windeployqt <path-to-executable>
```

## 开发计划
- 基于 AI 图像识别的自动标签生成
- 收藏夹功能
- 自定义标签管理
- 标签翻译功能
- 批量操作支持

## 开源协议

本项目采用 **GNU GPL v3** 开源协议，具体内容请参阅 [LICENSE.txt](LICENSE.txt) 文件。

## 相关项目
- [powerful pixiv downloader](https://github.com/xuejianxianzun/PixivBatchDownloader) - Pixiv 图片下载工具
- [gallery-dl](https://github.com/mikf/gallery-dl) - 社交媒体图片下载工具