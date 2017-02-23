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

#include "choosegeneralbox.h"
#include "engine.h"
#include "skinbank.h"
#include "freechoosedialog.h"
#include "banpair.h"
#include "button.h"
#include "client.h"
#include "clientplayer.h"
#include "cardcontainer.h"
#include "settings.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QPushButton>

using namespace QSanProtocol;

GeneralCardItem::GeneralCardItem(const QString &generalName, const int skinId)
    : CardItem(generalName), hasCompanion(false)
{
    _skinId = skinId;
    setAcceptHoverEvents(true);

    const General *general = Sanguosha->getGeneral(generalName);
    Q_ASSERT(general);

    setOuterGlowEffectEnabled(true);
    setOuterGlowColor(Sanguosha->getKingdomColor(general->getKingdom()));
#ifdef Q_OS_ANDROID
    moveRange = 1.0;
#endif
}

void GeneralCardItem::changeGeneral(const QString &generalName)
{
    CardItem::changeGeneral(generalName);

    const General *general = Sanguosha->getGeneral(generalName);
    Q_ASSERT(general);
    setOuterGlowColor(Sanguosha->getKingdomColor(general->getKingdom()));
}

void GeneralCardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    QRect rect = G_COMMON_LAYOUT.m_cardMainArea;
    if (frozen || !isEnabled()) {
        painter->fillRect(rect, QColor(0, 0, 0));
        painter->setOpacity(0.4 * opacity());
    }

    if (!_m_isUnknownGeneral)
        painter->drawPixmap(rect, G_ROOM_SKIN.getGeneralCardPixmap(objectName(), _skinId));
    else
        painter->drawPixmap(rect, G_ROOM_SKIN.getPixmap("generalCardBack"));

    if (!hasCompanion) return;

    QString kingdom = Sanguosha->getGeneral(objectName())->getKingdom();
    QPixmap icon = G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_GENERAL_CARD_ITEM_COMPANION_ICON, kingdom);
    painter->drawPixmap(boundingRect().center().x() - icon.width() / 2 + 3, boundingRect().bottom() - icon.height(), icon);

    painter->drawPixmap(G_COMMON_LAYOUT.m_generalCardItemCompanionPromptRegion, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_GENERAL_CARD_ITEM_COMPANION_FONT, kingdom));
}

void GeneralCardItem::showCompanion()
{
    if (hasCompanion) return;
    hasCompanion = true;
    update();
}

void GeneralCardItem::hideCompanion()
{
    if (!hasCompanion) return;
    hasCompanion = false;
    update();
}

#ifdef Q_OS_ANDROID
void GeneralCardItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    pressPos = event->pos();
    timerLongPress.setInterval(1000);
    timerLongPress.setSingleShot(true);
    outOfRange = false;
    timerLongPress.start();
}

void GeneralCardItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    qreal curX = event->pos().x(), curY = event->pos().y();
    qreal pressX = pressPos.x(), pressY = pressPos.y();

    if (curX < pressX - moveRange || curX > pressX + moveRange ||
        curY < pressY - moveRange || curY > pressY + moveRange)
        outOfRange = true;
    CardItem::mouseMoveEvent(event);
}

#endif

void GeneralCardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
#ifdef Q_OS_ANDROID
    qreal releaseX = event->pos().x(), releaseY = event->pos().y();
    qreal pressX = pressPos.x(), pressY = pressPos.y();

    if (ServerInfo.FreeChoose && !outOfRange && !timerLongPress.isActive() && releaseX >= pressX - moveRange
        && releaseX <= pressX + moveRange && releaseY >= pressY - moveRange && releaseY <= pressY + moveRange) {
#else
    if (ServerInfo.FreeChoose && Qt::RightButton == event->button()) {
#endif
        FreeChooseDialog *general_changer = new FreeChooseDialog(QApplication::focusWidget());
        connect(general_changer, &FreeChooseDialog::general_chosen, this, &GeneralCardItem::changeGeneral);
        general_changer->exec();
        general_changer->deleteLater();
        return;
    }
    CardItem::mouseReleaseEvent(event);
}

ChooseGeneralBox::ChooseGeneralBox()
    : general_number(0), single_result(false), m_viewOnly(false),
    confirm(new Button(tr("fight"), 0.6)),
    progress_bar(NULL)
{
    confirm->setEnabled(ClientInstance->getReplayer());
    confirm->setParentItem(this);
    connect(confirm, &Button::clicked, this, &ChooseGeneralBox::reply);
}

