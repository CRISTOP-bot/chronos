#include "chronos/gui/mainwindow.h"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    chronos::gui::MainWindow w;
    if (argc > 1) w.loadRecording(argv[1]);
    w.show();
    return app.exec();
}
