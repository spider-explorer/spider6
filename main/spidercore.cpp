#include <winsock2.h>
#include "spidercore.h"
#include "cmdprocess.h"
#include "common.h"
#include "msys2dialog.h"
#include "programdb.h"
#include "projectchecker.h"
#include "spiderprocess.h"
#include "windowsutils.h"
#include "wslcore.h"
#include "jnetwork.h"
#include "junctionmanager.h"
#include "jarchiver.h"
//#include "MemoryModule.h"
//#include "archive_api.h"
#include "debug_line.h"
static SpiderCore *s_core = nullptr;
static JsonSettings *s_settings = nullptr;
static QMutex *s_mutex = nullptr;
QString SpiderCore::prepareProgram(const QVariantMap &progEntry)
{
    qdebug_line1("SpiderCore::prepareProgram(1)");
    JNetworkManager nm;
    QString progName = progEntry["name"].toString();
    QString urlString = progEntry["url"].toString();
    QString version = progEntry["version"].toString();
    QString ext = progEntry["ext"].toString();
    QString path = progEntry["path"].toString();
    m_splash.showMessage(QString("%1 を更新中(%2)...").arg(progName).arg(version), Qt::AlignLeft, Qt::white);
    ////QString dlPath = m_env["swRoot"] + QString("/%1/%1-%2.%3").arg(progName).arg(version).arg(ext);
    QString dlPath = m_env["cacheDir"] + QString("/%1-%2.%3").arg(progName).arg(version).arg(ext);
    QLocale locale;
    qdebug_line1("SpiderCore::prepareProgram(2)");
    if (!QFileInfo(dlPath).exists())
    {
        qdebug_line1("SpiderCore::prepareProgram(3)");
        //QString parentPath = QFileInfo(dlPath).absolutePath();
        //QDir(parentPath).removeRecursively();
        qDebug() << nm.getBatchAsFile(urlString, dlPath,
                                      [this, &locale, progName, version](qint64 progress, qint64 size)
        {
            m_splash.showMessage(
                QString("%1 を更新中(%2)...ダウンロード中: %3/%4")
                .arg(progName)
                .arg(version)
                .arg(locale.formattedDataSize(progress)).arg(locale.formattedDataSize(size)),
                Qt::AlignLeft, Qt::white);
        });
    }
    qdebug_line1("SpiderCore::prepareProgram(4)");
    m_splash.showMessage(
        QString("%1 を更新中(%2)...インストール中")
        .arg(progName)
        .arg(version),
        Qt::AlignLeft, Qt::white);
    //QString installDir = m_env["swRoot"] + QString("/%1/%2").arg(progName).arg(version);
    QString installDir = m_env["swRoot"] + QString("/%1").arg(progName);
    QString installDirJson = m_env["swRoot"] + QString("/%1/%1-%2.json").arg(progName).arg(version);
    qdebug_line1("SpiderCore::prepareProgram(5)");
    qdebug_line2("installDir", installDir);
    if (!QFileInfo(installDirJson).exists())
    {
        qdebug_line1("SpiderCore::prepareProgram(6)");
        qdebug_line2("(!QFileInfo(installDir).exists())", installDir);
        QDir(installDir).removeRecursively();
        QDir(installDir).mkpath(".");
        qDebug() << extract_archive(dlPath, installDir,
                                    [this, &locale, progName, version](qint64 extractSizeTotal)
        {
            m_splash.showMessage(
                QString("%1 を更新中(%2)...インストール中: %3")
                .arg(progName)
                .arg(version)
                .arg(locale.formattedDataSize(extractSizeTotal)),
                Qt::AlignLeft, Qt::white);
        });
        QFile f(installDirJson);
        if (f.open(QIODevice::WriteOnly))
        {
            f.write("{}");
            f.close();
        }
        //JunctionManager().remove(junctionDir);
        //JunctionManager().create(junctionDir, installDir);
    }
    qdebug_line1("SpiderCore::prepareProgram(7)");
    //if (!QFileInfo(junctionDir).exists())
    //{
    //    JunctionManager().create(junctionDir, installDir);
    //}
    qdebug_line1("SpiderCore::prepareProgram(8)");
    if(!path.isEmpty())
    {
        qdebug_line1("SpiderCore::prepareProgram(9)");
        QStringList pathList = path.split(";");
        for(int i=0; i<pathList.length(); i++)
        {
            QString pathElem = pathList[i];
            if(pathElem==".")
            {
                //pathElem = junctionDir;
                pathElem = installDir;
            }
            else
            {
                //pathElem = junctionDir + pathElem;
                pathElem = installDir + pathElem;
            }
            pathList[i] = np(pathElem);
        }
        m_env["path"] = pathList.join(";") + ";" + m_env["path"];
    }
    qdebug_line2("SpiderCore::prepareProgram(10)", progName);
    //return junctionDir;
    return installDir;
}
SpiderCore::SpiderCore(QSplashScreen &splash, const QString &bootExePath, const QString &mainDllPath) : m_splash(splash), m_settings("spider")
{
    qdebug_line1("SpiderCore::SpiderCore(1)");
    s_core = this;
    s_settings = &this->m_settings;
    s_mutex = &this->m_mutex;
    //
    QPixmap pixmap(":/one-moment-please.png");
    m_one_moment.setPixmap(pixmap);
    qdebug_line1("SpiderCore::SpiderCore(2)");
    //
    QFileInfo bootExe(bootExePath);
    QFileInfo mainDll(mainDllPath);
    m_env["bootDir"] =
        bootExe.absolutePath().endsWith("/") ?
        bootExe.absolutePath().replace(QRegExp("/$"), "") :
        bootExe.absolutePath();
    m_env["cacheDir"] = m_env["bootDir"] + QString("/.cache");
    QDir(m_env["cacheDir"]).mkpath(".");
    m_env["dir"] = mainDll.absolutePath();
    //m_env["dir"] = mainDll.path();
    m_env["temp"] = m_env["dir"] + "/temp";
    m_env["prof"] = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    //m_env["swRoot"] = m_env["prof"] + "/.software";
    m_env["swRoot"] = m_env["bootDir"] + "/.software";
    m_env["docs"] = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    //m_env["repoRoot"] = m_env["docs"] + "/.repo";
    m_env["repoRoot"] = m_env["bootDir"] + "/.repo";
    QDir(m_env["repoRoot"]).mkpath(".");
    m_env["msys2"] = m_env["swRoot"] + "/_msys2";
    qdebug_line1("SpiderCore::SpiderCore(3)");
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_env["path"] = env.value("PATH");
    //
    // prepareScoop();
    m_env["scoop"] = m_env["prof"] + "/scoop";
    m_env["path"] = np(m_env["scoop"] + "/shims") + ";" + m_env["path"];
    //
    m_env["path"] = np(m_env["dir"]) + ";" + m_env["path"];
    //
    m_env["path"] = np(m_env["dir"] + "/cmd") + ";" + m_env["path"];
    //
    m_env["path"] = np(m_env["dir"] + "/spiderbrowser") + ";" + m_env["path"];
    qdebug_line1("SpiderCore::SpiderCore(4)");
    //
    //QFile file(":/archive-api-x86_64-static.dll");
    //if (file.open(QIODevice::ReadOnly))
    {
        //
        QUrl settingsUrl("https://gitlab.com/javacommons/settings/-/raw/main/spider-settings.json");
        JsonSettings settingsSettings(settingsUrl);
        //
        //QUrl softwareUrl("https://gitlab.com/spider-explorer/spider-software-2/-/raw/main/spider-software.json");
        //QUrl softwareUrl(settingsSettings.value("softwareCatalogUrl").toString());
        QUrl softwareUrl(settingsSettings.value("softwareArrayUrl").toString());
        JsonSettings softwareSettings(softwareUrl);
        qdebug_line1("SpiderCore::SpiderCore(5)");
        ////prepareProgram(softwareSettings, "busybox");
        //QStringList appList = softwareSettings.value("software").toMap().keys();
        QVariantList appList = softwareSettings.value("software").toList();
        QFile software_settings_json(m_env["cacheDir"] + "/software-settings.json");
        //QMessageBox::information(nullptr, "確認2", QString("appList.size()=%1").arg(appList.size()));
        if(appList.size()>0)
        {
            if(software_settings_json.open(QIODevice::WriteOnly))
            {
                software_settings_json.write(QJsonDocument::fromVariant(softwareSettings.object()).toJson());
                software_settings_json.close();
            }
        }
        else
        {
            //QMessageBox::information(nullptr, "確認2", QString("software_settings_json.fileName()=%1").arg(software_settings_json.fileName()));
            JsonSettings localSoftwereSettings(software_settings_json);
            appList = localSoftwereSettings.value("software").toList();
            //QMessageBox::information(nullptr, "確認2", QString("appList.size(2)=%1").arg(appList.size()));
        }
        for(int i=0; i<appList.size(); i++)
        {
            QString progDir = prepareProgram(appList[i].toMap());
#if 0x0
            if (appList[i].toMap()["name"].toString() == "git")
            {
                QProcess gitProc;
                gitProc.setProgram(progDir + "/bin/git.exe");
                gitProc.setArguments(QStringList() << "config"
                                                   << "--system"
                                                   << "core.autocrlf"
                                                   << "input");
                gitProc.start();
                gitProc.waitForFinished();
                gitProc.setArguments(QStringList() << "config"
                                                   << "--system"
                                                   << "credential.helper"
                                                   << "manager-core");
                gitProc.start();
                gitProc.waitForFinished();
            }
#endif
        }
        qdebug_line();
    }
    //
#if 0x0
    QString ubuntuTar = prepareWsl("Ubuntu");
    m_env["ubuntuTar"] = ubuntuTar;
    qDebug() << ubuntuTar << QFileInfo(ubuntuTar).exists();
#endif
    //m_watcher.setTopDir(m_env["repoRoot"]);
}
QSplashScreen &SpiderCore::splash()
{
    return m_splash;
}
QMap<QString, QString> &SpiderCore::env()
{
    return m_env;
}
JsonSettings &SpiderCore::settings()
{
    return m_settings;
}
QString SpiderCore::selectedRepoName()
{
    // MySettings settings;
    QString repo = g_settings().value("selected/repoName").toString();
    QString repoDir = m_env["repoRoot"] + "/" + repo;
    if (!QDir(repoDir).exists())
    {
        g_settings().setValue("selected/repoName", "");
        return "";
    }
    return repo;
}
QString SpiderCore::selectedMsys2Name()
{
    // MySettings settings;
    QString msys2 = g_settings().value("selected/msys2Name").toString();
    QString msys2Dir = m_env["swRoot"] + "/_msys2/" + msys2;
    if (!QDir(msys2Dir).exists())
    {
        g_settings().setValue("selected/msys2Name", "");
        return "";
    }
    return msys2;
}
void SpiderCore::open_nyagos(QWidget *widget, QString path)
{
    if (path.isEmpty())
    {
        path =
            QFileDialog::getExistingDirectory(widget, "nyagosで開くディレクトリを選択してください", m_env["repoRoot"]);
        if (path.isEmpty())
        {
            return;
        }
    }
    auto uhomeName = this->selectedRepoName();
    auto msys2Name = this->selectedMsys2Name();
    m_one_moment.show();
    m_one_moment.showMessage("nyagosを起動中...");
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, path, uhomeName, msys2Name](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("wt.exe"));
            proc->proc()->setArguments(
                QStringList() /*<< "--focus"*/ << R"(nt)"
                                               << "--title"
                                               << QString("(Nyagos) %1 + %2")
                    .arg(uhomeName.isEmpty() ? "" : uhomeName)
                    .arg(msys2Name.isEmpty() ? "(none)" : msys2Name)
                                               << "-d" << path << R"(nyagos.exe)");
            proc->proc()->setWorkingDirectory(path);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            m_one_moment.finish(widget);
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "nyagosを起動しました");
                // WindowsUtils::ShowWindow(proc->proc()->processId());
                // WindowsUtils::RestoreWindow(proc->proc()->processId());
                widget->showMinimized();
            }
            else
            {
                QMessageBox::information(widget, "確認", "nyagosの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::open_bash(QWidget *widget, QString path)
{
    if (path.isEmpty())
    {
        path = QFileDialog::getExistingDirectory(widget, "bashで開くディレクトリを選択してください", m_env["repoRoot"]);
        if (path.isEmpty())
        {
            return;
        }
    }
    auto uhomeName = this->selectedRepoName();
    auto msys2Name = this->selectedMsys2Name();
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, path, uhomeName, msys2Name](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("wt.exe"));
            proc->proc()->setArguments(QStringList() << "nt"
                                                     << "--title"
                                                     << QString("(Bash) %1 + %2")
                                       .arg(uhomeName.isEmpty() ? "" : uhomeName)
                                       .arg(msys2Name.isEmpty() ? "(none)" : msys2Name)
                                                     << "-d" << path
                                       //<< "nyagos.exe" << "-c"
                                       //<< "busybox.exe bash"
                                                     << "busybox.exe" << "bash" << "-l"
                                       );
            proc->proc()->setWorkingDirectory(path);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "bashを起動しました");
                widget->showMinimized();
            }
            else
            {
                QMessageBox::information(widget, "確認", "bashの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::open_file(QWidget *widget, QString path)
{
    QFileInfo info(path);
    qDebug() << info.suffix();
    QFile file(path);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray bytes = file.read(8000);
        bool isBinary = false;
        for (int i = 0; i < bytes.size(); i++)
        {
            if (bytes.at(i) == 0)
            {
                isBinary = true;
                break;
            }
        }
        if (info.suffix() == "sln" || info.suffix() == "csproj")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                QString devenv = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe";
                QString prog;
                QString wd;
                QString target;
                if (QFileInfo(devenv).exists())
                {
                    prog = devenv;
                    wd = QFileInfo(path).absolutePath();
                    target = path;
                }
                else
                {
                    prog = ProgramDB().which("Code.exe");
                    wd = QFileInfo(path).absolutePath();
                    target = wd;
                }
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(prog);
                    proc->proc()->setArguments(QStringList() << target);
                    proc->proc()->setWorkingDirectory(wd);
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget,
                        // "確認", "プロジェクト/ソリューションを開きました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "プロジェクト/ソリューションを開けませんでした");
                    }
                    proc->deleteLater();
                }
            });
            sproc->start();
        }
        else if (info.suffix() == "7z" || info.suffix() == "gz" || info.suffix() == "xz" || info.suffix() == "tar" ||
                 info.suffix() == "zip")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(ProgramDB().which("7zFM.exe"));
                    proc->proc()->setArguments(QStringList() << path);
                    proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget,
                        // "確認", "7zFMを起動しました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "7zFMの起動が失敗しました");
                    }
                    proc->deleteLater();
                }
            });
            sproc->start();
        }
        else if (info.suffix() == "pdf")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(ProgramDB().which("SumatraPDF.exe"));
                    proc->proc()->setArguments(QStringList() << path);
                    proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget,
                        // "確認", "SumatraPDFを起動しました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "SumatraPDFの起動が失敗しました");
                    }
                    proc->deleteLater();
                }
            });
            sproc->start();
        }
        else if (info.suffix() == "c" || info.suffix() == "cpp" || info.suffix() == "cxx" ||
                 info.suffix() == "h" || info.suffix() == "hpp" || info.suffix() == "hxx" ||
                 info.suffix() == "mk" || info.suffix() == "make" ||
                 info.suffix() == "html" || info.suffix() == "htm" ||
                 info.suffix() == "js" || info.suffix() == "mjs")
        {
            this->develop_with_geany(widget, path);
        }
        else if (info.suffix() == "odb" || info.suffix() == "odf" || info.suffix() == "odg" || info.suffix() == "odp" ||
                 info.suffix() == "ods" || info.suffix() == "odt" ||
                 info.suffix() == "doc" || info.suffix() == "xls" || info.suffix() == "ppt" ||
                 info.suffix() == "docx" || info.suffix() == "xlsx" || info.suffix() == "pptx")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(ProgramDB().which("soffice.exe"));
                    proc->proc()->setArguments(QStringList() << path);
                    proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget, "確認",
                        // "LibreOfficeを起動しました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "LibreOfficeの起動が失敗しました");
                    }
                    proc->deleteLater();
                }
            });
            sproc->start();
        }
        else if (info.suffix() == "py" || info.suffix() == "pyw")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(ProgramDB().which(QStringList() << "pycharm.exe" << "pycharm.cmd"));
                    proc->proc()->setArguments(QStringList() << QFileInfo(path).absolutePath());
                    proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget, "確認",
                        // "PyCharmを起動しました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "PyCharmの起動が失敗しました");
                    }
                    //proc->deleteLater();
                }
            });
            //sproc->start();
            sproc->proc()->startDetached();
            sproc->deleteLater();
        }
        else if (info.suffix() == "db3" || info.suffix() == "sqlite")
        {
            SpiderProcess *sproc = new SpiderProcess(
                [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
            {
                if (stage == SpiderProcStage::PROC_SETUP)
                {
                    proc->proc()->setProgram(ProgramDB().which("sqlitestudio.exe"));
                    proc->proc()->setArguments(QStringList() << path);
                    proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
                }
                else if (stage == SpiderProcStage::PROC_FINISH)
                {
                    if (proc->proc()->exitCode() == 0)
                    {
                        // QMessageBox::information(widget, "確認",
                        // "SQLiteStudioを起動しました");
                    }
                    else
                    {
                        QMessageBox::information(widget, "確認", "SQLiteStudioの起動が失敗しました");
                    }
                    //proc->deleteLater();
                }
            });
            //sproc->start();
            sproc->proc()->startDetached();
            sproc->deleteLater();
        }
        else if (info.suffix() == "pro")
        {
            this->develop_with_qtcreator(widget, info.absoluteFilePath());
        }
        else if (info.suffix() == "lpr")
        {
            this->develop_with_lazarus(widget, info.absoluteFilePath());
        }
        else if (info.suffix() == "workspace" || info.suffix() == "project")
        {
            this->develop_with_codelite(widget, info.absoluteFilePath());
        }
        else if (isBinary)
        {
            // QMessageBox::information(widget, "確認", "バイナリファイルです");
            QUrl url = QUrl::fromLocalFile(path);
            if (!QDesktopServices::openUrl(url))
            {
                QMessageBox::information(widget, "確認", "このファイルは開けませんでした");
            }
        }
        else
        {
            // QMessageBox::information(widget, "確認", "テキストファイルです");
            QString notepad3 = ProgramDB().which("notepad3.exe");
            QProcess proc;
            proc.setProgram(notepad3);
            proc.setArguments(QStringList() << path);
            proc.setWorkingDirectory(QFileInfo(path).absolutePath());
            qDebug() << proc.startDetached();
        }
    }
}
void SpiderCore::open_notepad3(QWidget *widget, QString path)
{
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("notepad3.exe"));
            proc->proc()->setArguments(QStringList() << path);
            proc->proc()->setWorkingDirectory(QFileInfo(path).absolutePath());
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "notepad3を起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "notepad3の起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
QMessageBox::StandardButton SpiderCore::check_system_qt_project(QWidget *widget, QString proFile)
{
    QFileInfo userInfo = QFileInfo(proFile + ".user");
    if(userInfo.exists())
    {
        QFile userFile(userInfo.absoluteFilePath());
        if(userFile.open(QIODevice::ReadOnly))
        {
            QByteArray bytes = userFile.readAll();
            if(bytes.contains("(MSYS2)"))
            {
                return QMessageBox::No;
            }
            else
            {
                return QMessageBox::Yes;
            }
        }
    }
    QFileInfo sysQt = QFileInfo(QStringLiteral("C:/Qt/Tools/QtCreator/bin/qtcreator.exe"));
    if(!sysQt.exists()) return QMessageBox::No;
    QMessageBox::StandardButton reply = QMessageBox::question(widget, "確認", QString("%1で開きますか?").arg(sysQt.absoluteFilePath()),
                                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    return reply;
#if 0x0
    if (reply == QMessageBox::Yes)
    {
        return true;
    }
    return false;
#endif
}
void SpiderCore::develop_with_qtcreator(QWidget *widget, QString proFile)
{
    m_one_moment.show();
    m_one_moment.showMessage("QtCreatorを起動中...");
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, proFile /*, useSysQt*/](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("qtcreator.exe"));
            if(proFile.isEmpty())
            {
                proc->proc()->setArguments(QStringList());
            }
            else
            {
                proc->proc()->setArguments(QStringList() << proFile);
            }
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            //m_one_moment.finish(widget);
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認", QString("QtCreatorを起動しました(%1)").arg(proFile));
                //widget->showMinimized();
            }
            else
            {
                QMessageBox::information(widget, "確認", QString("QtCreatorの起動が失敗しました(%1)").arg(proFile));
            }
            proc->deleteLater();
        }
    });
