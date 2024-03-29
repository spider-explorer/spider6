//#include "binarysettings.h"
#include "utf8LogHandler.h"
#include <QtCore>
static QMap<QString, QString> g_env;
void dumpJsonProcessBucket(QJsonObject &result, QString bucketRootDir)
{
    // qDebug() << bucketRootDir;
    QString bucketName = QFileInfo(bucketRootDir).fileName();
    QString bucketDir = bucketRootDir + "/bucket";
    // qDebug() << QFileInfo(bucketDir).exists();
    QDir::Filters filters = QDir::Files;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    QDirIterator it(bucketDir, filters, flags);
    while (it.hasNext())
    {
        QString bucketJson = it.next();
        QString app = QFileInfo(bucketJson).fileName();
        app = QFileInfo(app).completeBaseName();
        if (bucketJson.endsWith("/.") || bucketJson.endsWith("/.."))
            continue;
        QFile bucketJsonFile(bucketJson);
        if (bucketJsonFile.open(QIODevice::ReadOnly))
        {
            QJsonDocument jdoc = QJsonDocument::fromJson(bucketJsonFile.readAll());
            // qDebug() << bucketName << app <<
            // jdoc.object()["description"].toString();
            QJsonObject map;
            map["app"] = app;
            map["bucket"] = bucketName;
            map["description"] = jdoc.object()["description"].toString();
            map["version"] = jdoc.object()["version"].toString();
            result[app] = map;
        }
    }
}
QJsonDocument dumpJson()
{
    QString rootDir = g_env["prof"] + "/scoop/buckets";
    // qDebug() << QFileInfo(rootDir).exists();
    // BinarySettings settings(jsonPath, JsonFormat);
    // settings.clear();
    QJsonObject settings;
    QDir::Filters filters = QDir::Dirs;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    QDirIterator it(rootDir, filters, flags);
    while (it.hasNext())
    {
        QString bucketRootDir = it.next();
        if (bucketRootDir.endsWith("/.") || bucketRootDir.endsWith("/.."))
            continue;
        // qDebug() << bucketRootDir;
        dumpJsonProcessBucket(settings, bucketRootDir);
    }
    QJsonDocument jsonDoc(settings);
    // qDebug() << settings.fileName();
    // QTextStream cerr(stderr, QIODevice::WriteOnly);
    // cerr << settings.fileName() << Qt::endl;
    // cerr << jsonDoc.toJson() << Qt::endl;
    return jsonDoc;
}
void dumpLatestVersion(QString appName)
{
    QDateTime now = QDateTime::currentDateTime();
    QString path = QString("tmp-%1.json").arg(now.toString("yy-MM-dd-hh.mm.ss.zzz"));
    QFile file(path);
    file.remove();
    // qDebug() << file.exists();
    QJsonDocument jsonDoc = dumpJson();
#if 0x0
    file.open(QIODevice::ReadOnly);
    QByteArray json = file.readAll();
    file.close();
    file.remove();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json);
#endif
    QString version = jsonDoc.toVariant().toMap()[appName].toMap()["version"].toString();
    QString versionPath = g_env["prof"] + "/scoop/apps/" + appName + "/" + version;
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << version << " " << versionPath << Qt::endl;
}
int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    qInstallMessageHandler(utf8LogHandler);
    // qDebug() << GetConsoleOutputCP();
    // qDebug().noquote() << "ハロー2©";
    g_env["dir"] = qApp->applicationDirPath();
    g_env["temp"] = g_env["dir"] + "/temp";
    g_env["prof"] = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    g_env["docs"] = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    g_env["msys2"] = g_env["prof"] + "/.software/_msys2";
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    g_env["path"] = env.value("PATH");
    // qDebug() << g_env;
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption dumpJsonOpt(QStringList() << "dump-json", "Dump scoop app list to json.");
    parser.addOption(dumpJsonOpt);
    QCommandLineOption latestOpt(QStringList() << "latest", "Print out the latest version string of the app.",
                                 "appName");
    parser.addOption(latestOpt);
    if (!parser.parse(qApp->arguments()))
    {
        parser.showHelp(-1);
        return (-1);
    }
    if (parser.isSet(dumpJsonOpt))
    {
        QJsonDocument jsonDoc = dumpJson();
        QTextStream cerr(stderr, QIODevice::WriteOnly);
        cerr << jsonDoc.toJson() << Qt::endl;
        return 0;
    }
    else if (parser.isSet(latestOpt))
    {
        QString cmdArg = parser.value(latestOpt);
        // qDebug() << cmdArg;
        dumpLatestVersion(cmdArg);
        return 0;
    }
    else
    {
        qDebug() << "Option not specified.";
        parser.showHelp(-1);
        return -1;
    }
    return 0;
}
