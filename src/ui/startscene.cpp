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

#include "startscene.h"
#include "engine.h"
#include "audio.h"
#include "qsanselectableitem.h"
#include "stylehelper.h"
#include "tile.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QNetworkInterface>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QPainter>

StartScene::StartScene(QObject *parent)
    : QGraphicsScene(parent)
{
    // game logo
    QDate date = QDate::currentDate();
    if (date.month() == 8 && date.day() >= 19 && date.day() <= 26) {
        logo = new QSanSelectableItem("image/logo/logo-moxuan.png", true);
        QString tip = "<img src='image/system/developers/moxuan.jpg' height = 125/><br/>";
        tip.append(QString("<font color=%1><b>%2</b></font>").arg(Config.SkillDescriptionInToolTipColor.name()).arg(tr("At 10:40 a.m., August 19, 2014, Moxuanyanyun, a developer of QSanguosha, passed away peacefully in Dalian Medical College. He was 18 and had struggled with leukemia for more than 4 years. May there is no pain in Heaven.")));
        logo->setToolTip(tip);
        shouldMourn = true;
    } else {
        logo = new QSanSelectableItem("image/logo/logo.png", true);
        shouldMourn = false;
    }

    logo->moveBy(-Config.Rect.width() / 4, 0);
    addItem(logo);

    //the website URL
    /*QFont website_font(Config.SmallFont);
    website_font.setStyle(QFont::StyleItalic);
    QGraphicsSimpleTextItem *website_text = addSimpleText("http://qsanguosha.org", website_font);
    website_text->setBrush(Qt::white);
    website_text->setPos(Config.Rect.width() / 2 - website_text->boundingRect().width(),
    Config.Rect.height() / 2 - website_text->boundingRect().height());*/
    serverLog = NULL;

    setBackgroundBrush(QBrush(QPixmap(Config.BackgroundImage)));

    connect(this, &StartScene::sceneRectChanged, this, &StartScene::onSceneRectChanged);
}

void StartScene::addButton(QAction *action)
{
    Tile *button = new Tile(action->text());
    QString icon = action->objectName();
    icon.remove(0, 6);
    button->setIcon(icon.toLower());

    connect(button, &Tile::clicked, action, &QAction::trigger);
    addItem(button);

    QRectF rect = button->boundingRect();
    int n = buttons.length();
    qreal center_x, top_y;
#ifdef Q_OS_IOS
    center_x = Config.Rect.width() / 6;
    top_y = - (1.5 * rect.height()) - (4 * 3);
    if (n < 3)
        button->setPos(center_x - rect.width() - 4, top_y + n * (rect.height() + 8));
    else if (n < 6)
        button->setPos(center_x + 4, top_y + (n - 3) * (rect.height() + 8));
    else
        button->setPos(center_x + 12 + rect.width(), top_y + (n - 6) * (rect.height() + 8));
#else
    center_x = Config.Rect.width() / 4;
    top_y = -(2 * rect.height()) - (4 * 3);
    if (n < 4)
        button->setPos(center_x - rect.width() - 4, top_y + n * (rect.height() + 8));
    else
        button->setPos(center_x + 4, top_y + (n - 4) * (rect.height() + 8));
#endif

    buttons << button;
}

void StartScene::setServerLogBackground()
{
    if (serverLog) {
        // make its background the same as background, looks transparent
        QPalette palette;
        palette.setBrush(QPalette::Base, backgroundBrush());
        serverLog->setPalette(palette);
    }
}

void StartScene::switchToServer(Server *server)
{
#ifdef AUDIO_SUPPORT
    Audio::quit();
#endif
    logo->load("image/logo/logo-server.png");
    logo->setToolTip(QString());
    // performs leaving animation
    QPropertyAnimation *logoShift = new QPropertyAnimation(logo, "pos");
    logoShift->setEndValue(QPointF(Config.Rect.center().rx() - 330, Config.Rect.center().ry() - 215));

    QGraphicsScale *scale = new QGraphicsScale;
    scale->setXScale(1);
    scale->setYScale(1);
    QList<QGraphicsTransform *> transformations;
    transformations << scale;
    logo->setTransformations(transformations);

    QPropertyAnimation *xScale = new QPropertyAnimation(scale, "xScale");
    xScale->setEndValue(0.5);

    QPropertyAnimation *yScale = new QPropertyAnimation(scale, "yScale");
    yScale->setEndValue(0.5);

    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(logoShift);
    group->addAnimation(xScale);
    group->addAnimation(yScale);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    foreach(Tile *button, buttons)
        button->hide();

    serverLog = new QTextEdit;
    serverLog->setReadOnly(true);
    serverLog->resize(700, 420);
    serverLog->move(-400, -180);
    serverLog->setFrameShape(QFrame::NoFrame);

    QFont font = StyleHelper::getFontByFileName("wqy-microhei.ttc");
    font.setPointSize(12);
    serverLog->setFont(font);

    serverLog->setTextColor(Config.TextEditColor);
    setServerLogBackground();
    addWidget(serverLog);

    QScrollBar *bar = serverLog->verticalScrollBar();
    bar->setStyleSheet(StyleHelper::styleSheetOfScrollBar());

    printServerInfo();
    connect(server, &Server::server_message, serverLog, &QTextEdit::append);
    update();
}

