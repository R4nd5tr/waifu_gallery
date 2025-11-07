#pragma once

#include "../../service/database.h"
#include "../../service/model.h"
#include <QFrame>
#include <QPixmap>

namespace Ui {
class PictureFrame;
}

class PictureFrame : public QFrame {
    Q_OBJECT
public:
    explicit PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField = SearchField::None);
    ~PictureFrame();
    void setPixmap(const QPixmap& pixmap);

private:
    Ui::PictureFrame* ui;
};
