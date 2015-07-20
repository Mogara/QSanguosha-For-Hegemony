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

#include "guanxingbox.h"
#include "roomscene.h"
#include "engine.h"
#include "client.h"
#include "graphicsbox.h"
#include "lua.hpp"
#include <QMessageBox>

GuanxingBox::GuanxingBox()
    : CardContainer()
{
}

void GuanxingBox::doGuanxing(const QList<int> &cardIds, bool up_only)
{
    if (cardIds.isEmpty()) {
        clear();
        return;
    }

    zhuge.clear();//self
    this->up_only = up_only;
    upItems.clear();
    scene_width = RoomSceneInstance->sceneRect().width();

    foreach (int cardId, cardIds) {
        CardItem *cardItem = new CardItem(Sanguosha->getCard(cardId));
        cardItem->setAutoBack(false);
        cardItem->setFlag(QGraphicsItem::ItemIsFocusable);

        connect(cardItem, &CardItem::released, this, &GuanxingBox::onItemReleased);
        connect(cardItem, &CardItem::clicked, this, &GuanxingBox::onItemClicked);

        upItems << cardItem;
        cardItem->setParentItem(this);
    }

    itemCount = upItems.length();
    prepareGeometryChange();
    GraphicsBox::moveToCenter(this);
    show();

    int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    int width = (cardWidth + cardInterval) * upItems.length() - cardInterval + 50;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width())
        width = (cardWidth + cardInterval) * ((upItems.length() + 1) / 2) - cardInterval + 50;

    const int firstRow = itemNumberOfFirstRow();

    for (int i = 0; i < upItems.length(); i++) {
        CardItem *cardItem = upItems.at(i);

        QPointF pos;
        if (i < firstRow) {
            pos.setX(25 + (cardWidth + cardInterval) * i);
            pos.setY(45);
        } else {
            if (upItems.length() % 2 == 1)
                pos.setX(25 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRow));
            else
                pos.setX(25 + (cardWidth + cardInterval) * (i - firstRow));
            pos.setY(45 + cardHeight + cardInterval);
        }

        cardItem->resetTransform();
        cardItem->setOuterGlowEffectEnabled(true);
        cardItem->setPos(25, 45);
        cardItem->setHomePos(pos);
        cardItem->goBack(true);
    }
}

void GuanxingBox::mirrorGuanxingStart(const QString &who, bool up_only, const QList<int> &cards)
{
    doGuanxing(cards, up_only);

    foreach (CardItem *item, upItems) {
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->disconnect(this);
    }

    zhuge = who;
}

void GuanxingBox::mirrorGuanxingMove(int from, int to)
{
    if (from == 0 || to == 0)
        return;

    QList<CardItem *> *fromItems = NULL;
    if (from > 0) {
        fromItems = &upItems;
        from = from - 1;
    } else {
        fromItems = &downItems;
        from = -from - 1;
    }

    if (from < fromItems->length()) {
        CardItem *card = fromItems->at(from);

        QList<CardItem *> *toItems = NULL;
        if (to > 0) {
            toItems = &upItems;
            to = to - 1;
        } else {
            toItems = &downItems;
            to = -to - 1;
        }

        if (to >= 0 && to <= toItems->length()) {
            fromItems->removeOne(card);
            toItems->insert(to, card);
            adjust();
        }
    }
}