void ChooseGeneralBox::paintLayout(QPainter *painter)
{
    //============================================================
    //||========================================================||
    //||      Please select the same nationality generals       ||
    //||       ______   ______   ______   ______   ______       ||
    //||      |      | |      | |      | |      | |      |      ||
    //||      |  g1  | |  g2  | |  g3  | |  g4  | |  g5  |      ||
    //||      |      | |      | |      | |      | |      |      ||
    //||       ------   ------   ------   ------   ------       ||
    //||           ______   ______   ______   ______            ||
    //||          |      | |      | |      | |      |           ||
    //||          |  g6  | |  g7  | |  g8  | |  g9  |           ||
    //||          |      | |      | |      | |      |           ||
    //||           ------   ------   ------   ------            ||
    //||     ----------------------------------------------     ||
    //||                           \/                           ||
    //||                    ______   ______                     ||
    //||                   |      | |      |                    ||
    //||                   |  hg  | |  dg  |                    ||
    //||                   |      | |      |                    ||
    //||                    ------   ------                     ||
    //||                       __________                       ||
    //||                      |   fight  |                      ||
    //||                       ----------                       ||
    //||               =========================                ||
    //||                                                        ||
    //============================================================
    //
    //
    //==================================================
    //||             KnownBoth View Head              ||
    //||==============================================||
    //||                                              ||
    //||             __________________               ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||            |                  |              ||
    //||             ------------------               ||
    //||                                              ||
    //||             ==================               ||
    //||             ||   confirm    ||               ||
    //||             ==================               ||
    //||                                              ||
    //==================================================

    if (m_viewOnly || single_result) return;

    int split_line_y = top_blank_width + G_COMMON_LAYOUT.m_cardNormalHeight + card_bottom_to_split_line;
    if (general_number > 5)
        split_line_y += (card_to_center_line + G_COMMON_LAYOUT.m_cardNormalHeight);

    QPixmap line = G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_SPLIT_LINE);
    const int line_length = boundingRect().width() - 2 * left_blank_width;
    const QRectF rect = boundingRect();

    painter->drawPixmap(left_blank_width, split_line_y, line, (line.width() - line_length) / 2, rect.y(), line_length, line.height());

    QPixmap seat = G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT);
    QRect seat1_rect(rect.center().x() - G_COMMON_LAYOUT.m_cardNormalWidth - card_to_center_line - 2, split_line_y + split_line_to_card_seat - 2, G_COMMON_LAYOUT.m_cardNormalWidth + 4, G_COMMON_LAYOUT.m_cardNormalHeight + 4);
    painter->drawPixmap(seat1_rect, seat);
    IQSanComponentSkin::QSanSimpleTextFont font = G_COMMON_LAYOUT.m_chooseGeneralBoxDestSeatFont;
    font.paintText(painter, seat1_rect, Qt::AlignCenter, tr("head_general"));
    QRect seat2_rect(rect.center().x() + card_to_center_line - 2, split_line_y + split_line_to_card_seat - 2, G_COMMON_LAYOUT.m_cardNormalWidth + 4, G_COMMON_LAYOUT.m_cardNormalHeight + 4);
    painter->drawPixmap(seat2_rect, seat);
    font.paintText(painter, seat2_rect, Qt::AlignCenter, tr("deputy_general"));
}

QRectF ChooseGeneralBox::boundingRect() const
{
    //confirm the general count of the first and second row
    int first_row, second_row = 0;

    //arrange them in two rows if there are more than 6 generals.
    //Number of cards in the second row cannot be greater than that in the first row
    //and the difference should not be greater than 1.
    if (general_number < 6) {
        first_row = general_number;
    } else {
        second_row = general_number / 2;
        first_row = general_number - second_row;
    }

    const int width = first_row * G_COMMON_LAYOUT.m_cardNormalWidth + (first_row - 1) * card_to_center_line + left_blank_width * 2;

    int height = top_blank_width + G_COMMON_LAYOUT.m_cardNormalHeight + bottom_blank_width;

    if (second_row)
        height += (card_to_center_line + G_COMMON_LAYOUT.m_cardNormalHeight);

    //No need to reserve space for button
    if (single_result) {
        height -= 30;
    } else if (!m_viewOnly) {
        height += G_COMMON_LAYOUT.m_cardNormalHeight + card_bottom_to_split_line + split_line_to_card_seat;
    }

    return QRectF(0, 0, width, height);
}

