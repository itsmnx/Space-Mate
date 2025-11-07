#include "mainwindow.h"
#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Register custom types for Qt signal/slot system
    qRegisterMetaType<std::vector<std::pair<std::string, long long>>>("std::vector<std::pair<std::string,long long>>");
    
    // Set application style
    app.setStyle("Fusion");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}