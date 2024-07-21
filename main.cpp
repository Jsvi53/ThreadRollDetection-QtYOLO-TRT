#include "main.hpp"


int main(int argc, char *argv[])
{   cudaSetDevice(0);

    QApplication a(argc, argv);
    QTranslator translator;     // create translator object
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "YOLO_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    YOLOWINDOW w;

    w.show();
    return a.exec();
}