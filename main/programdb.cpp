﻿#include <winsock2.h>
#include "programdb.h"
#include "spidercore.h"
#include "common.h"
static void copyPath(QString src, QString dst)
{
    QDir dir(src);
    if (!dir.exists())
        return;
    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QString dst_path = dst + QDir::separator() + d;
        dir.mkpath(dst_path);
        copyPath(src + QDir::separator() + d, dst_path);
    }
    foreach (QString f, dir.entryList(QDir::Files))
    {
        QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
    }
}
ProgramDB::ProgramDB()
{
    QString prof = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString scoop = prof + "/scoop";
    QString wt_src_dir = qApp->applicationDirPath() + "/wt-1.12.10393.0";
    m_android_studio = QFileInfo(prof + "/scoop/apps/android-studio/current/bin/studio64.exe");
    m_idea = QFileInfo(prof + "/scoop/apps/idea/current/IDE/bin/idea64.exe");
    m_emacs = QFileInfo(prof + "/scoop/apps/emacs/current/bin/emacs.exe");
    //m_vscode = g_core().env()["vscode"];
    //m_lazarus = g_core().env()["lazarus"];
    //m_codelite = g_core().env()["codelite"];
    //m_chrome = prof + "/scoop/apps/googlechrome/current/chrome.exe";
    //m_joinmp4 = g_core().env()["joinmp4"];
}
QString ProgramDB::which(const QString &progName)
{
    QString path = g_core().env()["path"];
    QStringList pathList = path.split(";");
    foreach(QString pathElem, pathList)
    {
        QFileInfo info = QFileInfo(np(pathElem + "/" + progName));
        if(info.exists()) return info.absoluteFilePath();
    }
    return "";
}
QString ProgramDB::which(const QStringList &progNameList)
{
    QString path = g_core().env()["path"];
    QStringList pathList = path.split(";");
    foreach(QString pathElem, pathList)
    {
        foreach(QString progName, progNameList)
        {
            QFileInfo info = QFileInfo(np(pathElem + "/" + progName));
            if(info.exists()) return info.absoluteFilePath();
        }
    }
    return "";
}
#if 0x0
QFileInfo ProgramDB::wt() const
{
    return m_wt;
}
#endif
QFileInfo ProgramDB::android_studio() const
{
    return m_android_studio;
}
QFileInfo ProgramDB::idea() const
{
    return m_idea;
}
QFileInfo ProgramDB::emacs() const
{
    return m_emacs;
}
