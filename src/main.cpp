/*#if defined(WIN32) && defined(VS2010) && !defined(QT_NO_DEBUG)
#include <vld.h>
#endif*/

#include <QApplication>

#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <cstring>
#include <QDateTime>
#include <QProcess>

#include "mainwindow.h"
#include "settings.h"
#include "server.h"
#include "audio.h"

#if defined(WIN32) && defined(VS2010)
#include "breakpad/client/windows/handler/exception_handler.h"

using namespace google_breakpad;

static bool callback(const wchar_t *dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded) {
    if (succeeded && QFile::exists("QSanSMTPClient.exe")){
        char *ID = new char[65535];
        memset(ID, 0, sizeof(ID));
        WideCharToMultiByte(CP_ACP, 0, id, -1, ID, wcslen(id), NULL, NULL);
        QProcess *process = new QProcess(qApp);
        QStringList args;
        args << QString(ID) + ".dmp";
        process->start("QSanSMTPClient", args);
        delete[] ID;
        ID = NULL;
    }
    return succeeded;
}

int main(int argc, char *argv[]) {
    ExceptionHandler eh(L"./dmp", NULL, callback, NULL,
                        ExceptionHandler::HANDLER_ALL);
#else
int main(int argc, char *argv[]) {
#endif
	if (argc > 1 && strcmp(argv[1], "-server") == 0)
        new QCoreApplication(argc, argv);
    else
        new QApplication(argc, argv);

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    QDir::setCurrent(qApp->applicationDirPath());
#endif
#endif

#ifdef Q_OS_LINUX
    QDir dir(QString("lua"));
    if (dir.exists() && (dir.exists(QString("config.lua")))) {
        // things look good and use current dir
    } else {
        QDir::setCurrent(qApp->applicationFilePath().replace("games", "share"));
    }
#endif

    // initialize random seed for later use
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&translator);

    Sanguosha = new Engine;
    Config.init();
    qApp->setFont(Config.AppFont);

    if (qApp->arguments().contains("-server")) {
        Server *server = new Server(qApp);
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen())
            printf("Starting successfully\n");
        else
            printf("Starting failed!\n");

        return qApp->exec();
    }

    QFile file("sanguosha.qss");
    QString styleSheet;
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        styleSheet = stream.readAll();
    }
    qApp->setStyleSheet(styleSheet + QString("QToolTip{ border: 0px solid; background: %1; }").arg(Config.ToolTipBackgroundColor.name()));

#ifdef AUDIO_SUPPORT
    Audio::init();
#endif

	MainWindow main_window;
	Sanguosha->setParent(&main_window);
	main_window.show();

    foreach (QString arg, qApp->arguments()) {
        if (arg.startsWith("-connect:")) {
            arg.remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostAddress", arg);

			main_window.startConnection();
            break;
        }
    }

    return qApp->exec();
}

