/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>

#include "version.h"

namespace Ui {
    class MainWindow;
}

class FitView;
class QGraphicsScene;
class QSystemTrayIcon;
class Server;
class QTextEdit;
class QNetworkReply;
class ConnectionDialog;
class ConfigDialog;
class Window;

class BroadcastBox : public QDialog
{
    Q_OBJECT

public:
    BroadcastBox(Server *server, QWidget *parent = 0);

protected:
    virtual void accept();

private:
    Server *server;
    QTextEdit *text_edit;
};

class BackLoader
{
public:
    static void preload();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void fitBackgroundBrush();

#if defined(Q_OS_WIN) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
#endif

    virtual void changeEvent(QEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    void repaintButtons();

    bool isLeftPressDown;
    QPoint movePosition;

    static const int S_PADDING = 4;
    static const int S_CORNER_SIZE = 5;
    enum Direction
    {
        Up, Down, Left, Right, LeftTop, LeftBottom, RightTop, RightBottom, None = -1
    };

    bool isZoomReady;
    Direction direction;

protected:
    virtual void closeEvent(QCloseEvent *event);

private:
    FitView *view;
    QGraphicsScene *scene;
    Ui::MainWindow *ui;
    ConnectionDialog *connection_dialog;
    ConfigDialog *config_dialog;
    QSystemTrayIcon *systray;
    Server *server;
    Window *about_window;
    UpdateInfoStruct updateInfomation;

    QPushButton *minButton;
    QPushButton *maxButton;
    QPushButton *normalButton;
    QPushButton *closeButton;
    QPushButton *menu;

    QNetworkReply *versionInfomationReply;
    QNetworkReply *changeLogReply;

    void restoreFromConfig();
    void region(const QPoint &cursorGlobalPoint);
    void fetchUpdateInformation();
    void roundCorners();

public slots:
    void startConnection();

private slots:
    void on_actionAbout_GPLv3_triggered();
    void on_actionAbout_Lua_triggered();
    //void on_actionAbout_fmod_triggered();
    void on_actionReplay_file_convert_triggered();
    void on_actionPC_Console_Start_triggered();
    void on_actionRecord_analysis_triggered();
    void on_actionCard_editor_triggered();
    void on_actionAcknowledgement_triggered();
    void on_actionBroadcast_triggered();
    void on_actionRule_Summary_triggered();
    void on_actionMinimize_to_system_tray_triggered();
    void on_actionFullscreen_triggered();
    void on_actionReplay_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Us_triggered();
    void on_actionEnable_Hotkey_toggled(bool);
    void on_actionNever_nullify_my_trick_toggled(bool);
    void on_actionCard_Overview_triggered();
    void on_actionGeneral_Overview_triggered();
    void on_actionStart_Server_triggered();
    void on_actionExit_triggered();
    void on_actionCheckUpdate_triggered();

    void checkVersion(const QString &server_version, const QString &server_mod);
    void networkError(const QString &error_msg);
    void enterRoom();
    void gotoScene(QGraphicsScene *scene);
    void gotoStartScene();
    void startGameInAnotherInstance();
    void changeBackground();
    void on_actionManage_Ban_IP_triggered();

    void onVersionInfomationGotten();
    void onChangeLogGotten();

signals:
    void about_to_close();
};

#endif //MAINWINDOW_H

