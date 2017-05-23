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

#include "dashboard_copy.h"
#include "engine.h"
#include "settings.h"
#include "client.h"
#include "standard.h"
#include "roomscene.h"
#include "graphicspixmaphoveritem.h"
#include "skinbank.h"

#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QTimer>

DashboardCopy::DashboardCopy()
    : width(200), index(0)
{
}

void DashboardCopy::refresh()
{
    foreach (QMappingButton *item, buttons)
        item->update();
}

int DashboardCopy::getWidth()
{
    return this->width;
}

void DashboardCopy::killPlayer(const ClientPlayer *victim)
{
    foreach (QMappingButton *item, buttons) {
        if (item->getPlayer() == victim) {
            disconnect(item, &QMappingButton::clicked, RoomSceneInstance, &RoomScene::dashboardchange);
            item->setState(QMappingButton::S_STATE_DISABLED);
        }
    }
}

void DashboardCopy::revivePlayer(const ClientPlayer *player)
{
    foreach (QMappingButton *item, buttons) {
        if (item->getPlayer() == player) {
            item->setState(QMappingButton::S_STATE_UP);
            connect(item, &QMappingButton::clicked, RoomSceneInstance, &RoomScene::dashboardchange);
        }
    }
}

QRectF DashboardCopy::boundingRect() const
{
    return QRectF(0, 0, width, 30 * index);
}

void DashboardCopy::changePlayer(ClientPlayer *old, ClientPlayer *new_player)
{
    //m_changePlayer.lock();

    foreach (QMappingButton *item, buttons) {
        if (item->getPlayer() == old) {
            item->setEffect(false);
            item->setPlayer(new_player);
        }
    }

    //m_changePlayer.unlock();
}

void DashboardCopy::setPlayer(ClientPlayer *player)
{
    if (!player) return;

    index++;

    QMappingButton *button = new QMappingButton(player, this, index);
    button->setPos(0, - index * 30 - index);
    connect(button, &QMappingButton::clicked, RoomSceneInstance, &RoomScene::dashboardchange);
    buttons << button;
}

const ClientPlayer *DashboardCopy::getPlayerbyIndex(int index)
{
    foreach (QMappingButton *button, buttons)
        if (button->getIndex() == index)
            return button->getPlayer();

    return NULL;
}

void DashboardCopy::noticeActive(const Player *player)
{
    if (player) {
        foreach (QMappingButton *button, buttons)
            if (button->getPlayer() == player)
                button->setEffect(true);
    } else {
        foreach (QMappingButton *button, buttons)
            button->setEffect(false);
    }
}

void DashboardCopy::clear()
{
    foreach (QMappingButton *button, buttons)
        button->deleteLater();
}

void DashboardCopy::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

QMappingButton::QMappingButton(ClientPlayer *player, QGraphicsItem *parent, int index)
    : QGraphicsObject(parent), _m_state(S_STATE_UP), index(index),
    _m_mouseEntered(false), m_isFirstState(true), _m_timerId(0), current(0)
{
    _m_player = player;
    _m_bgPixmap[0] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/normal.png");
    _m_bgPixmap[1] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/havor.png");
    _m_bgPixmap[2] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/disable.png");
    _m_bgPixmap[3] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/flash1.png");

    setSize(QSize(200, 30));

    const IQSanComponentSkin::QSanShadowTextFont &font = G_DASHBOARD_LAYOUT.getSkillTextFont(QSanButton::S_STATE_UP,
                        QSanInvokeSkillButton::S_SKILL_COMPULSORY, QSanInvokeSkillButton::S_WIDTH_WIDE);

    int seat_number = _m_player->getSeat();
    if (_m_player->property("UI_Seat").toInt())
        seat_number = _m_player->property("UI_Seat").toInt();
    QString seat = Sanguosha->translate(QString("SEAT(%1)").arg(QString::number(seat_number)));

    QString name;
    if (_m_player->getGeneral())
        if (_m_player->getGeneral2())
            name = QString("%1/%2").arg(Sanguosha->translate(_m_player->getGeneralName())).arg(Sanguosha->translate(_m_player->getGeneral2Name()));
        else
            name = Sanguosha->translate(_m_player->getGeneralName());
    else
        name = _m_player->screenName();

    QPainter painter1(&_m_bgPixmap[0]);
    font.paintText(&painter1, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));
    QPainter painter2(&_m_bgPixmap[1]);
    font.paintText(&painter2, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));
    QPainter painter3(&_m_bgPixmap[2]);
    font.paintText(&painter3, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(Sanguosha->translate("dead_general")).arg(name));
    QPainter painter4(&_m_bgPixmap[3]);
    font.paintText(&painter4, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void QMappingButton::setPlayer(ClientPlayer *player)
{
    _m_player = player;
    _repaint();
    if (player->isDead()) {
        connect(this, &QMappingButton::clicked, RoomSceneInstance, &RoomScene::dashboardchange);
        setState(QMappingButton::S_STATE_DISABLED);
    }
}

void QMappingButton::timerEvent(QTimerEvent *)
{
    advance(1);
}

void QMappingButton::setEffect(bool active)
{
    if (active) {
        if (_m_timerId > 0) return;
            _m_timerId = startTimer(100);
    } else {
        killTimer(_m_timerId);
        _m_timerId = 0;
        current = 0;
        update();
    }
}

void QMappingButton::advance(int phase)
{
    if (phase) {
        current++;
        update();
    }
}

void QMappingButton::click()
{
    _onMouseClick(true);
}

QRectF QMappingButton::boundingRect() const
{
    return QRectF(0, 0, _m_size.width(), _m_size.height());
}

void QMappingButton::_repaint()
{
    _m_bgPixmap[0] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/normal.png");
    _m_bgPixmap[1] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/havor.png");
    _m_bgPixmap[2] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/disable.png");
    _m_bgPixmap[3] = G_ROOM_SKIN.getPixmapFromFileName("image/system/button/mapping/flash1.png");

    const IQSanComponentSkin::QSanShadowTextFont &font = G_DASHBOARD_LAYOUT.getSkillTextFont(QSanButton::S_STATE_UP,
                        QSanInvokeSkillButton::S_SKILL_COMPULSORY, QSanInvokeSkillButton::S_WIDTH_WIDE);

    int seat_number = _m_player->getSeat();
    if (_m_player->property("UI_Seat").toInt())
        seat_number = _m_player->property("UI_Seat").toInt();
    QString seat = Sanguosha->translate(QString("SEAT(%1)").arg(QString::number(seat_number)));

    QString name;
    if (_m_player->getGeneral())
        if (_m_player->getGeneral2())
            name = QString("%1/%2").arg(Sanguosha->translate(_m_player->getGeneralName())).arg(Sanguosha->translate(_m_player->getGeneral2Name()));
        else
            name = Sanguosha->translate(_m_player->getGeneralName());
    else
        name = _m_player->screenName();

    QPainter painter1(&_m_bgPixmap[0]);
    font.paintText(&painter1, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));
    QPainter painter2(&_m_bgPixmap[1]);
    font.paintText(&painter2, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));
    QPainter painter3(&_m_bgPixmap[2]);
    font.paintText(&painter3, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(Sanguosha->translate("dead_general")).arg(name));
    QPainter painter4(&_m_bgPixmap[3]);
    font.paintText(&painter4, QRect(10, 3, 180, 24), Qt::AlignCenter, QString("%1 %2").arg(seat).arg(name));

    update();
}

void QMappingButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (current > 0) {
        if (current % 2 == 0)
            painter->drawPixmap(0, 0, _m_bgPixmap[3]);
        else
            painter->drawPixmap(0, 0, _m_bgPixmap[1]);
    } else {
        if (_m_state == S_STATE_HOVER)
            painter->drawPixmap(0, 0, _m_bgPixmap[1]);
        else if (_m_state == S_STATE_DISABLED)
            painter->drawPixmap(0, 0, _m_bgPixmap[2]);
        else
            painter->drawPixmap(0, 0, _m_bgPixmap[0]);
    }
}

void QMappingButton::setSize(QSize newSize)
{
    _m_size = newSize;
    if (_m_size.width() == 0 || _m_size.height() == 0) {
        _m_mask = QRegion();
        return;
    }
    Q_ASSERT(!_m_bgPixmap[0].isNull());
    QPixmap pixmap = _m_bgPixmap[0];
    _m_mask = QRegion(pixmap.mask().scaled(newSize));
}

void QMappingButton::setState(QMappingButton::ButtonState state)
{
    if (this->_m_state != state) {
        this->_m_state = state;
        update();
    }
}

bool QMappingButton::insideButton(QPointF pos) const
{
    return _m_mask.contains(QPoint(pos.x(), pos.y()));
}

bool QMappingButton::isMouseInside() const
{
    QGraphicsScene *scenePtr = scene();
    if (NULL == scenePtr) {
        return false;
    }

    QPoint cursorPos = QCursor::pos();
    foreach (QGraphicsView *view, scenePtr->views()) {
        QPointF pos = mapFromScene(view->mapToScene(view->mapFromGlobal(cursorPos)));
        if (insideButton(pos)) {
            return true;
        }
    }

    return false;
}

void QMappingButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QPointF point = event->pos();
    if (_m_mouseEntered || !insideButton(point)) return; // fake event;

    Q_ASSERT(_m_state != S_STATE_HOVER);
    _m_mouseEntered = true;
    if (_m_state == S_STATE_UP)
        setState(S_STATE_HOVER);
}

void QMappingButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    if (!_m_mouseEntered) return;

    Q_ASSERT(_m_state != S_STATE_DISABLED && _m_state != S_STATE_CANPRESHOW);
    if (_m_state == S_STATE_HOVER)
        setState(S_STATE_UP);
    _m_mouseEntered = false;
}

void QMappingButton::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QPointF point = event->pos();
    if (insideButton(point)) {
        if (!_m_mouseEntered) hoverEnterEvent(event);
    } else {
        if (_m_mouseEntered) hoverLeaveEvent(event);
    }
}

void QMappingButton::_onMouseClick(bool inside)
{
    if (inside && _m_player->isAlive()) {
        emit clicked(_m_player);
    }
}

void QMappingButton::mousePressEvent(QGraphicsSceneMouseEvent *)
{
}

void QMappingButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF point = event->pos();
    bool inside = insideButton(point);
    _onMouseClick(inside);
}
