#include "gui/main_window.h"
#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char* argv[]) {
    QLoggingCategory::setFilterRules("qt.gui.imageio=false\n"
                                     "qt.gui.icc=false\n"
                                     "qt.text.font.db=false\n"
                                     "qt.qpa.fonts=false");
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}