void GuanxingBox::onItemReleased()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    int fromPos = 0;
    if (upItems.contains(item)) {
        fromPos = upItems.indexOf(item);
        upItems.removeOne(item);
        fromPos = fromPos + 1;
    } else {
        fromPos = downItems.indexOf(item);
        downItems.removeOne(item);
        fromPos = -fromPos - 1;
    }

    const int count = upItems.length() + downItems.length();
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int middleY = 45 + (isOneRow() ? cardHeight : (cardHeight * 2 + cardInterval));

    bool toUpItems = (up_only || item->y() + cardHeight / 2 <= middleY);
    QList<CardItem *> *items = toUpItems ? &upItems : &downItems;
    bool oddRow = true;
    if (!isOneRow() && count % 2) {
        const qreal y = item->y() + cardHeight / 2;
        if ((y >= 45 + cardHeight && y <= 45 + cardHeight * 2 + cardInterval)
            || y >= 45 + cardHeight * 3 + cardInterval * 3) oddRow = false;
    }
    const int startX = 25 + (oddRow ? 0 : (cardWidth / 2 + cardInterval / 2));
    int c = (item->x() + item->boundingRect().width() / 2 - startX) / cardWidth;
    c = qBound(0, c, items->length());
    items->insert(c, item);

    int toPos = toUpItems ? c + 1 : -c - 1;
    ClientInstance->onPlayerDoGuanxingStep(fromPos, toPos);
    adjust();
}

void GuanxingBox::onItemClicked()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL || up_only) return;

    int fromPos, toPos;
    if (upItems.contains(item)) {
        fromPos = upItems.indexOf(item) + 1;
        toPos = -downItems.size() - 1;
        upItems.removeOne(item);
        downItems.append(item);
    } else {
        fromPos = -downItems.indexOf(item) - 1;
        toPos = upItems.size() + 1;
        downItems.removeOne(item);
        upItems.append(item);
    }

    ClientInstance->onPlayerDoGuanxingStep(fromPos, toPos);
    adjust();
}

void GuanxingBox::adjust()
{
    const int firstRowCount = itemNumberOfFirstRow();
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int count = upItems.length() + downItems.length();

    for (int i = 0; i < upItems.length(); i++) {
        QPointF pos;
        if (i < firstRowCount) {
            pos.setX(25 + (cardWidth + cardInterval) * i);
            pos.setY(45);
        } else {
            if (count % 2 == 1)
                pos.setX(25 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRowCount));
            else
                pos.setX(25 + (cardWidth + cardInterval) * (i - firstRowCount));
            pos.setY(45 + card_height + cardInterval);
        }
        upItems.at(i)->setHomePos(pos);
        upItems.at(i)->goBack(true);
    }

    for (int i = 0; i < downItems.length(); i++) {
        QPointF pos;
        if (i < firstRowCount) {
            pos.setX(25 + (cardWidth + cardInterval) * i);
            pos.setY(45 + (card_height + cardInterval) * (isOneRow() ? 1 : 2));
        } else {
            if (count % 2 == 1)
                pos.setX(25 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRowCount));
            else
                pos.setX(25 + (cardWidth + cardInterval) * (i - firstRowCount));
            pos.setY(45 + card_height * 3 + cardInterval * 3);
        }
        downItems.at(i)->setHomePos(pos);
        downItems.at(i)->goBack(true);
    }
}

int GuanxingBox::itemNumberOfFirstRow() const
{
    const int count = upItems.length() + downItems.length();

    return isOneRow() ? count : (count + 1) / 2;
}

bool GuanxingBox::isOneRow() const
{
    const int count = upItems.length() + downItems.length();

    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    int width = (cardWidth + cardInterval) * count - cardInterval + 50;
    bool oneRow = true;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width()) {
        width = (cardWidth + cardInterval) * (count + 1) / 2 - cardInterval + 50;
        oneRow = false;
    }

    return oneRow;
}

void GuanxingBox::clear()
{
    foreach(CardItem *card_item, upItems)
        card_item->deleteLater();
    foreach(CardItem *card_item, downItems)
        card_item->deleteLater();

    upItems.clear();
    downItems.clear();

    prepareGeometryChange();
    hide();
}

void GuanxingBox::reply()
{
    QList<int> up_cards, down_cards;
    foreach(CardItem *card_item, upItems)
        up_cards << card_item->getCard()->getId();

    foreach(CardItem *card_item, downItems)
        down_cards << card_item->getCard()->getId();

    ClientInstance->onPlayerReplyGuanxing(up_cards, down_cards);
    clear();
}

