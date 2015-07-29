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

#include "cardoverview.h"
#ifdef Q_OS_IOS
#include "ui_cardoverview_ios.h"
#else
#include "ui_cardoverview.h"
#endif
#include "engine.h"
#include "stylehelper.h"
#include "clientstruct.h"
#include "settings.h"
#include "client.h"
#include "clientplayer.h"
#include "skinbank.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QFile>

static CardOverview *Overview;

CardOverview *CardOverview::getInstance(QWidget *main_window)
{
    if (Overview == NULL)
        Overview = new CardOverview(main_window);

    return Overview;
}

CardOverview::CardOverview(QWidget *parent)
    : FlatDialog(parent, false), ui(new Ui::CardOverview)
{
    ui->setupUi(this);

    connect(this, &CardOverview::windowTitleChanged, ui->titleLabel, &QLabel::setText);

#if !defined(Q_OS_IOS)
    ui->tableWidget->setColumnWidth(0, 80);
    ui->tableWidget->setColumnWidth(1, 60);
    ui->tableWidget->setColumnWidth(2, 30);
    ui->tableWidget->setColumnWidth(3, 60);
    ui->tableWidget->setColumnWidth(4, 70);
#endif

    connect(ui->getCardButton, &QPushButton::clicked, this, &CardOverview::askCard);
    connect(ui->closeButton, &QPushButton::clicked, this, &CardOverview::reject);

    ui->cardDescriptionBox->setProperty("description", true);
    ui->malePlayButton->hide();
    ui->femalePlayButton->hide();
    ui->playAudioEffectButton->hide();

    const QString style = StyleHelper::styleSheetOfScrollBar();
#if !defined(Q_OS_IOS)
    ui->tableWidget->verticalScrollBar()->setStyleSheet(style);
#else
    ui->cardDescriptionBox->verticalScrollBar()->setStyleSheet(style);
#endif
}

void CardOverview::loadFromAll()
{
    int n = Sanguosha->getCardCount();
#if !defined(Q_OS_IOS)
    ui->tableWidget->setRowCount(n);
#endif
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        addCard(i, card);
    }
#ifdef Q_OS_IOS
    connect(ui->cardComboBox, &QComboBox::currentTextChanged, this, &CardOverview::comboBoxChanged);
#endif
    if (n > 0) {
        //SE
#ifdef Q_OS_IOS
        ui->cardComboBox->setCurrentIndex(0);
        comboBoxChanged();
#else
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));
#endif

        const Card *card = Sanguosha->getEngineCard(0);
        if (card->getTypeId() == Card::TypeEquip) {
            ui->playAudioEffectButton->show();
            ui->malePlayButton->hide();
            ui->femalePlayButton->hide();
        } else {
            ui->playAudioEffectButton->hide();
            ui->malePlayButton->show();
            ui->femalePlayButton->show();
        }
    }
}

void CardOverview::loadFromList(const QList<const Card *> &list)
{
    int n = list.length();
#ifdef Q_OS_IOS
    ui->cardComboBox->setMaxCount(n);
#else
    ui->tableWidget->setRowCount(n);
#endif
    for (int i = 0; i < n; i++)
        addCard(i, list.at(i));

    if (n > 0) {
#ifdef Q_OS_IOS
        ui->cardComboBox->setCurrentIndex(0);
        comboBoxChanged();
#else
        ui->tableWidget->setCurrentItem(ui->tableWidget->item(0, 0));
#endif

        const Card *card = list.first();
        if (card->getTypeId() == Card::TypeEquip) {
            ui->playAudioEffectButton->show();
            ui->malePlayButton->hide();
            ui->femalePlayButton->hide();
        } else {
            ui->playAudioEffectButton->hide();
            ui->malePlayButton->show();
            ui->femalePlayButton->show();
        }
    }
}

void CardOverview::addCard(int i, const Card *card)
{
    QString name = Sanguosha->translate(card->objectName());
    QIcon suit_icon = QIcon(QString("image/system/suit/%1.png").arg(card->getSuitString()));
    QString point = card->getNumberString();
#if !defined(Q_OS_IOS)
    QString suit_str = Sanguosha->translate(card->getSuitString());

    QString type = Sanguosha->translate(card->getType());
    QString subtype = Sanguosha->translate(card->getSubtype());
    QString package = Sanguosha->translate(card->getPackage());

    QTableWidgetItem *name_item = new QTableWidgetItem(name);
    name_item->setData(Qt::UserRole, card->getId());

    ui->tableWidget->setItem(i, 0, name_item);
    ui->tableWidget->setItem(i, 1, new QTableWidgetItem(suit_icon, suit_str));
    ui->tableWidget->setItem(i, 2, new QTableWidgetItem(point));
    ui->tableWidget->setItem(i, 3, new QTableWidgetItem(type));
    ui->tableWidget->setItem(i, 4, new QTableWidgetItem(subtype));

    QTableWidgetItem *package_item = new QTableWidgetItem(package);
    if (Config.value("LuaPackages", QString()).toString().split("+").contains(card->getPackage())) {
        package_item->setBackgroundColor(QColor(0x66, 0xCC, 0xFF));
        package_item->setToolTip(tr("<font color=%1>This is an Lua extension</font>").arg(Config.SkillDescriptionInToolTipColor.name()));
    }
    ui->tableWidget->setItem(i, 5, package_item);
#else
    ui->cardComboBox->addItem(suit_icon, name + " " + point, card->getId());
#endif
}