static bool sortByKingdom(const QString &gen1, const QString &gen2)
{
    static QMap<QString, int> kingdom_priority_map;
    if (kingdom_priority_map.isEmpty()) {
        QStringList kingdoms = Sanguosha->getKingdoms();
        //kingdoms << "god";
        int i = 0;
        foreach (const QString &kingdom, kingdoms)
            kingdom_priority_map[kingdom] = i++;
    }
    const General *g1 = Sanguosha->getGeneral(gen1);
    const General *g2 = Sanguosha->getGeneral(gen2);

    if (g1 != NULL && g2 != NULL)
        return kingdom_priority_map[g1->getKingdom()] < kingdom_priority_map[g2->getKingdom()];
    else
        return false;

}

void ChooseGeneralBox::chooseGeneral(const QStringList &_generals, bool view_only, bool single_result, const QString &reason, const Player *player, const bool can_convert)
{
    //repaint background
    QStringList generals = _generals;
    this->single_result = single_result;
    if (view_only)
        title = reason;
    if (this->m_viewOnly != view_only) {
        this->m_viewOnly = view_only;
        confirm->setText(view_only ? tr("confirm") : tr("fight"));
    }
    foreach (const QString &general, _generals) {
        if (general.endsWith("(lord)"))
            generals.removeOne(general);
    }

    general_number = generals.length();
    if (!view_only) {
        title = single_result ? tr("Please select one general")
            : tr("Please select the same nationality generals");
        if (!single_result && Self->getSeat() > 0)
            title.prepend(Sanguosha->translate(QString("SEAT(%1)").arg(Self->getSeat()))
            + " ");
    }

    prepareGeometryChange();

    items.clear();
    selected.clear();
    int z = generals.length();

    convertContainer = new CardContainer;
    convertContainer->setObjectName("");
    convertContainer->hide();
    convertContainer->setZValue(z + 3);
    convertContainer->setParentItem(this);

    //DO NOT USE qSort HERE FOR WE NEED TO KEEP THE INITIAL ORDER IN SOME CASES
    qStableSort(generals.begin(), generals.end(), sortByKingdom);

    foreach (const QString &general, generals) {
        int skinId = 0;
        if (player) {
            if (player->getGeneralName() == general)
                skinId = player->getHeadSkinId();
            else
                skinId = player->getDeputySkinId();
        }

        GeneralCardItem *general_item = new GeneralCardItem(general, skinId);
        general_item->setProperty("source", general);
        general_item->setFlag(QGraphicsItem::ItemIsFocusable);
        general_item->setZValue(z--);

        if (view_only || single_result) {
            general_item->setFlag(QGraphicsItem::ItemIsMovable, false);
        } else {
            general_item->setAutoBack(true);
            connect(general_item, &GeneralCardItem::released, this, &ChooseGeneralBox::_adjust);
            if (!Sanguosha->getConvertGenerals(general).isEmpty() && can_convert) {
                Button *button = new Button(Sanguosha->translate("convert_general"), 0.45);
                button->setPos((93 - button->boundingRect().width()) / 2, 130 - button->boundingRect().height());
                button->setParentItem(general_item);
                button->setObjectName(general);
                connect(button, &Button::clicked, this, &ChooseGeneralBox::_onConvertButtonClicked);
            }
        }

        if (!view_only) {
            connect(general_item, &GeneralCardItem::clicked, this, &ChooseGeneralBox::_onItemClicked);
            if (!single_result)
                connect(general_item, &GeneralCardItem::general_changed, this, &ChooseGeneralBox::adjustItems);
        }

        if (!single_result && !view_only) {
            const General *hero = Sanguosha->getGeneral(general);
            foreach (const QString &other, generals) {
                if (general != other && hero->isCompanionWith(other)) {
                    general_item->showCompanion();
                    break;
                }
            }
        }

        items << general_item;
        general_item->setParentItem(this);
    }

    moveToCenter();
    show();

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    int first_row = (general_number < 6) ? general_number : ((general_number + 1) / 2);

    for (int i = 0; i < items.length(); ++i) {
        GeneralCardItem *card_item = items.at(i);

        QPointF pos;
        if (i < first_row) {
            pos.setX(left_blank_width + (card_width + card_to_center_line) * i + card_width / 2);
            pos.setY(top_blank_width + card_height / 2);
        } else {
            if (items.length() % 2 == 1) {
                pos.setX(left_blank_width + card_width / 2 + card_to_center_line / 2
                    + (card_width + card_to_center_line) * (i - first_row) + card_width / 2);
            } else {
                pos.setX(left_blank_width + (card_width + card_to_center_line) * (i - first_row) + card_width / 2);
            }
            pos.setY(top_blank_width + card_height + card_to_center_line + card_height / 2);
        }

        card_item->setPos(25, 45);
        //store initial position
        if (!single_result && !view_only)
            card_item->setData(S_DATA_INITIAL_HOME_POS, pos);
        card_item->setHomePos(pos);
        card_item->goBack(true);
    }

    if (single_result) {
        confirm->hide();
    } else {
        confirm->setPos(boundingRect().center().x() - confirm->boundingRect().width() / 2, boundingRect().height() - 60);
        confirm->show();
    }
    if (!view_only && !single_result)
        _initializeItems();

    if (view_only || ServerInfo.OperationTimeout != 0) {
        if (!progress_bar) {
            progress_bar = new QSanCommandProgressBar();
            progress_bar->setMaximumWidth(boundingRect().width() - 10);
            progress_bar->setMaximumHeight(12);
            progress_bar->setTimerEnabled(true);
            progress_bar_item = new QGraphicsProxyWidget(this);
            progress_bar_item->setWidget(progress_bar);
            progress_bar_item->setPos(boundingRect().center().x() - progress_bar_item->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progress_bar, &QSanCommandProgressBar::timedOut, this, &ChooseGeneralBox::reply);
        }
        if (view_only) {
            Countdown countdown;
            countdown.max = 10000;
            countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
            progress_bar->setCountdown(countdown);
        } else
            progress_bar->setCountdown(QSanProtocol::S_COMMAND_CHOOSE_GENERAL);
        progress_bar->show();
    }
}

