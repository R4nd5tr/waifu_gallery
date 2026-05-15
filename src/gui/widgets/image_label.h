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
#include <QLabel>
#include <QTimer>
#include <QWidget>

constexpr int START_LOAD_PREVIEW_DELAY = 300; // ms to delay before starting to load the preview when hovering over the thumbnail
constexpr int SHOW_PREVIEW_DELAY = 500;       // ms to delay before showing preview when hovering over the thumbnail

class ImageLabel : public QLabel {
    Q_OBJECT
public:
    explicit ImageLabel(QWidget* parent = nullptr) : QLabel(parent) {
        loadPreviewTimer.setSingleShot(true);
        showPreviewTimer.setSingleShot(true);

        connect(&loadPreviewTimer, &QTimer::timeout, this, [this]() { emit loadPreviewSignal(); });
        connect(&showPreviewTimer, &QTimer::timeout, this, [this]() { emit showPreviewSignal(); });
    }

signals:
    void loadPreviewSignal();
    void showPreviewSignal();

protected:
    void enterEvent(QEnterEvent* event) override {
        QLabel::enterEvent(event);

        loadPreviewTimer.start(START_LOAD_PREVIEW_DELAY);
        showPreviewTimer.start(SHOW_PREVIEW_DELAY);
    }
    void leaveEvent(QEvent* event) override {
        QLabel::leaveEvent(event);

        loadPreviewTimer.stop();
        showPreviewTimer.stop();
    }

private:
    QTimer loadPreviewTimer;
    QTimer showPreviewTimer;
};
