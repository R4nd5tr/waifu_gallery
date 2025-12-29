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

#include "about_dialog.h"
#include "ui_about_dialog.h"
#include "utils/version.h"
#include <QFile>

const QString aboutInfo =
    QStringLiteral("<p style=\"white-space: pre-wrap;\">"
                   "一个基于Qt的二次元插画管理系统，让图片管理变得轻松简单。\n\n"
                   "版权所有 © 2025 R4nd5tr\n\n" // 版权/版本
                   "<table>"
                   "<tr><td>作者：</td><td><a href=\"https://github.com/R4nd5tr\">R4nd5tr</a></td></tr>"
                   "<tr><td>邮箱：</td><td><a href=\"mailto:r4nd5tr@outlook.com\">r4nd5tr@outlook.com</a></td></tr>"
                   "<tr><td>GitHub 仓库：</td><td><a "
                   "href=\"https://github.com/R4nd5tr/waifu_gallery\">https://github.com/R4nd5tr/waifu_gallery</a></td></tr>"
                   "</table>"
                   "</p>");

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AboutDialog) {
    ui->setupUi(this);
    ui->titleLabel->setText(QStringLiteral("<b><h2>Waifu Gallery v" WG_VERSION "</h2></b>"));
    ui->logoLabel->setPixmap(QPixmap(":/icons/app.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->aboutLabel->setText(aboutInfo);
    ui->aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->aboutLabel->setOpenExternalLinks(true);
    QString licenseText;
    QFile licenseFile(":/licenses/gpl-3.0-standalone.html");
    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        licenseText = licenseFile.readAll();
        licenseFile.close();
    } else {
        licenseText = QStringLiteral("无法加载许可证文本。");
    }
    ui->licenseBrowser->setHtml(licenseText);
}
AboutDialog::~AboutDialog() {
    delete ui;
}
