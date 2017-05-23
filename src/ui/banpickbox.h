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

#ifndef _BAN_PICK_BOX_H
#define _BAN_PICK_BOX_H

#include "timedprogressbar.h"
#include "graphicsbox.h"
#include "protocol.h"
#include "graphicsbox.h"
#include <QTimer>

class Button;
class QGraphicsDropShadowEffect;

class GeneralHalf : public QGraphicsObject
{
    Q_OBJECT
    friend class BanPickBox;

signals:
    void clicked();

protected:
    GeneralHalf(QGraphicsObject *parent, const QString &name, int scale, const QString &status);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual QRectF boundingRect() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    inline void setActivate(bool activate)
    {
        this->activate = activate;
    }

private:
    QString cardName, status;
    int Scale;
    bool activate;
};


class BanPickBox : public GraphicsBox
{
    Q_OBJECT

public:
    explicit BanPickBox();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    QRectF boundingRect() const;
    void clear();
    void reply();
    void silence();

public slots:
    void showBox(QStringList self, QStringList opponent, QStringList bans, QStringList selected);
    void StartChoose(const QString &selector);

private:
    QString selector;
    QList<GeneralHalf *> buttons;
    QStringList self, opponent;
    QStringList selected, bans;
    int scale, row;

    QGraphicsProxyWidget *progress_bar_item;
    QSanCommandProgressBar *progress_bar;

    static const int topBlankWidth;
    static const int bottomBlankWidth;
    static const int interval;
};

#endif // _BAN_PICK_BOX_H
