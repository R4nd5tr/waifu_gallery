#include "clickable_label.h"
#include <QCursor>
#include <filesystem>

ClickableLabel::ClickableLabel(QWidget* parent) : QLabel(parent) {
    setCursor(Qt::PointingHandCursor); // 设置鼠标悬停时的手型光标
    setAttribute(Qt::WA_Hover, true);  // 启用悬停事件
    originalFont_ = font();
    originalPalette_ = palette();
}

ClickableLabel::ClickableLabel(const QString& text, QWidget* parent) : QLabel(text, parent) {
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    originalFont_ = font();
    originalPalette_ = palette();
}

void ClickableLabel::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    if (!responsive) return;
    // 悬停时改变样式（例如加下划线和颜色变化）
    QFont font = this->font();
    font.setUnderline(true);
    setFont(font);
    emit hovered(true); // 发出悬停信号
}

void ClickableLabel::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (!responsive) return;
    // 恢复原始样式
    setFont(originalFont_);
    setPalette(originalPalette_);
    emit hovered(false); // 发出离开信号
}

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
    if (responsive) {
        QPalette pal = palette();
        pal.setColor(QPalette::WindowText, Qt::gray); // 按下时变灰
        setPalette(pal);
    }
    QLabel::mousePressEvent(event);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent* event) {
    if (responsive) {
        if (event->button() == Qt::LeftButton) {
            emit clicked(); // 发出点击信号
        }
        setPalette(originalPalette_); // 恢复原始颜色
    }
    QLabel::mouseReleaseEvent(event);
}
