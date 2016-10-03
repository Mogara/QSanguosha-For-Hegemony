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

#include "chooseoptionsbox.h"
#include "engine.h"
#include "button.h"
#include "client.h"
#include "clientstruct.h"
#include "timedprogressbar.h"
#include "skinbank.h"

#include <QGraphicsProxyWidget>

ChooseOptionsBox::ChooseOptionsBox()
    : progressBar(NULL)
{
}
//====================
//||================||
//|| Please Choose: ||
//||    _______     ||
//||   |   1   |    ||
//||    -------     ||
//||    _______     ||
//||   |   2   |    ||
//||    -------     ||
//||    _______     ||
//||   |   3   |    ||
//||    -------     ||
//====================

QRectF ChooseOptionsBox::boundingRect() const
{
    const int width = getButtonWidth()* (qMax(options.length(), 1)) + outerBlankWidth * 2 + (qMax(options.length(), 1) - 1) * interval;

    int max = 0;
    foreach (const QString &str, options)
        max = qMax(max, str.split("+").length());

    int height = topBlankWidth + max * defaultButtonHeight + (max - 1) * interval + bottomBlankWidth;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    return QRectF(0, 0, width, height);
}

void ChooseOptionsBox::chooseOption(const QStringList &options)
{
#ifdef Q_OS_ANDROID
    minButtonWidth = G_DASHBOARD_LAYOUT.m_avatarArea.width() * 2;
    defaultButtonHeight = G_DASHBOARD_LAYOUT.m_normalHeight / 2;
#endif
    //repaint background
    this->options = options;

    QStringList titles = skillName.split("%");
    QString skillname = titles.at(0);
    QString titile_text;
    if (titles.length() > 1) {
        titile_text = translate("#" + skillname);
        foreach (const QString &element, titles) {
            if (element.startsWith("from:")) {
                QStringList froms = element.split(":");
                if (!froms.at(1).isEmpty()) {
                    QString from = ClientInstance->getPlayerName(froms.at(1));
                    titile_text.replace("%from", from);
                }
            } else if (element.startsWith("to:")) {
                QStringList tos = element.split(":");
                QStringList to_list;
                for (int i = 1; i < tos.length(); i++)
                    to_list << ClientInstance->getPlayerName(tos.at(i));
                QString to = to_list.join(", ");
                titile_text.replace("%to", to);
            } else if (element.startsWith("log:")) {
                QStringList logs = element.split(":");
                if (!logs.at(1).isEmpty()) {
                    QString log = logs.at(1);
                    titile_text.replace("%log", log);
                }
            }
        }
    } else
        titile_text = translate(skillName);

    title = QString("%1 %2").arg(Sanguosha->translate(titile_text)).arg(tr("Please choose:"));
    prepareGeometryChange();

    const int buttonWidth = getButtonWidth();
    QMap<Button *, QPoint> pos;
    int x = 0;
    int y = 0;
    foreach (const QString &option, options) {
        y = 0;
        ++x;
        foreach (const QString &choice, option.split("+")) {
            ++y;

            QStringList choices = choice.split("%");
            QString choice_ = choices.at(0);
            QString text = translate(choice_);

            foreach (const QString &element, choices) {
                if (element.startsWith("from:")) {
                    QStringList froms = element.split(":");
                    if (!froms.at(1).isEmpty()) {
                        QString from = ClientInstance->getPlayerName(froms.at(1));
                        text.replace("%from", from);
                    }
                } else if (element.startsWith("to:")) {
                    QStringList tos = element.split(":");
                    QStringList to_list;
                    for (int i = 1; i < tos.length(); i++)
                        to_list << ClientInstance->getPlayerName(tos.at(i));
                    QString to = to_list.join(", ");
                    text.replace("%to", to);
                } else if (element.startsWith("log:")) {
                    QStringList logs = element.split(":");
                    if (!logs.at(1).isEmpty()) {
                        QString log = logs.at(1);
                        text.replace("%log", log);
                    }
                }
            }

            Button *button = new Button(text, QSizeF(buttonWidth,
                defaultButtonHeight));
            button->setObjectName(choice);
            buttons << button;
            button->setParentItem(this);
            pos[button] = QPoint(x, y);

            QString original_tooltip = QString(":%1").arg(title);
            QString tooltip = Sanguosha->translate(original_tooltip);
            if (tooltip == original_tooltip) {
                original_tooltip = QString(":%1").arg(choice);
                tooltip = Sanguosha->translate(original_tooltip);
            }
            connect(button, &Button::clicked, this, &ChooseOptionsBox::reply);
            if (tooltip != original_tooltip)
                button->setToolTip(QString("<font color=%1>%2</font>")
                .arg(Config.SkillDescriptionInToolTipColor.name())
                .arg(tooltip));

        }

    }

    moveToCenter();
    show();

    for (int i = 0; i < buttons.length(); ++i) {
        Button *button = buttons.at(i);

        QPoint p = pos[button];

        QPointF pos;
        pos.setX(outerBlankWidth + (p.x() - 1) * (getButtonWidth() + interval));
        pos.setY(topBlankWidth + defaultButtonHeight *(p.y() - 1) + (p.y() - 2) * interval + defaultButtonHeight / 2);

        button->setPos(pos);
    }

    if (ServerInfo.OperationTimeout != 0) {
        if (!progressBar) {
            progressBar = new QSanCommandProgressBar();
            progressBar->setMaximumWidth(boundingRect().width() - 16);
            progressBar->setMaximumHeight(12);
            progressBar->setTimerEnabled(true);
            progressBarItem = new QGraphicsProxyWidget(this);
            progressBarItem->setWidget(progressBar);
            progressBarItem->setPos(boundingRect().center().x() - progressBarItem->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progressBar, &QSanCommandProgressBar::timedOut, this, &ChooseOptionsBox::reply);
        }
        progressBar->setCountdown(QSanProtocol::S_COMMAND_MULTIPLE_CHOICE);
        progressBar->show();
    }
}

