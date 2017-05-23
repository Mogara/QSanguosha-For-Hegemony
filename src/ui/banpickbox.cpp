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

#include "banpickbox.h"
#include "engine.h"
#include "skinbank.h"
#include "button.h"
#include "client.h"
#include "roomscene.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QPushButton>

using namespace QSanProtocol;

const int BanPickBox::topBlankWidth = 48;
const int BanPickBox::bottomBlankWidth = 30;
const int BanPickBox::interval = 10;

BanPickBox::BanPickBox()
    : progress_bar(NULL), selector(QString())
{
}

    //============================================================
    //||========================================================||
    //||      Please pick self or ban opponent's general        ||
    //||       ______   ______   ______   ______   ______       ||
    //||      |      | |      | |      | |      | |      |      ||
    //||      |  o1  | |  o2  | |  o3  | |  o4  | |  o5  |      ||
    //||      |      | |      | |      | |      | |      |      ||
    //||       ------   ------   ------   ------   ------       ||
    //||       ______   ______   ______   ______   ______       ||
    //||      |      | |      | |      | |      | |      |      ||
    //||      |  o6  | |  o7  | |  o8  | |  o9  | | o10  |      ||
    //||      |      | |      | |      | |      | |      |      ||
    //||       ------   ------   ------   ------   ------       ||
    //||       ______   ______   ______   ______   ______       ||
    //||      |      | |      | |      | |      | |      |      ||
    //||      |  s1  | |  s2  | |  s3  | |  s4  | |  s5  |      ||
    //||      |      | |      | |      | |      | |      |      ||
    //||       ------   ------   ------   ------   ------       ||
    //||       ______   ______   ______   ______   ______       ||
    //||      |      | |      | |      | |      | |      |      ||
    //||      |  s6  | |  s7  | |  s8  | |  s9  | | s10  |      ||
    //||      |      | |      | |      | |      | |      |      ||
    //||       ------   ------   ------   ------   ------       ||
    //||                                                        ||
    //============================================================


QRectF BanPickBox::boundingRect() const
{
    int defaultButtonwidth = 64 * scale / 10;
    int defaultButtonHeight = 148 * scale / 10;
    int row = qMax(self.length(), opponent.length()) > 10 ? 2 : 1;
    int count = qMax(ceil((double)self.length() / row), ceil((double)opponent.length() / row));
    int width = defaultButtonwidth * count + count + interval * 2;
    int height = topBlankWidth + bottomBlankWidth + floor(row / 2)
                + this->row * defaultButtonHeight + interval;

    return QRectF(0, 0, width, height);
}

void BanPickBox::showBox(QStringList self, QStringList opponent, QStringList bans, QStringList selected)
{
    this->self = self;
    this->opponent = opponent;
    this->selected = selected;
    this->bans = bans;
    selector = QString();

    if (!buttons.isEmpty()) {
        foreach(GeneralHalf *button, buttons)
            button->deleteLater();

        buttons.clear();
    }

    int row = qMax(self.length(), opponent.length()) > 10 ? 2 : 1;
    this->row = row + (qMin(self.length(), opponent.length()) > 10 ? 2 : 1);
    int count = qMax(ceil((double)self.length() / row), ceil((double)opponent.length() / row));
    int bounding_width = qMin(64 * count + count + interval * 2, int(RoomSceneInstance->sceneRect().width() * 0.8));
    scale = floor((bounding_width - count + interval * 2) * 10 / count / 64);

    moveToCenter();
    show();

    int width = 64 * scale / 10;
    int height = 148 * scale / 10;
    int count1 = opponent.length() > 10 ? ceil(opponent.length() / 2) : opponent.length();
    for (int i = 0; i < opponent.length(); i++) {
        QString name = opponent.at(i);

        QString status;
        if (bans.contains(name))
            status = "ban";
        else if (selected.contains(name))
            status = "selected";

        GeneralHalf *button = new GeneralHalf(this, name, scale, status);
        button->setObjectName(name);
        if (bans.contains(name) || selected.contains(name))
            button->setEnabled(false);
        buttons << button;

        QPointF apos;
        if (i + 1 <= count1) {
            apos.setX((boundingRect().width() - count1 - count1 * width) / 2 + i + width * i);
            apos.setY(topBlankWidth);
        } else {
            int x = opponent.length() - count1;
            apos.setX((boundingRect().width() - x - x * width) / 2 + (i - count1) + width * (i - count1));
            apos.setY(topBlankWidth + height + 1);
        }
        button->setPos(apos);
    }

    int count2 = self.length() > 10 ? ceil(self.length() / 2) : self.length();
    int app = count1 < opponent.length() ? (2 * height + 1 + interval) : (height + interval);
    for (int i = 0; i < self.length(); i++) {
        QString name = self.at(i);

        QString status;
        if (bans.contains(name))
            status = "ban";
        else if (selected.contains(name))
            status = "selected";

        GeneralHalf *button = new GeneralHalf(this, name, scale, status);
        button->setObjectName(name);
        if (bans.contains(name) || selected.contains(name))
            button->setEnabled(false);
        buttons << button;

        QPointF apos;
        if (i + 1 <= count2) {
            apos.setX((boundingRect().width() - count2 - count2 * width) / 2 + i + width * i);
            apos.setY(topBlankWidth + app);
        } else {
            int x = self.length() - count2;
            apos.setX((boundingRect().width() - x - x * width) / 2 + (i - count2) + width * (i - count2));
            apos.setY(topBlankWidth + height + 1 + app);
        }
        button->setPos(apos);
    }

    if (progress_bar != NULL) {
        progress_bar->hide();
        progress_bar->deleteLater();
        progress_bar = NULL;
    }

    update();
}