void ChooseGeneralBox::_adjust()
{
    GeneralCardItem *item = qobject_cast<GeneralCardItem *>(sender());
    if (item == NULL) return;

    int middle_y = top_blank_width + G_COMMON_LAYOUT.m_cardNormalHeight + card_bottom_to_split_line;
    if (general_number > 5)
        middle_y += (card_to_center_line + G_COMMON_LAYOUT.m_cardNormalHeight);

    if (selected.contains(item) && item->y() <= middle_y) {
        selected.removeOne(item);
        items << item;
        item->setHomePos(item->data(S_DATA_INITIAL_HOME_POS).toPointF());
        item->goBack(true);
        //the item is on the way
    } else if (selected.length() == 2
        && ((!Sanguosha->getGeneral(selected.first()->objectName())->isLord()
        && selected.first() == item && item->x() > boundingRect().center().x())
        || (selected.last() == item && item->x() < boundingRect().center().x())))
        qSwap(selected[0], selected[1]);
    else if (items.contains(item) && item->y() > middle_y) {
        if (selected.length() > 1) return;
        items.removeOne(item);
        selected << item;
    }

    adjustItems();
}

void ChooseGeneralBox::adjustItems()
{
    if (!selected.isEmpty()) {
        const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
        const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;

        int dest_seat_y = top_blank_width + G_COMMON_LAYOUT.m_cardNormalHeight
            + card_bottom_to_split_line + split_line_to_card_seat + card_height / 2
            - 1;
        if (general_number > 5)
            dest_seat_y += (card_to_center_line + card_height);
        selected.first()->setHomePos(QPointF(boundingRect().center().x()
            - card_to_center_line - card_width / 2 - 2,
            dest_seat_y));
        selected.first()->goBack(true);
        if (selected.length() == 2) {
            selected.last()->setHomePos(QPointF(boundingRect().center().x()
                + card_to_center_line + card_width / 2
                - 1, dest_seat_y));
            selected.last()->goBack(true);
        }
    }

    if (selected.length() == 2) {
        foreach(GeneralCardItem *card, items)
            card->setFrozen(true);
        confirm->setEnabled((Sanguosha->getGeneral(selected.first()->objectName())->getKingdom()
            == Sanguosha->getGeneral(selected.last()->objectName())->getKingdom()) ||
                            ServerInfo.FreeKingdom);
    } else if (selected.length() == 1) {
        selected.first()->hideCompanion();
        const General *seleted_general = Sanguosha->getGeneral(selected.first()->objectName());
        foreach (GeneralCardItem *card, items) {
            const General *general = Sanguosha->getGeneral(card->objectName());
            if (BanPair::isBanned(seleted_general->objectName(), general->objectName())
                || (general->getKingdom() != seleted_general->getKingdom() || general->isLord())) {
                card->setFrozen(true && !ServerInfo.FreeKingdom);
                card->hideCompanion();
            } else {
                if (card->isFrozen())
                    card->setFrozen(false);
                if (general->isCompanionWith(selected.first()->objectName())) {
                    selected.first()->showCompanion();
                    card->showCompanion();
                } else {
                    card->hideCompanion();
                }
            }
        }
        if (confirm->isEnabled()) confirm->setEnabled(false);
    } else {
        _initializeItems();
        foreach (GeneralCardItem *card, items) {
            card->hideCompanion();
            foreach (GeneralCardItem *other, items) {
                if (other->objectName().endsWith("(lord)")) continue;
                const General *hero = Sanguosha->getGeneral(card->objectName());
                if (card != other && hero->isCompanionWith(other->objectName())) {
                    card->showCompanion();
                    break;
                }
            }
        }
        if (confirm->isEnabled()) confirm->setEnabled(false);
    }
    convertContainer->clear();
}

