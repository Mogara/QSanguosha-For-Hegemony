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
#ifndef PINDIANBOX_H
#define PINDIANBOX_H

#include "cardcontainer.h"

class PindianBox : public CardContainer
{
    Q_OBJECT

public:
    PindianBox();
    virtual QRectF boundingRect() const;
    virtual void doPindianAnimation(const QString &who);
    void playSuccess(int type, int index);
    inline QString getRequestor()
    {
        return zhuge;
    }

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void doPindian(const QString &requestor, const QString &reason, const QStringList &targets);
    void onReply(const QString &who, int card_id);

private slots:
    void clear();

private:
    QList<CardItem *> upItems, downItems;
    QList<int> card_ids;
    int card_id;
    QString reason;
    int width;
    QString zhuge;
    QStringList targets;
    QMutex _m_mutex_pindian;
};

#endif
