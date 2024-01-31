#include <winSock2.h>
//#include "jinstaller.h"
#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <singleapplication.h>
#include "debug_line.h"
#include "jnetwork.h"
#include "junctionmanager.h"
#include "jarchiver.h"
//#include "MemoryModule.h"
//#include "archive_api.h"
#include "utf8LogHandler.h"
void myCallback(void *data, int64_t extractSizeTotal)
{
    //qDebug() << "myCallback() called.";
    QSplashScreen *splash = (QSplashScreen *)data;
    QLocale locale;
    splash->showMessage(
        QString("Spider本体を展開中...%1").arg(locale.formattedDataSize(extractSizeTotal)),
        Qt::AlignLeft, Qt::white);
}
static QString prepareMain(QSplashScreen &splash)
{
    qdebug_line();
    JNetworkManager nm;
    QLocale locale;
    QString spider5Json = nm.getBatchAsText(QUrl("https://raw.githubusercontent.com/spider-explorer/spider5/main/spider.json"));
    qDebug().noquote() << spider5Json;
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spider5Json.trimmed().toUtf8(), &error);
    qDebug() << error.errorString();
    qDebug().noquote() << jsonDoc.toJson();
    QJsonObject object = jsonDoc.object();
    qDebug() << object.keys();
    QString version = object["version"].toString();
    qDebug() << "version=" << version;
    QJsonArray array = object["url"].toArray();
    QString urlString = array[0].toString();
    qDebug() << urlString;
    QString installDir = qApp->applicationDirPath() +
                         QString("/.cache/.spider5/").replace("//", "/");
    QString jsonPath = installDir +
                       QString("/boot-%1.json").arg(version);
    if(version=="")
    {
        QMessageBox::information(nullptr, "確認", "ネット接続が失敗しました");
#ifdef QT_STATIC_BUILD
        QString mainDll = installDir + "/main-x86_64-static.dll";
#else
        QString mainDll = installDir + "/main-x86_64.dll";
#endif
        return mainDll;
    }
    splash.showMessage("Spider本体を準備中...", Qt::AlignLeft, Qt::white);
    QString dlPath = qApp->applicationDirPath() +
                     QString("/.cache/%1.7z").arg(version).replace("//", "/");
    qdebug_line();
    if (!QFileInfo(dlPath).exists())
    {
        qdebug_line();
        qDebug() << nm.getBatchAsFile(
            urlString, dlPath,
            [&splash, &locale](qint64 progress, qint64 size)
        {
            splash.showMessage(
                QString("Spider本体をダウンロード中...%1/%2")
                .arg(locale.formattedDataSize(progress)).arg(locale.formattedDataSize(size)),
                Qt::AlignLeft, Qt::white);
        });
    }
    qdebug_line();
    if(!QFileInfo(dlPath).exists())
    {
        qdebug_line();
        qDebug() << u8"本体のダウンロードが失敗しました";
        QMessageBox::information(nullptr, "確認", "本体のダウンロードが失敗しました");
    }
    qdebug_line();
    if(!QFileInfo(jsonPath).exists())
    {
        QDir(installDir).removeRecursively();
    }
    qdebug_line();
    if (!QFileInfo(installDir).exists())
    {
        qdebug_line();
        qDebug() << extract_archive(dlPath, installDir, [&splash, &locale](qint64 extractSizeTotal)
        {
            splash.showMessage(
                QString("Spider本体を展開中...%1").arg(locale.formattedDataSize(extractSizeTotal)),
                Qt::AlignLeft, Qt::white);
        });
        qdebug_line();
        QFile jsonFile(jsonPath);
        if(jsonFile.open(QIODevice::WriteOnly))
        {
            jsonFile.write("{}");
            jsonFile.close();
        }
        qdebug_line();
    }
    qdebug_line();
#ifdef QT_STATIC_BUILD
    QString mainDll = installDir + "/main-x86_64-static.dll";
#else
    QString mainDll = installDir + "/main-x86_64.dll";
#endif
    qdebug_line();
    return mainDll;
}
int main(int argc, char *argv[])
{
    qDebug() << "main(begin)";
    SingleApplication app(argc, argv, true);
    qInstallMessageHandler(utf8LogHandler);
    if (app.isSecondary())
    {
        qDebug() << "[boot] (app.isSecondary())";
        app.sendMessage("dummy");
        return 0;
    }
    QString mainDll;
    // mainDll = QFileInfo(app.applicationDirPath() + "/main-x86_64-static.dll").absoluteFilePath();
    bool force = (app.arguments().size() >= 2 && app.arguments()[1]=="--force");
    mainDll = app.applicationDirPath() + "/main-x86_64-static.dll";
    // QMessageBox::information(nullptr, "確認", mainDll);
    if (force || !QFileInfo(mainDll).exists())
    {
        QSplashScreen splash(QPixmap(":/splash.png"));
        splash.show();
        mainDll = prepareMain(splash);
        splash.finish(nullptr);
    }
    // exit(0);
    QLibrary lib(mainDll);
    typedef void (*prote_entry1)(const char *boot_exe_path, const char *main_dll_path, int argc, char **argv);
    prote_entry1 entry1 = (prote_entry1)lib.resolve("entry1");
    typedef void (*prote_entry2)();
    prote_entry2 entry2 = (prote_entry2)lib.resolve("entry2");
    QObject::connect(&app, &SingleApplication::receivedMessage,
                     [entry2](quint32 instanceId, QByteArray message)
    {
        qDebug() << "[boot] before calling entry2()";
        entry2();
    });
    QThread *thread =
        QThread::create([entry1, &app, mainDll, argc, argv]
    {
        entry1(app.applicationFilePath().toStdString().c_str(), mainDll.toStdString().c_str(), argc, argv);
    });
    QObject::connect(thread, &QThread::finished, [&app]()
    {
        app.exit(0);
    });
    thread->start();
    return app.exec();
}