QRectF GuanxingBox::boundingRect() const
{
    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > (scene_width ? scene_width : 800)) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 50;
        one_row = false;
    }
    int height = (one_row ? 1 : 2) * card_height + (one_row ? 0 : cardInterval);
    if (!up_only) height = height * 2 + cardInterval;
    height += 90;

    return QRectF(0, 0, width, height);
}

void GuanxingBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (zhuge.isEmpty()) {
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("Please arrange the cards"), boundingRect());
    } else {
        QString playerName = ClientInstance->getPlayerName(zhuge);
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("%1 is arranging the cards").arg(playerName), boundingRect());
    }

    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width()) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 50;
        one_row = false;
    }
    const int firstRow = itemNumberOfFirstRow();

    for (int i = 0; i < itemCount; ++i) {
        int x, y = 0;
        if (i < firstRow) {
            x = 25 + (card_width + cardInterval) * i;
            y = 45;
        } else {
            if (itemCount % 2 == 1)
                x = 25 + card_width / 2 + cardInterval / 2
                + (card_width + cardInterval) * (i - firstRow);
            else
                x = 25 + (card_width + cardInterval) * (i - firstRow);
            y = 45 + card_height + cardInterval;
        }
        QRect top_rect(x, y, card_width, card_height);
        painter->drawPixmap(top_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
        if (!up_only) {
            IQSanComponentSkin::QSanSimpleTextFont font = G_COMMON_LAYOUT.m_chooseGeneralBoxDestSeatFont;
            font.paintText(painter, top_rect, Qt::AlignCenter, tr("cards on the top of the pile"));
            QRect bottom_rect(x, y + (card_height + cardInterval) * (one_row ? 1 : 2), card_width, card_height);
            painter->drawPixmap(bottom_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
            font.paintText(painter, bottom_rect, Qt::AlignCenter, tr("cards at the bottom of the pile"));
        }
    }
}

CardChooseBox::CardChooseBox()
    : CardContainer()
{
}

void CardChooseBox::doCardChoose(const QList<int> &upcards, const QList<int> &downcards, const QString &reason, const QString &func, bool moverestricted, int min_num, int max_num)
{
    if (upcards.isEmpty() && downcards.isEmpty()) {
        clear();
        return;
    }

    zhuge.clear();//self
    this->reason = reason;
    this->func = func;
    this->moverestricted = (func == "") ? false : moverestricted;
    buttonstate = ClientInstance->m_isDiscardActionRefusable;
    this->min_num = min_num;
    upItems.clear();
    downItems.clear();
    scene_width = RoomSceneInstance->sceneRect().width();

    foreach (int cardId, upcards) {
        CardItem *cardItem = new CardItem(Sanguosha->getCard(cardId));
        cardItem->setAutoBack(false);
        cardItem->setFlag(QGraphicsItem::ItemIsFocusable);

        connect(cardItem, &CardItem::released, this, &CardChooseBox::onItemReleased);
        connect(cardItem, &CardItem::clicked, this, &CardChooseBox::onItemClicked);

        upItems << cardItem;
        cardItem->setParentItem(this);
    }

    foreach (int cardId, downcards) {
        CardItem *cardItem = new CardItem(Sanguosha->getCard(cardId));
        cardItem->setAutoBack(false);
        cardItem->setFlag(QGraphicsItem::ItemIsFocusable);

        connect(cardItem, &CardItem::released, this, &CardChooseBox::onItemReleased);
        connect(cardItem, &CardItem::clicked, this, &CardChooseBox::onItemClicked);

        downItems << cardItem;
        cardItem->setParentItem(this);
    }

    int down_num = qMax(min_num, max_num);
    itemCount = qMax(upItems.length(), downItems.length());
    downCount = (max_num > 0) ? qMax(down_num, downItems.length()) : itemCount;
    prepareGeometryChange();
    GraphicsBox::moveToCenter(this);
    show();

    int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    int width = (cardWidth + cardInterval) * upItems.length() - cardInterval + 50;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width())
        width = (cardWidth + cardInterval) * ((upItems.length() + 1) / 2) - cardInterval + 50;

    const int firstRow = itemNumberOfFirstRow();

    for (int i = 0; i < upItems.length(); i++) {
        CardItem *cardItem = upItems.at(i);

        QPointF pos;
        int X, Y;
        if (i < firstRow) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = cardWidth / 2 + cardInterval / 2;
            X = (45 + app + (cardWidth + cardInterval) * fix);
            Y = (45);
        } else {
            if (upItems.length() % 2 == 1)
                X = (45 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRow));
            else
                X = (45 + (cardWidth + cardInterval) * (i - firstRow));
            Y = (45 + cardHeight + cardInterval);
        }
        pos.setX(X);
        pos.setY(Y);
        cardItem->resetTransform();
        cardItem->setOuterGlowEffectEnabled(true);
        if (itemCount == 1 || itemCount == 2)
            cardItem->setPos(X, 45);
        else
            cardItem->setPos(25, 45);
        cardItem->setHomePos(pos);
        cardItem->goBack(true);

        if (this->moverestricted){
            QList<int> empty;
            cardItem->setEnabled(check(empty, cardItem->getCard()->getId()));
        }
    }

    for (int i = 0; i < downItems.length(); i++) {
        CardItem *cardItem = downItems.at(i);

        QPointF pos;
        int X, Y;
        if (i < firstRow) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = cardWidth / 2 + cardInterval / 2;
            X = (45 + app + (cardWidth + cardInterval) * fix);
            Y = (45 + cardHeight + cardInterval);
        } else {
            if (upItems.length() % 2 == 1)
                X = (45 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRow));
            else
                X = (45 + (cardWidth + cardInterval) * (i - firstRow));
            Y = (45 + (cardHeight + cardInterval) * 2);
        }
        pos.setX(X);
        pos.setY(Y);
        cardItem->resetTransform();
        cardItem->setOuterGlowEffectEnabled(true);
        if (itemCount == 1 || itemCount == 2)
            cardItem->setPos(X, 45);
        else
            cardItem->setPos(25, 45);
        cardItem->setHomePos(pos);
        cardItem->goBack(true);

        if (this->moverestricted){
            QList<int> empty;
            cardItem->setEnabled(check(empty, cardItem->getCard()->getId()));
        }
    }
}

