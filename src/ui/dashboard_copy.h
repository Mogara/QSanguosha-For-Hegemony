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

#ifndef _DASHBOARD_COPY_H
#define _DASHBOARD_COPY_H

#include "player.h"

#include <QGraphicsObject>
#include <QPixmap>
#include <QRegion>
#include <QRect>
#include <QMutex>

class GraphicsPixmapHoverItem;
class QMappingButton;

class DashboardCopy : public QGraphicsObject
{
    Q_OBJECT

public:
    DashboardCopy();

    virtual QRectF boundingRect() const;
    void refresh();
    virtual void killPlayer(const ClientPlayer *victim);
    virtual void revivePlayer(const ClientPlayer *who);

    int getTextureWidth() const;

    int getWidth();
    int height();

    static const int S_PENDING_OFFSET_Y = -25;

    void setPlayer(ClientPlayer *player);
    void changePlayer(ClientPlayer *old, ClientPlayer *new_player);
    const ClientPlayer *getPlayerbyIndex(int index);
    void noticeActive(const Player *player);
    void clear();

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QList<QMappingButton *> buttons;
    int width;
    int index;
    //QMutex m_changePlayer;

private:
    static const int CARDITEM_Z_DATA_KEY = 0413;

};

class QMappingButton : public QGraphicsObject
{
    Q_OBJECT

public:
    QMappingButton(ClientPlayer *player, QGraphicsItem *parent, int index);
    enum ButtonState
    {
        S_STATE_UP, S_STATE_HOVER, S_STATE_DOWN, S_STATE_CANPRESHOW,
        S_STATE_DISABLED, S_NUM_BUTTON_STATES
    };
    void setSize(QSize size);
    virtual void setState(ButtonState state);
    inline ButtonState getState() const
    {
        return _m_state;
    }
    virtual QRectF boundingRect() const;
    bool insideButton(QPointF pos) const;
    bool isMouseInside() const;

    void setFirstState(bool isFirstState);
    void setPlayer(ClientPlayer *player);
    void setEffect(bool active);
    void timerEvent(QTimerEvent *event);
    void advance(int phase);
    inline ClientPlayer *getPlayer()
    {
        return _m_player;
    }
    inline int getIndex() const
    {
        return index;
    }

public slots:
    void click();

protected:
    virtual void _repaint();
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void _onMouseClick(bool inside);
    ButtonState _m_state;
    ClientPlayer *_m_player;
    QRegion _m_mask;
    QSize _m_size;
    // @todo: currently this is an extremely dirty hack. Refactor the button states to
    // get rid of it.
    bool _m_mouseEntered;
    QPixmap _m_bgPixmap[4];
    //this property is designed for trust button.
    bool m_isFirstState;
    int index;
    int _m_timerId, current;

signals:
    void clicked(ClientPlayer *player);

};
#endif

