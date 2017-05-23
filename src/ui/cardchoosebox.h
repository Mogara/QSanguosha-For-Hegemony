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
#ifndef CARDCHOOSEBOX_H
#define CARDCHOOSEBOX_H

#include "cardcontainer.h"

class CardChooseBox : public CardContainer
{
    Q_OBJECT

public:
    CardChooseBox();
    void reply();
    virtual QRectF boundingRect() const;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    bool check(const QList<int> &selected, int to_select);

public slots:
    void doCardChoose(const QList<int> &upcards, const QList<int> &downcards, const QString &reason, const QString &pattern, bool moverestricted, int min_num, int max_num);
    void clear();

    void mirrorCardChooseStart(const QString &who, const QString &reason, const QList<int> &upcards, const QList<int> &downcards, const QString &pattern, bool moverestricted, int min_num, int max_num);
    void mirrorCardChooseMove(int from, int to);

private slots:
    void onItemReleased();
    void onItemClicked();

private:
    QList<CardItem *> upItems, downItems;
    QString reason;
    QString func;
    int downCount, min_num, up_app1 = 0, up_app2 = 0, down_app1 = 0, down_app2 = 0, width;
    bool moverestricted;
    bool buttonstate;
    bool noneoperator = false;
    void adjust();
    int itemNumberOfFirstRow(bool up) const;
    bool isOneRow(bool up) const;
    QString zhuge;
};


#endif