void CardChooseBox::mirrorCardChooseStart(const QString &who, const QString &reason, const QList<int> &upcards, const QList<int> &downcards,
    const QString &pattern, bool moverestricted, int min_num, int max_num)
{
    doCardChoose(upcards, downcards, reason, pattern, moverestricted, min_num, max_num);

    foreach (CardItem *item, upItems) {
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->disconnect(this);
    }

    foreach (CardItem *item, downItems) {
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->disconnect(this);
    }

    zhuge = who;
}

void CardChooseBox::mirrorCardChooseMove(int from, int to)
{
    if (from == 0 || to == 0)
        return;

    QList<CardItem *> *fromItems = NULL;
    if (from > 0) {
        fromItems = &upItems;
        from = from - 1;
    } else {
        fromItems = &downItems;
        from = -from - 1;
    }

    if (from < fromItems->length()) {
        CardItem *card = fromItems->at(from);

        QList<CardItem *> *toItems = NULL;
        if (to > 0) {
            toItems = &upItems;
            to = to - 1;
        } else {
            toItems = &downItems;
            to = -to - 1;
        }

        if (to >= 0 && to <= toItems->length()) {
            fromItems->removeOne(card);
            toItems->insert(to, card);
            adjust();
        }
    }
}

void CardChooseBox::onItemReleased()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    QList<int> down_cards;
    foreach(CardItem *card_item, downItems)
        down_cards << card_item->getCard()->getId();
    bool check = (func == "") ? true : this->check(down_cards, item->getCard()->getId());

    bool movefromUp = false;
    int fromPos = 0;
    if (upItems.contains(item)) {
        fromPos = upItems.indexOf(item);
        upItems.removeOne(item);
        fromPos = fromPos + 1;
        movefromUp = true;
    } else {
        fromPos = downItems.indexOf(item);
        downItems.removeOne(item);
        fromPos = -fromPos - 1;
    }

    const int count = upItems.length() + downItems.length();
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int middleY = 45 + (isOneRow() ? cardHeight : (cardHeight * 2 + cardInterval));

    bool toUpItems = (item->y() + cardHeight / 2 <= middleY);
    QList<CardItem *> *items = toUpItems ? &upItems : &downItems;
    bool oddRow = true;
    if (!isOneRow() && count % 2) {
        const qreal y = item->y() + cardHeight / 2;
        if ((y >= 45 + cardHeight && y <= 45 + cardHeight * 2 + cardInterval)
            || y >= 45 + cardHeight * 3 + cardInterval * 3) oddRow = false;
    }
    int fix = 0;
    if (itemCount == 1)
        fix = cardWidth + cardInterval;
    if (itemCount == 2)
        fix = cardWidth / 2 + cardInterval / 2;
    const int startX = 45 + (oddRow ? 0 : (cardWidth / 2 + cardInterval / 2));
    int c = (item->x() + item->boundingRect().width() / 2 - startX) / cardWidth;
    c = qBound(0, c, items->length());
    items->insert(c, item);

    int toPos = toUpItems ? c + 1 : -c - 1;

    if (moverestricted && movefromUp && !toUpItems && !check){
        downItems.removeOne(item);
        upItems.append(item);
        toPos = upItems.size();
    }

    if (upItems.length() > itemCount){
        int count = upItems.length();
        QList<CardItem *> readjust;
        for (int i = itemCount; i < count; i++)
            readjust << upItems.at(i);
        foreach (CardItem *item, readjust) {
            upItems.removeOne(item);
            downItems.append(item);
        }
    }

    if (downItems.length() > downCount){
        int count = downItems.length();
        QList<CardItem *> readjust;
        for (int i = downCount; i < count; i++)
            readjust << downItems.at(i);
        foreach (CardItem *item, readjust) {
            downItems.removeOne(item);
            upItems.append(item);
        }
    }

    if (downItems.length() >= min_num) buttonstate = true;

    if (!moverestricted && func != ""){
        down_cards.clear();
        foreach(CardItem *card_item, downItems)
            down_cards << card_item->getCard()->getId();
        buttonstate = this->check(down_cards, -1);
    }else
        buttonstate = (func == "") || !downItems.isEmpty();

    if (downItems.length() < min_num || downItems.length() > downCount) buttonstate = false;

    ClientInstance->onPlayerDoMoveCardsStep(fromPos, toPos, buttonstate);
    adjust();
}