void StartScene::showOrganization()
{
#ifdef AUDIO_SUPPORT
    if (shouldMourn)
        Audio::playAudioOfMoxuan();
#endif

    QSanSelectableItem *title = new QSanSelectableItem("image/system/organization.png", true);

    title->setOpacity(0);
    addItem(title);
    title->setPos(0, 0);
    title->setZValue(1000);

    QGraphicsBlurEffect *blur = new QGraphicsBlurEffect;
    blur->setBlurHints(QGraphicsBlurEffect::AnimationHint);
    blur->setBlurRadius(100);
    title->setGraphicsEffect(blur);

    QGraphicsScale *scale = new QGraphicsScale;
    scale->setXScale(0.2);
    scale->setYScale(0.2);
    QList<QGraphicsTransform *> transformations;
    transformations << scale;
    title->setTransformations(transformations);

    QSequentialAnimationGroup *sequentialGroup = new QSequentialAnimationGroup(this);
    QParallelAnimationGroup *parallelGroup = new QParallelAnimationGroup;

    QPropertyAnimation *radius = new QPropertyAnimation(blur, "blurRadius");
    radius->setEndValue(0);
    radius->setDuration(3200);
    radius->setEasingCurve(QEasingCurve::InOutQuad);

    QPropertyAnimation *xScale = new QPropertyAnimation(scale, "xScale");
    xScale->setEndValue(1);
    xScale->setDuration(4000);
    xScale->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation *yScale = new QPropertyAnimation(scale, "yScale");
    yScale->setEndValue(1);
    yScale->setDuration(4000);
    yScale->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(title, "opacity");
    fadeIn->setStartValue(0);
    fadeIn->setEndValue(1);
    fadeIn->setDuration(2000);

    QPropertyAnimation *fadeOut = new QPropertyAnimation(title, "opacity");
    fadeOut->setEndValue(0);
    fadeOut->setDuration(2000);

    parallelGroup->addAnimation(radius);
    parallelGroup->addAnimation(xScale);
    parallelGroup->addAnimation(yScale);
    parallelGroup->addAnimation(fadeIn);
    sequentialGroup->addAnimation(parallelGroup);
    sequentialGroup->addPause(1000);
    sequentialGroup->addAnimation(fadeOut);

    sequentialGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

void StartScene::onSceneRectChanged(const QRectF &rect)
{
    QRectF newRect(rect);
    if (rect.width() < 1024 || rect.height() < 706) {
        qreal sx = 1024 / rect.width();
        qreal sy = 706 / rect.height();
        qreal scale = sx > sy ? sx : sy;
        newRect.setWidth(rect.width() * scale);
        newRect.setHeight(rect.height() * scale);
    }
    newRect.moveTopLeft(newRect.bottomRight() * -0.5);
    disconnect(this, &StartScene::sceneRectChanged, this, &StartScene::onSceneRectChanged);
    setSceneRect(newRect);
    connect(this, &StartScene::sceneRectChanged, this, &StartScene::onSceneRectChanged);
}

static bool isLanAddress(const QString &address)
{
    if (address.startsWith("192.168.") || address.startsWith("10."))
        return true;
    else if (address.startsWith("172.")) {
        bool ok = false;
        int n = address.split(".").value(1).toInt(&ok);
        if (ok && (n >= 16 && n < 32))
            return true;
    }

    return false;
}

void StartScene::printServerInfo()
{
    QStringList items;
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    foreach (const QHostAddress &address, addresses) {
        quint32 ipv4 = address.toIPv4Address();
        if (ipv4)
            items << address.toString();
    }

    items.sort();

    foreach (const QString &item, items) {
        if (isLanAddress(item))
            serverLog->append(tr("Your LAN address: %1, this address is available only for hosts that in the same LAN").arg(item));
        else if (item == "127.0.0.1")
            serverLog->append(tr("Your loopback address %1, this address is available only for your host").arg(item));
        else if (item.startsWith("5.") || item.startsWith("25."))
            serverLog->append(tr("Your Hamachi address: %1, the address is available for users that joined the same Hamachi network").arg(item));
        else if (!item.startsWith("169.254."))
            serverLog->append(tr("Your other address: %1, if this is a public IP, that will be available for all cases").arg(item));
    }

    serverLog->append(tr("Binding port number is %1").arg(Config.ServerPort));
    serverLog->append(tr("Game mode is %1").arg(Sanguosha->getModeName(Config.GameMode)));
    serverLog->append(tr("Player count is %1").arg(Sanguosha->getPlayerCount(Config.GameMode)));
    serverLog->append(Config.OperationNoLimit ?
        tr("There is no time limit") :
        tr("Operation timeout is %1 seconds").arg(Config.OperationTimeout));
    serverLog->append(Config.EnableCheat ? tr("Cheat is enabled") : tr("Cheat is disabled"));
    if (Config.EnableCheat)
        serverLog->append(Config.FreeChoose ? tr("Free choose is enabled") : tr("Free choose is disabled"));

    if (Config.EnableCheat)
        serverLog->append(Config.FreeChoose ? tr("Free Kingdom is enabled") : tr("Free Kingdom is disabled"));

    if (Config.RewardTheFirstShowingPlayer)
        serverLog->append(tr("The reward of showing general first is enabled"));

    if (!Config.ForbidAddingRobot) {
        serverLog->append(tr("This server is AI enabled, AI delay is %1 milliseconds").arg(Config.AIDelay));
    } else {
        serverLog->append(tr("This server is AI disabled"));
    }
}
