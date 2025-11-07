#pragma once
#include <QEnterEvent>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget* parent = nullptr);
    explicit ClickableLabel(const QString& text, QWidget* parent = nullptr);
    void setResponsive(bool enable) {
        responsive = enable;
        if (enable) {
            setCursor(Qt::PointingHandCursor);
            setAttribute(Qt::WA_Hover, true);
        } else {
            setCursor(Qt::ArrowCursor);
            setAttribute(Qt::WA_Hover, false);
        }
    }
signals:
    void clicked();
    void hovered(bool entered);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QFont originalFont_;
    QPalette originalPalette_;
    bool responsive = true;
};