void CardChooseBox::onItemClicked()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    QList<int> down_cards;
    foreach(CardItem *card_item, downItems)
        down_cards << card_item->getCard()->getId();
    bool check = (func == "") ? true : this->check(down_cards, item->getCard()->getId());
    if (moverestricted && upItems.contains(item) && !check) return;

    int fromPos, toPos;
    if (upItems.contains(item)) {
        if (downItems.length() >= downCount) return;
        fromPos = upItems.indexOf(item) + 1;
        toPos = -downItems.size() - 1;
        upItems.removeOne(item);
        downItems.append(item);
    } else {
        if (upItems.length() >= itemCount) return;
        fromPos = -downItems.indexOf(item) - 1;
        toPos = upItems.size() + 1;
        downItems.removeOne(item);
        upItems.append(item);
    }

    if (downItems.length() >= min_num) buttonstate = true;

    if (!moverestricted && func != "") {
        down_cards.clear();
        foreach(CardItem *card_item, downItems)
            down_cards << card_item->getCard()->getId();
        buttonstate = this->check(down_cards, -1);
    }else
        buttonstate = (func == "") || !downItems.isEmpty();

    if (downItems.length() < min_num || downItems.length() > downCount) buttonstate = false;

    ClientInstance->onPlayerDoMoveCardsStep(fromPos, toPos, buttonstate);
    adjust();
}