void ChooseGeneralBox::_initializeItems()
{
    QList<const General *> generals;
    foreach (GeneralCardItem *item, items)
        generals << Sanguosha->getGeneral(item->objectName());

    int index = 0;
    foreach (const General *general, generals) {
        int party = 0;
        bool has_lord = false;
        foreach (const General *other, generals) {
            if (other->getKingdom() == general->getKingdom()) {
                party++;
                if (other != general && other->isLord())
                    has_lord = true;
            }
        }
        GeneralCardItem *item = items.at(index);
        if ((party < 2 || (selected.isEmpty() && has_lord && party == 2)))
            item->setFrozen(true && !ServerInfo.FreeKingdom);
        else
            item->setFrozen(false);

        if (Self->isDead() && item->isFrozen())
            item->setFrozen(false);
        ++index;
    }
}

void ChooseGeneralBox::reply()
{
    if (progress_bar != NULL) {
        progress_bar->hide();
        progress_bar->deleteLater();
        progress_bar = NULL;
    }

    if (m_viewOnly)
        return clear();

    QString generals;
    if (!selected.isEmpty()) {
        generals = selected.first()->objectName();
        if (selected.length() == 2)
            generals += ("+" + selected.last()->objectName());
    }
    ClientInstance->onPlayerChooseGeneral(generals);
}

void ChooseGeneralBox::clear()
{
    foreach(GeneralCardItem *card_item, items)
        card_item->deleteLater();

    foreach(GeneralCardItem *card_item, selected)
        card_item->deleteLater();

    items.clear();
    selected.clear();

    disappear();
}

void ChooseGeneralBox::_onItemClicked()
{
    GeneralCardItem *item = qobject_cast<GeneralCardItem *>(sender());
    if (item == NULL) return;

    if (single_result) {
        selected << item;
        reply();
        return;
    }

    if (selected.contains(item)) {
        selected.removeOne(item);
        items << item;
        item->setHomePos(item->data(S_DATA_INITIAL_HOME_POS).toPointF());
        item->goBack(true);
    } else if (items.contains(item)) {
        if (selected.length() > 1) return;
        items.removeOne(item);
        selected << item;
    }

    adjustItems();
}

void ChooseGeneralBox::_onConvertButtonClicked()
{
    Button *button = qobject_cast<Button *>(sender());
    QString general = button->objectName();
    if (convertContainer->isVisible()) {
        convertContainer->clear();
    }
    if (convertContainer->objectName() == general) {
        convertContainer->setObjectName("");
        return;
    }
    QList<CardItem *> generals;
    GeneralCardItem *origin_item = new GeneralCardItem(general, 0);
    origin_item->setParentItem(convertContainer);
    origin_item->setFlag(ItemIsMovable, false);
    origin_item->setObjectName(general);
    origin_item->setProperty("source", general);
    connect(origin_item, &GeneralCardItem::clicked, this, &ChooseGeneralBox::_onConvertClicked);
    generals << origin_item;

    foreach (QString name, Sanguosha->getConvertGenerals(general)) {
        GeneralCardItem *item = new GeneralCardItem(name, 0);
        item->setParentItem(convertContainer);
        item->setFlag(ItemIsMovable, false);
        item->setObjectName(name);
        item->setProperty("source", general);
        connect(item, &GeneralCardItem::clicked, this, &ChooseGeneralBox::_onConvertClicked);
        generals << item;
    }
    convertContainer->fillGeneralCards(generals);
    convertContainer->setObjectName(general);
    convertContainer->setPos(-93 * (Sanguosha->getConvertGenerals(general).length() +  1.5), 0);
    convertContainer->show();
}

void ChooseGeneralBox::_onConvertClicked()
{
    GeneralCardItem *source_item = qobject_cast<GeneralCardItem *>(sender());
    foreach (GeneralCardItem *item, items) {
        if (item->property("source").toString() == source_item->property("source").toString()) {
            item->changeGeneral(source_item->objectName());
            break;
        }
    }
    foreach (GeneralCardItem *item, selected) {
        if (item->property("source").toString() == source_item->property("source").toString()) {
            item->changeGeneral(source_item->objectName());
            break;
        }
    }
    adjustItems();
}
