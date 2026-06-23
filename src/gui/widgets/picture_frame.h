/*
 * Waifu Gallery - A anime illustration gallery application.
 * Copyright (C) 2025 R4nd5tr <r4nd5tr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "clickable_label.h"
#include "image_label.h"
#include "image_previewer.h"
#include "service/database.h"
#include "service/model.h"
#include <QDesktopServices>
#include <QFrame>
#include <QPixmap>
#include <QUrl>
#include <cstdlib>

class ImageLoader;
enum class SearchField;
enum class LoadType;

namespace Ui {
class PictureFrame;
}

class PictureFrame : public QFrame {
    Q_OBJECT
public:
    explicit PictureFrame(QWidget* parent,
                          const PicItem* picItem,
                          const MetadataItem* metadataItem,
                          ImageLoader& imageLoader,
                          SearchField searchField = SearchField::None);
    ~PictureFrame();

    void displayImage(uint64_t picId, LoadType loadType); // asynchronous loaded image will be displayed through this function

    void reset();
    void updateDisplayItem(const PicItem* newPicItem, const MetadataItem* newMetadataItem, SearchField searchField) {
        picItem = newPicItem;
        metadataItem = newMetadataItem;
        released = false;
        showInfo(searchField);
        showThumbnail();
    }

protected:
    void leaveEvent(QEvent* event) override {
        QFrame::leaveEvent(event);
        hidePreview();
        showPicInfo();
    }
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    Ui::PictureFrame* ui;
    const PicItem* picItem;
    const MetadataItem* metadataItem;
    ImageLoader& imageLoader;
    bool released = true; // whether this frame is released back to the pool
    void connectSignals();

    void showInfo(SearchField searchField) const;
    void showPicInfo() const;             // default
    void showPicInfo(size_t index) const; // show specific pic info

    void showThumbnail() const;

    // shortcuts
    void openFileWithDefaultApp() const;
    void openFileLocation() const;
    void openIllustratorUrl() const;
    void openIdUrl() const;

    // previewer
    size_t previewingIndex = 0;
    bool displayingPreview = false;
    ImagePreviewer previewer;
    void loadPreviewImage() const;
    void displayPreviewImage(size_t index);
    void displayPreviewImage();
    void showPreview();
    void hidePreview();
};