void BanPickBox::StartChoose(const QString &selector)
{
    this->selector = selector;
    if (Self->objectName() == selector) {
        foreach(GeneralHalf *button, buttons) {
            button->setActivate(true);
            if (!button->objectName().isEmpty() && !bans.contains(button->objectName()) && !selected.contains(button->objectName()))
                connect(button, &GeneralHalf::clicked, this, &BanPickBox::reply);
        }
    }

    if (ServerInfo.OperationTimeout != 0) {
        if (!progress_bar) {
            progress_bar = new QSanCommandProgressBar();
            progress_bar->setMaximumWidth(boundingRect().width() - 100);
            progress_bar->setMaximumHeight(12);
            progress_bar->setTimerEnabled(true);
            progress_bar_item = new QGraphicsProxyWidget(this);
            progress_bar_item->setWidget(progress_bar);
            progress_bar_item->setPos(boundingRect().center().x() - progress_bar_item->boundingRect().width() / 2, boundingRect().height() - 20);
            if (Self->objectName() == selector)
                connect(progress_bar, &QSanCommandProgressBar::timedOut, this, &BanPickBox::reply);
        }
        progress_bar->setCountdown(QSanProtocol::S_COMMAND_CHOOSE_GENERAL);
        progress_bar->show();
    }

    update();
}

void BanPickBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (selector == Self->objectName())
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("Please pick self or ban opponent's general"), boundingRect());
    else if (!selector.isEmpty() && (!bans.isEmpty() || !selected.isEmpty()))
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("opponent is choosing"), boundingRect());
    else if (selector.isEmpty()) {
        if (bans.isEmpty() && selected.isEmpty())
            GraphicsBox::paintGraphicsBoxStyle(painter, tr("ready to start"), boundingRect());
        else
            GraphicsBox::paintGraphicsBoxStyle(painter, tr("result"), boundingRect());
    }
}

void BanPickBox::reply()
{
    const QString &answer = sender()->objectName();
    silence();

    if (progress_bar != NULL) {
        progress_bar->hide();
        progress_bar->deleteLater();
        progress_bar = NULL;
    }

    ClientInstance->onPlayerReplyBanPick(answer);
}

void BanPickBox::silence()
{
    foreach(GeneralHalf *button, buttons) {
        button->setActivate(false);
        disconnect(button, &GeneralHalf::clicked, this, &BanPickBox::reply);
    }
}

void BanPickBox::clear()
{
    foreach(GeneralHalf *card_item, buttons)
        card_item->deleteLater();

    buttons.clear();
    disappear();
}

GeneralHalf::GeneralHalf(QGraphicsObject *parent, const QString &name, int scale, const QString &status)
    : QGraphicsObject(parent), cardName(name), Scale(scale), status(status), activate(false)
{
    setAcceptedMouseButtons(Qt::LeftButton);

    setAcceptHoverEvents(true);
    setFlag(ItemIsFocusable);

    setOpacity(0.7);
    setObjectName(name);

    const General *general = Sanguosha->getGeneral(name);
    if (general)
        setToolTip(general->getSkillDescription(true));
}

void GeneralHalf::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    QPixmap generalImage = G_ROOM_SKIN.getGeneralPixmap(cardName, QSanRoomSkin::S_GENERAL_ICON_SIZE_PHOTO_PRIMARY);
    QPixmap pixmap = generalImage.scaled(QSize(128, 148) * Scale / 10, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (!status.isEmpty()) {
        QPixmap pix = G_ROOM_SKIN.getPixmapFromFileName("image/system/" + status + ".png");
        QPainter paint(&pixmap);
        paint.drawPixmap(boundingRect().width() - pix.width(), boundingRect().height() - pix.height(), pix.width(), pix.height(), pix);
    }

    painter->setBrush(pixmap);
    painter->drawRoundedRect(boundingRect(), 5, 5, Qt::RelativeSize);
}

QRectF GeneralHalf::boundingRect() const
{
    QSize cardButtonSize = QSize(64, 148);
    return QRectF(QPoint(0, 0), cardButtonSize * Scale / 10);
}

void GeneralHalf::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void GeneralHalf::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

void GeneralHalf::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    if (!activate) return;
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(1);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void GeneralHalf::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    if (!activate) return;
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(0.7);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