void CardChooseBox::adjust()
{
    const int firstRowCount = itemNumberOfFirstRow();
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int count = upItems.length() + downItems.length();

    QList<int> down_cards;
    foreach(CardItem *card_item, downItems)
        down_cards << card_item->getCard()->getId();

    for (int i = 0; i < upItems.length(); i++) {
        QPointF pos;
        if (i < firstRowCount) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = cardWidth / 2 + cardInterval / 2;
            pos.setX(45 + app + (cardWidth + cardInterval) * fix);
            pos.setY(45);
        } else {
            if (count % 2 == 1)
                pos.setX(45 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRowCount));
            else
                pos.setX(45 + (cardWidth + cardInterval) * (i - firstRowCount));
            pos.setY(45 + card_height + cardInterval);
        }
        upItems.at(i)->setHomePos(pos);
        upItems.at(i)->goBack(true);

        if (moverestricted)
            upItems.at(i)->setEnabled(check(down_cards, upItems.at(i)->getCard()->getId()));
    }

    for (int i = 0; i < downItems.length(); i++) {
        QPointF pos;
        if (i < firstRowCount) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = cardWidth / 2 + cardInterval / 2;
            pos.setX(45 + app + (cardWidth + cardInterval) * fix);
            pos.setY(45 + (card_height + cardInterval) * (isOneRow() ? 1 : 2));
        } else {
            if (count % 2 == 1)
                pos.setX(45 + cardWidth / 2 + cardInterval / 2
                + (cardWidth + cardInterval) * (i - firstRowCount));
            else
                pos.setX(45 + (cardWidth + cardInterval) * (i - firstRowCount));
            pos.setY(45 + card_height * 3 + cardInterval * 3);
        }
        downItems.at(i)->setHomePos(pos);
        downItems.at(i)->goBack(true);
    }
}

int CardChooseBox::itemNumberOfFirstRow() const
{
    const int count = upItems.length() + downItems.length();

    return isOneRow() ? count : (count + 1) / 2;
}

bool CardChooseBox::isOneRow() const
{
    const int count = upItems.length() + downItems.length();

    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    int width = (cardWidth + cardInterval) * count - cardInterval + 70;
    bool oneRow = true;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width()) {
        width = (cardWidth + cardInterval) * (count + 1) / 2 - cardInterval + 70;
        oneRow = false;
    }

    return oneRow;
}

static void pushQIntList(lua_State *L, const QList<int> &list)
{
    lua_createtable(L, list.length(), 0);

    for (int i = 0; i < list.length(); i++) {
        lua_pushinteger(L, list.at(i));
        lua_rawseti(L, -2, i + 1);
    }
}

bool CardChooseBox::check(const QList<int> &selected, int to_select)
{
    lua_State *l = Sanguosha->getLuaState();
    QString pattern = func;

    lua_getglobal(l, pattern.toLatin1().data());
    pushQIntList(l, selected);
    lua_pushinteger(l, to_select);
    int result = lua_pcall(l, 2, 1, 0);
    if (result) {
        QMessageBox::warning(NULL, "lua_error", lua_tostring(l, -1));
        lua_pop(l, 1);
        return false;
    }
    bool returns = lua_toboolean(l, -1);
    lua_pop(l, 1);

    return returns;
}

void CardChooseBox::clear()
{
    foreach(CardItem *card_item, upItems)
        card_item->deleteLater();
    foreach(CardItem *card_item, downItems)
        card_item->deleteLater();

    upItems.clear();
    downItems.clear();

    prepareGeometryChange();
    hide();
}

void CardChooseBox::reply()
{
    QList<int> up_cards, down_cards;
    foreach(CardItem *card_item, upItems)
        up_cards << card_item->getCard()->getId();

    foreach(CardItem *card_item, downItems)
        down_cards << card_item->getCard()->getId();

    ClientInstance->onPlayerReplyMoveCards(up_cards, down_cards);
    clear();
}

