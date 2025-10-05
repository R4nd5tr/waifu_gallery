# Waifu Gallery

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Qt6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![SQLite3](https://img.shields.io/badge/SQLite-3-lightgrey.svg)](https://sqlite.org/)

一个功能强大的二次元插画管理系统，提供高级标签搜索、筛选和排序功能，让图片管理变得轻松简单。

## 项目背景

平时经常浏览 Pixiv、Twitter 等平台，看到喜欢的插画就会保存下来。随着收藏的图片越来越多，管理和查找变得不太方便，很难快速找到想要的图片。因此，我开发了这个工具来解决这个问题。

## 项目简介

Waifu Gallery 是基于 Qt6 开发的桌面应用程序，专门为管理二次元插画设计。支持多种图片格式，能够高效管理大量图片，并提供实用的搜索、筛选和排序功能，帮助你更好地整理和浏览插画收藏。

## 主要功能

### 格式支持
- 支持常见图片格式：JPG、PNG、GIF、WebP
- 兼容 [powerful pixiv downloader](https://github.com/xuejianxianzun/PixivBatchDownloader) 下载的图片、抓取结果和元数据
- 兼容 [gallery-dl](https://github.com/mikf/gallery-dl) 下载的 Twitter 图片与元数据

> 💡 **实用提示** 💡
> 如何启用下载器的元数据保存功能？
> - 在 [powerful pixiv downloader](https://github.com/xuejianxianzun/PixivBatchDownloader) 中，进入`更多-抓取`，启用`自动导出抓取结果`，设置`抓取结果>0`，`文件格式`选择`JSON`或`CSV`（更推荐使用`JSON`）。或者在`更多-下载`中，勾选`保存作品的元数据`的`插画`选项。
> - 在 [gallery-dl](https://github.com/mikf/gallery-dl) 中，使用`--write-metadata`选项启用元数据保存。

### 搜索功能
- 支持图片标签（开发中）、Pixiv 标签和 Twitter 标签搜索
- 支持多标签组合搜索，包含和排除特定标签
- 提供作者名称、作品标题，推文ID，图片ID等字段的全文搜索（需要下载器元数据支持）

### 筛选与排序
- 可按限制等级、文件类型、分辨率进行筛选
- 支持按文件大小、创建时间、宽高比等条件排序

## 使用指南

1. **导入图片**：  
 - 在菜单栏点击`文件-导入新图片`，选择包含普通图片或非下载器下载图片的文件夹，程序会自动扫描并导入。  
 - 点击`文件-导入Powerful Pixiv Downloader下载的图片`，选择包含该下载器图片或元数据的文件夹，程序会用专用解析器解析文件名和元数据文件，导入图片、标签和作者等信息。
 - 点击`文件-导入gallery-dl下载的Twitter图片`，选择包含gallery-dl下载的Twitter图片或元数据的文件夹，程序同样会用专用解析器导入相关内容。  

本工具支持所有二次元插画，若图片带有下载器元数据，则可按平台标签、作者、作品标题等更多维度进行搜索和筛选。
> ⚠️ **重要提示** ⚠️
> 使用专用解析器导入图片时，请务必确保所选文件夹内**只包含该下载器下载的文件**，不要混入其他文件。否则可能导致程序异常、数据丢失等严重后果。

2. **标签搜索**：单击左侧标签栏中的标签可包含该标签，双击则可排除，点击已选中的标签可取消选择
3. **文本搜索**：选择搜索字段（如作者、标题等）然后在顶部搜索栏输入关键词
4. **筛选图片**：使用侧边栏的筛选条件和排序选项，快速找到目标图片

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
cmake --preset=default
cmake --build --preset=mingw-build
```
编译结果将在`/bin`目录下。

如果你遇到了打开程序直接闪退的问题，可能是缺少某些DLL文件，需要使用`windeployqt`工具复制Qt依赖，如果仍然有问题，请检查是否缺少vcpkg下载的DLL文件，在`build/mingw/vcpkg_installed/x64-mingw-dynamic/bin`文件夹中可以找到这些DLL文件。

使用Qt6的`windeployqt`工具将必要的Qt库复制到输出目录：
```bash
windeployqt <path-to-executable>
```

## 开发计划
- 基于 AI 图像识别的自动标签生成
- 线程池优化
- 图片预览功能
- 收藏夹功能
- 自定义标签管理
- 标签翻译功能
- 批量操作支持

## 开源协议

本项目采用 **GNU GPL v3** 开源协议，具体内容请参阅 [LICENSE.txt](LICENSE.txt) 文件。

## 相关项目
- [powerful pixiv downloader](https://github.com/xuejianxianzun/PixivBatchDownloader) - Pixiv 图片下载工具
- [gallery-dl](https://github.com/mikf/gallery-dl) - 社交媒体图片下载工具