CardOverview::~CardOverview()
{
    delete ui;
}

#ifdef Q_OS_IOS
void CardOverview::comboBoxChanged() {
    int card_id = ui->cardComboBox->currentData().toInt();
    const Card *card = Sanguosha->getEngineCard(card_id);
    QString pixmap_path = QString("image/card/%1.png").arg(card->objectName());
    ui->cardLabel->setPixmap(pixmap_path);

    ui->cardDescriptionBox->setText(card->getDescription(false));
    ui->cardDescriptionBox->setTextColor(Config.SkillDescriptionInToolTipColor.name());
    ui->packageLine->setText(Sanguosha->translate(card->getPackage()));
    ui->subtypeLine->setText(Sanguosha->translate(card->getSubtype()));
    ui->typeLine->setText(Sanguosha->translate(card->getType()));

    if (card->getTypeId() == Card::TypeEquip) {
        ui->playAudioEffectButton->show();
        ui->malePlayButton->hide();
        ui->femalePlayButton->hide();
    } else {
        ui->playAudioEffectButton->hide();
        ui->malePlayButton->show();
        ui->femalePlayButton->show();
    }
}
#endif
void CardOverview::on_tableWidget_itemSelectionChanged()
{
#ifndef Q_OS_IOS
    int row = ui->tableWidget->currentRow();
    int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
    const Card *card = Sanguosha->getEngineCard(card_id);
    QString pixmap_path = QString("image/big-card/%1.png").arg(card->objectName());
    ui->cardLabel->setPixmap(pixmap_path);

    ui->cardDescriptionBox->setTextColor(Config.SkillDescriptionInToolTipColor);
    ui->cardDescriptionBox->setText(card->getDescription(false));


    if (card->getTypeId() == Card::TypeEquip) {
        ui->playAudioEffectButton->show();
        ui->malePlayButton->hide();
        ui->femalePlayButton->hide();
    } else {
        ui->playAudioEffectButton->hide();
        ui->malePlayButton->show();
        ui->femalePlayButton->show();
    }
#endif
}

void CardOverview::on_tableWidget_itemDoubleClicked(QTableWidgetItem *)
{
#ifndef Q_OS_IOS
    if (Self) askCard();
#endif
}


void CardOverview::askCard()
{
    if (!ServerInfo.EnableCheat || !ClientInstance)
        return;

#ifdef Q_OS_IOS
    int row = ui->cardComboBox->currentIndex();
#else
    int row = ui->tableWidget->currentRow();
#endif
    if (row >= 0) {
#ifdef Q_OS_IOS
        int card_id = ui->cardComboBox->currentData().toInt();
#else
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
#endif
        if (!ClientInstance->getAvailableCards().contains(card_id)) {
            QMessageBox::warning(this, tr("Warning"), tr("These packages don't contain this card"));
            return;
        }
        ClientInstance->requestCheatGetOneCard(card_id);
    }
}



void CardOverview::on_malePlayButton_clicked()
{
#ifdef Q_OS_IOS
    int row = ui->cardComboBox->currentIndex();
#else
    int row = ui->tableWidget->currentRow();
#endif
    if (row >= 0) {
#ifdef Q_OS_IOS
        int card_id = ui->cardComboBox->currentData().toInt();
#else
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
#endif
        const Card *card = Sanguosha->getEngineCard(card_id);
        Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card->objectName(), true));
    }
}

void CardOverview::on_femalePlayButton_clicked()
{
#ifdef Q_OS_IOS
    int row = ui->cardComboBox->currentIndex();
#else
    int row = ui->tableWidget->currentRow();
#endif
    if (row >= 0) {
#ifdef Q_OS_IOS
        int card_id = ui->cardComboBox->currentData().toInt();
#else
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
#endif
        const Card *card = Sanguosha->getEngineCard(card_id);
        Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(card->objectName(), false));
    }
}

void CardOverview::on_playAudioEffectButton_clicked()
{
#ifdef Q_OS_IOS
    int row = ui->cardComboBox->currentIndex();
#else
    int row = ui->tableWidget->currentRow();
#endif
    if (row >= 0) {
#ifdef Q_OS_IOS
        int card_id = ui->cardComboBox->currentData().toInt();
#else
        int card_id = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toInt();
#endif
        const Card *card = Sanguosha->getEngineCard(card_id);
        if (card->getTypeId() == Card::TypeEquip) {
            QString effectName = card->getEffectName();
            if (effectName == "vscrossbow")
                effectName = "crossbow";
            QString fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(effectName, QString("equip"), -1);
            if (!QFile::exists(fileName))
                fileName = G_ROOM_SKIN.getPlayerAudioEffectPath(card->getCommonEffectName(), QString("common"), -1);
            Sanguosha->playAudioEffect(fileName);
        }
    }
}

void CardOverview::showEvent(QShowEvent *)
{
    if (ServerInfo.EnableCheat && ClientInstance) {
        ui->getCardButton->show();
    } else {
        ui->getCardButton->hide();
    }
}