#if 0x0
    sproc->startDetached();
    m_one_moment.finish(widget);
#else
    sproc->start();
    sproc->proc()->waitForStarted();
    m_one_moment.finish(widget);
    widget->showMinimized();
#endif
}
void SpiderCore::develop_with_lazarus(QWidget *widget, QString lprFile)
{
#if 0x0
#endif
}
void SpiderCore::develop_with_codelite(QWidget *widget, QString projFile)
{
#if 0x0
#endif
}
void SpiderCore::develop_with_geany(QWidget *widget, QString path)
{
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, path](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("geany.exe"));
            proc->proc()->setArguments(QStringList() << path);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認", QString("geanyを起動しました(%1)").arg(path));
                widget->showMinimized();
            }
            else
            {
                QMessageBox::information(widget, "確認", QString("geanyの起動が失敗しました(%1)").arg(path));
            }
            proc->deleteLater();
        }
    });
    sproc->startDetached();
}
void SpiderCore::open_qt_dir(QWidget *widget, QString path)
{
    ProjectChecker ck(path);
    if (ck.isQtProject(true))
    {
        QString proFile = ck.getQtProjectFile();
        if (!proFile.isEmpty())
        {
            this->develop_with_qtcreator(widget, proFile);
        }
        else
        {
            // QMessageBox::information(widget, "確認",
            // "Qtプロジェクトがありません");
        }
    }
    else
    {
        QMessageBox::information(widget, "確認", "Qtプロジェクトではありません");
    }
}
void SpiderCore::open_explorer(QWidget *widget, QString repoDir)
{
    auto uhomeName = this->selectedRepoName();
    auto msys2Name = this->selectedMsys2Name();
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, repoDir, uhomeName, msys2Name](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram("explorer.exe");
            proc->proc()->setArguments(QStringList() << np(repoDir));
            proc->proc()->setWorkingDirectory(repoDir);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "explorerを起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "explorerの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::remove_repo(QWidget *widget, QString repoDir)
{
    QString repo = QFileInfo(repoDir).fileName();
    QString repoRoot = QFileInfo(repoDir).absolutePath();
    QDateTime dt = QDateTime::currentDateTime();
    QString tempPath = QDir::tempPath() + "/spider.deleting." + repo + "." + dt.toString("yyyy-MM-dd-hh-mm-ss");
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(widget, "確認", QString("%1を削除しますか?").arg(repo),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }
    QString buffer;
    QTextStream strm(&buffer);
    strm << QString("set -uvx") << Qt::endl;
    strm << QString("set -e") << Qt::endl;
    strm << QString("pwd") << Qt::endl;
    strm << QString("cd %1").arg(repoRoot) << Qt::endl;
    strm << QString("mv %1 %2").arg(repo, tempPath) << Qt::endl;
    strm << QString("cmd.exe /c rmdir /s /q '%1'").arg(np(tempPath)) << Qt::endl;
    strm << Qt::flush;
    QString cmdLines = *strm.string();
    CmdProcess *proc = new CmdProcess(m_env, QString("%1 を削除").arg(repo), cmdLines, ".sh");
    proc->run();
}
void SpiderCore::refresh_repo(QWidget *widget, QString repoDir)
{
    QString repo = QFileInfo(repoDir).fileName();
    QString repoRoot = QFileInfo(repoDir).absolutePath();
    QDateTime dt = QDateTime::currentDateTime();
    QString tempPath = QDir::tempPath() + "/spider.deleting." + repo + "." + dt.toString("yyyy-MM-dd-hh-mm-ss");
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(widget, "確認", QString("%1を削除してクローンしなおしますか?").arg(repo),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }
    QString buffer;
    QTextStream strm(&buffer);
    strm << QString("set -uvx") << Qt::endl;
    strm << QString("set -e") << Qt::endl;
    strm << QString("pwd") << Qt::endl;
    strm << QString("cd %1/%2").arg(g_core().env()["repoRoot"]).arg(repo) << Qt::endl;
    strm << QString("url=`git config --get remote.origin.url`") << Qt::endl;
    strm << QString("cd %1").arg(g_core().env()["repoRoot"]) << Qt::endl;
    strm << QString("mv %1 %2").arg(repo, tempPath) << Qt::endl;
    strm << QString("cmd.exe /c rmdir /s /q '%1'").arg(np(tempPath)) << Qt::endl;
    strm << QString("git clone --recursive $url %1").arg(repo) << Qt::endl;
    strm << Qt::flush;
    QString cmdLines = *strm.string();
    CmdProcess *proc = new CmdProcess(g_core().env(), QString("%1 を再取得").arg(repo), cmdLines, ".sh");
    proc->run();
}
void SpiderCore::open_msys2(QWidget *widget, QString msys2Dir, QString currentDir)
{
    qDebug() << "SpiderCore::open_msys2():" << msys2Dir;
    auto uhomeName = this->selectedRepoName();
    auto msys2Name = QFileInfo(msys2Dir).fileName(); // this->selectedMsys2Name();
    if (currentDir.isEmpty())
    {
        currentDir = m_env["repoRoot"] + "/" + uhomeName;
    }
    SpiderProcess *sproc = new SpiderProcess(
        [widget, currentDir, uhomeName, msys2Name](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->env()->insert("MSYS2", msys2Name);
            proc->proc()->setProgram(ProgramDB().which("wt.exe"));
            proc->proc()->setArguments(QStringList() << "nt"
                                                     << "--title"
                                                     << QString("(Msys2) %1 + %2")
                                       .arg(uhomeName.isEmpty() ? "" : uhomeName)
                                       .arg(msys2Name.isEmpty() ? "(none)" : msys2Name)
                                                     << "-d" << currentDir << "cmd.exe"
                                                     << "/c"
                                                     << "mingw.cmd");
            proc->proc()->setWorkingDirectory(currentDir);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "msys2を起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "msys2の起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::install_msys2(QWidget *widget)
{
}
void SpiderCore::remove_msys2(QWidget *widget, QString name)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(widget, "確認", QString("%1を削除しますか?").arg(name),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }
    QString buffer;
    QTextStream strm(&buffer);
    strm << QString("busybox pwd") << Qt::endl;
    strm << QString("cd /d %1").arg(np(g_core().env()["msys2"])) << Qt::endl;
    strm << QString("move %1 %1.deleting").arg(name) << Qt::endl;
    strm << QString("rmdir /s /q %1.deleting").arg(name) << Qt::endl;
    strm << Qt::flush;
    QString cmdLines = *strm.string();
    CmdProcess *proc = new CmdProcess(g_core().env(), QString("%1 を削除").arg(name), cmdLines, ".cmd");
    proc->run();
}
void SpiderCore::open_git_page(QWidget *widget, QString repoDir)
{
    QProcess proc;
    proc.setProgram(ProgramDB().which("git.exe"));
    proc.setArguments(QStringList() << "config"
                                    << "--get"
                                    << "remote.origin.url");
    proc.setWorkingDirectory(repoDir);
    proc.start();
    proc.waitForFinished();
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(proc.readAll().trimmed())));
}
void SpiderCore::open_vscode(QWidget *widget, QString repoDir)
{
    SpiderProcess *sproc = new SpiderProcess(
        [widget, repoDir](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("Code.exe"));
            proc->proc()->setArguments(QStringList() << repoDir);
            proc->proc()->setWorkingDirectory(repoDir);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "vscodeを起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "vscodeの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::open_emacs(QWidget *widget, QString repoDir)
{
    SpiderProcess *sproc = new SpiderProcess(
        [widget, repoDir](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            //proc->proc()->setProgram(ProgramDB().which("runemacs.exe"));
            //proc->proc()->setArguments(QStringList() << "--debug-init" << repoDir);
            //proc->proc()->setWorkingDirectory(repoDir);
            proc->proc()->setProgram(ProgramDB().which("wt.exe"));
            proc->proc()->setArguments(
                QStringList() /*<< "--focus"*/ << R"(nt)"
                                               << "--title"
                                               << QString("(Emacs) %1").arg(QFileInfo(repoDir).fileName())
                    //<< "-d" << repoDir << R"(nyagos.exe)" << "-c" << "emacs.exe -nw --debug-init ."
                                               << "-d" << repoDir << R"(nyagos.exe)" << "-c" << "runemacs.exe --debug-init ."
                );
            proc->proc()->setWorkingDirectory(repoDir);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "emacsを起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "emacsの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
void SpiderCore::open_smartgit(QWidget *widget, QString repoDir)
{
    m_one_moment.show();
    m_one_moment.showMessage("nyagosを起動中...");
    SpiderProcess *sproc = new SpiderProcess(
        [this, widget, repoDir](SpiderProcStage stage, SpiderProcess *proc)
    {
        if (stage == SpiderProcStage::PROC_SETUP)
        {
            proc->proc()->setProgram(ProgramDB().which("smartgit.exe"));
            proc->proc()->setArguments(QStringList() << "--open" << repoDir);
            proc->proc()->setWorkingDirectory(repoDir);
        }
        else if (stage == SpiderProcStage::PROC_FINISH)
        {
            m_one_moment.finish(widget);
            if (proc->proc()->exitCode() == 0)
            {
                // QMessageBox::information(widget, "確認",
                // "smartgitを起動しました");
            }
            else
            {
                QMessageBox::information(widget, "確認", "smartgitの起動が失敗しました");
            }
            proc->deleteLater();
        }
    });
    sproc->start();
}
SpiderCore &g_core()
{
    return *s_core;
}
JsonSettings &g_settings()
{
    return *s_settings;
}
QMutex &g_mutex()
{
    return *s_mutex;
}
