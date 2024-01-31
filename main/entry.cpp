#include <winSock2.h>
#include "common.h"
#include "jsonserializer.h"
#include "mainwindow.h"
#include "spidercore.h"
#include "spidermain.h"
#include "variantserializer.h"
#include <QApplication>
#include <QtNetwork>
#include "utf8LogHandler.h"
#include "debug_line.h"
class SpiderApplication : public QApplication
{
    Q_OBJECT
public:
    SpiderApplication(int &argc, char **argv) : QApplication(argc, argv)
    {
    }
    void handele_entry2()
    {
        emit signal_entry2();
    }
signals:
    void signal_entry2();
};
extern "C" __declspec(dllexport) void entry1(const char *boot_exe_path, const char *main_dll_path, int argc, char **argv)
{
    qdebug_line();
    SpiderApplication app(argc, argv);
    qInstallMessageHandler(utf8LogHandler);
    qDebug() << "entry1() called." << main_dll_path;
    auto font = app.font();
    qDebug() << font.pointSizeF();
    font.setPointSizeF(12.0);
    app.setFont(font);
    QPixmap pixmap(":/a-hand-with-a-paintbrush-sticks-out-of-the-laptop-screen.png");
    QSplashScreen splash(pixmap);
    splash.show();
    qdebug_line();
    SpiderCore core(splash, boot_exe_path, main_dll_path);
    qdebug_line();
    SpiderMain w;
    qdebug_line();
    QObject::connect(&app, &SpiderApplication::signal_entry2, &w, &SpiderMain::on__signal_entry2);
    qdebug_line();
    w.show();
    qdebug_line();
    splash.finish(&w);
    qdebug_line();
    //w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    //w.activateWindow(); // for Windows
    //w.raise();          // for MacOS
    exit(app.exec());
}
extern "C" __declspec(dllexport) void entry2()
{
    qDebug() << "entry2(begin)";
    ((SpiderApplication *)qApp)->handele_entry2();
#if 0x0
    w.showMaximized();
    w.activateWindow();
    w.raise();
    // w.setWindowState(Qt::WindowNoState | Qt::WindowActive);
    w.setWindowState(Qt::WindowMaximized | Qt::WindowActive);
#endif
}
#include "entry.moc"