QRectF CardChooseBox::boundingRect() const
{
    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int min = itemCount >= 3 ? itemCount : 3;
    int width = (card_width + cardInterval) * min - cardInterval + 70;
    if (width * 1.5 > (scene_width ? scene_width : 800)) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 70;
        one_row = false;
    }
    int height = (one_row ? 1 : 2) * card_height + (one_row ? 0 : cardInterval);
    bool second_one_row = true;
    if (width * 1.5 > (scene_width ? scene_width : 800)) {
        width = (card_width + cardInterval) * ((downCount + 1) / 2) - cardInterval + 70;
        second_one_row = false;
    }
    height = height + cardInterval + (second_one_row ? 1 : 2) * card_height + (second_one_row ? 0 : cardInterval);
    height += 90;

    return QRectF(0, 0, width, height);
}

void CardChooseBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QString title = QString("%1: %2").arg(Sanguosha->translate(reason)).arg(Sanguosha->translate("@" + reason));
    if (zhuge.isEmpty()) {
        GraphicsBox::paintGraphicsBoxStyle(painter, title, boundingRect());
    } else {
        QString playerName = ClientInstance->getPlayerName(zhuge);
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("%1 is Choosing: %2").arg(playerName).arg(Sanguosha->translate(reason)), boundingRect());
    }

    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 70;
    if (width * 1.5 > RoomSceneInstance->sceneRect().width()) {
        width = (card_width + cardInterval) * ((itemCount + 1) / 2) - cardInterval + 70;
        one_row = false;
    }
    const int firstRow = itemNumberOfFirstRow();

    QString description1 = Sanguosha->translate(reason + "#up");
    QRect up_rect(15, 45, 20, card_height);
    G_COMMON_LAYOUT.playerCardBoxPlaceNameText.paintText(painter, up_rect, Qt::AlignCenter, description1);

    for (int i = 0; i < itemCount; ++i) {
        int x, y = 0;
        if (i < firstRow) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = card_width / 2 + cardInterval / 2;
            x = 45 + app + (card_width + cardInterval) * fix;
            y = 45;
        } else {
            if (itemCount % 2 == 1)
                x = 45 + card_width / 2 + cardInterval / 2
                + (card_width + cardInterval) * (i - firstRow);
            else
                x = 45 + (card_width + cardInterval) * (i - firstRow);
            y = 45 + card_height + cardInterval;
        }

        QRect top_rect(x, y, card_width, card_height);
        painter->drawPixmap(top_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
        IQSanComponentSkin::QSanSimpleTextFont font = G_COMMON_LAYOUT.m_chooseGeneralBoxDestSeatFont;
        font.paintText(painter, top_rect, Qt::AlignCenter, description1);
    }

    QString description2 = Sanguosha->translate(reason + "#down");
    QRect down_rect(15, 45 + (card_height + cardInterval) * (one_row ? 1 : 2), 20, card_height);
    G_COMMON_LAYOUT.playerCardBoxPlaceNameText.paintText(painter, down_rect, Qt::AlignCenter, description2);

    for (int i = 0; i < downCount; ++i) {
        int x, y = 0;
        if (i < firstRow) {
            int fix = i;
            int app = 0;
            if (itemCount == 1) fix = 1;
            if (itemCount == 2) app = card_width / 2 + cardInterval / 2;
            x = 45 + app + (card_width + cardInterval) * fix;
            y = 45;
        } else {
            if (downCount % 2 == 1)
                x = 45 + card_width / 2 + cardInterval / 2
                + (card_width + cardInterval) * (i - firstRow);
            else
                x = 45 + (card_width + cardInterval) * (i - firstRow);
            y = 45 + card_height + cardInterval;
        }
        QRect bottom_rect(x, y + (card_height + cardInterval) * (one_row ? 1 : 2), card_width, card_height);
        painter->drawPixmap(bottom_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
        IQSanComponentSkin::QSanSimpleTextFont font = G_COMMON_LAYOUT.m_chooseGeneralBoxDestSeatFont;
        font.paintText(painter, bottom_rect, Qt::AlignCenter, description2);
    }
}