void ChooseOptionsBox::reply()
{
    QString choice = sender()->objectName();
    if (choice.isEmpty())
        choice = options.first();
    ClientInstance->onPlayerMakeChoice(choice);
}

int ChooseOptionsBox::getButtonWidth() const
{
    if (options.isEmpty())
        return minButtonWidth;

    QFontMetrics fontMetrics(Button::defaultFont());
    int biggest = 0;
    foreach (const QString &section, options) {
        foreach (const QString &choice, section.split("+")) {
            QStringList choices = choice.split("%");
            QString choice_ = choices.at(0);
            QString text = translate(choice_);

            foreach (const QString &element, choices) {
                if (element.startsWith("from:")) {
                    QStringList froms = element.split(":");
                    if (!froms.at(1).isEmpty()) {
                        QString from = ClientInstance->getPlayerName(froms.at(1));
                        text.replace("%from", from);
                    }
                } else if (element.startsWith("to:")) {
                    QStringList tos = element.split(":");
                    QStringList to_list;
                    for (int i = 1; i < tos.length(); i++)
                        to_list << ClientInstance->getPlayerName(tos.at(i));
                    QString to = to_list.join(", ");
                    text.replace("%to", to);
                } else if (element.startsWith("log:")) {
                    QStringList logs = element.split(":");
                    if (!logs.at(1).isEmpty()) {
                        QString log = logs.at(1);
                        text.replace("%log", log);
                    }
                }
            }
            const int width = fontMetrics.width(text);
            if (width > biggest)
                biggest = width;
        }
    }

    // Otherwise it would look compact
    biggest += 20;

    int width = minButtonWidth;
    return qMax(biggest, width);
}

QString ChooseOptionsBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skillName.split("%").at(0)).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}

void ChooseOptionsBox::clear()
{
    if (progressBar != NULL) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = NULL;
    }

    foreach (Button *button, buttons)
        button->deleteLater();

    buttons.clear();

    disappear();
}
