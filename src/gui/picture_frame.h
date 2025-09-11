#ifndef PICTURE_FRAME_H
#define PICTURE_FRAME_H

#include "../service/model.h"
#include "../service/database.h"
#include <QFrame>
#include <QPixmap>

namespace Ui {
    class PictureFrame;
}

class PictureFrame : public QFrame {
    Q_OBJECT
public:
    explicit PictureFrame(QWidget* parent, const PicInfo& picinfo, SearchField searchField=SearchField::None);
    ~PictureFrame();
    void setPixmap(const QPixmap& pixmap);
private:
    Ui::PictureFrame* ui;
};


#endif