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

#include "connectiondialog.h"
#ifdef Q_OS_IOS
#include "ui_connectiondialog_ios.h"
#else
#include "ui_connectiondialog.h"
#endif
#include "settings.h"
#include "engine.h"
#include "stylehelper.h"
#include "udpdetectordialog.h"
#include "avatarmodel.h"
#include "skinbank.h"

#include <QMessageBox>
#include <QRadioButton>
#include <QBoxLayout>
#include <QScrollBar>
#include <QDesktopWidget>

static const int ShrinkWidth = 317;
static const int ExpandWidth = 619;

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : FlatDialog(parent, false), ui(new Ui::ConnectionDialog)
{
    QDesktopWidget* desktop = qApp->desktop();
#ifdef Q_OS_ANDROID
    QFont f = this->font();
    f.setPointSize(3);
    setFont(f);
#endif
    ui->setupUi(this);
    //QMetaObject::connect(ui->cancelButton, SIGNAL(QPushButton::click()), this, SLOT(reject());)
    ui->nameLineEdit->setText(Config.UserName.left(8));

    ui->hostComboBox->addItems(Config.HistoryIPs);
    ui->hostComboBox->lineEdit()->setText(Config.HostAddress);

    ui->connectButton->setFocus();

    ui->avatarPixmap->setPixmap(G_ROOM_SKIN.getGeneralPixmap(Config.UserAvatar,
        QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));

    ui->reconnectionCheckBox->setChecked(Config.value("EnableReconnection", false).toBool());

    connect(this, &ConnectionDialog::windowTitleChanged, ui->title, &QLabel::setText);

    QScrollBar *bar = ui->avatarList->verticalScrollBar();
    bar->setStyleSheet(StyleHelper::styleSheetOfScrollBar());

#ifdef Q_OS_ANDROID
    setMinimumSize(desktop->width(), desktop->height());
    setStyleSheet("background-color: #F0FFF0; color: black;");
    ui->groupBox->setMinimumSize(desktop->width() / 2, desktop->height() / 5 * 4);
    ui->layoutWidget1->setGeometry(QRect(10, 17, desktop->width() / 2, 310));
    ui->nameLineEdit->setMinimumSize(desktop->width() / 3, 100);
    ui->hostComboBox->setMinimumSize(desktop->width() / 3, 150);
    ui->nameLabel->setMinimumSize(100, 20);
    ui->hostLabel->setMinimumSize(100, 20);
/*
    delete ui->formLayout;
    ui->formLayout = new QFormLayout(ui->layoutWidget1);
    ui->formLayout->setObjectName(QStringLiteral("formLayout"));
    ui->formLayout->setContentsMargins(0, 0, 0, 0);
    ui->formLayout->setWidget(0, QFormLayout::LabelRole, ui->nameLabel);
    ui->formLayout->setWidget(0, QFormLayout::FieldRole, ui->nameLineEdit);
    ui->formLayout->setWidget(1, QFormLayout::LabelRole, ui->hostLabel);
    ui->formLayout->setWidget(1, QFormLayout::FieldRole, ui->hostComboBox);
*/
    ui->avatarLabel->setGeometry(QRect(20, 320, 42, 80));
    ui->avatarPixmap->setGeometry(QRect(15, 400, 114, 136));
    ui->changeAvatarButton->setGeometry(QRect(140, 600, 400, 100));
    ui->detectLANButton->setGeometry(QRect(140, 400, 400, 100));
    ui->clearHistoryButton->setGeometry(QRect(140, 500, 400, 100));
    ui->frame->setGeometry(QRect(desktop->width() / 2 + 20, desktop->height() / 4, desktop->width(), desktop->height()));
    ui->reconnectionCheckBox->setGeometry(QRect(30, 10, 400, 60));
    ui->connectButton->setGeometry(QRect(30, 170, 300, 100));
    ui->cancelButton->setGeometry(QRect(30, 280, 300, 100));
#else
    resize(ShrinkWidth, height());
#endif

    ui->avatarList->hide();
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::hideAvatarList()
{
    if (!ui->avatarList->isVisible()) return;
    ui->avatarList->hide();
}

void ConnectionDialog::showAvatarList()
{
    if (ui->avatarList->isVisible()) return;

    if (ui->avatarList->model() == NULL) {
        QList<const General *> generals = Sanguosha->getGeneralList();
        QMutableListIterator<const General *> itor = generals;
        while (itor.hasNext()) {
            if (itor.next()->isTotallyHidden())
                itor.remove();
        }

        AvatarModel *model = new AvatarModel(generals);
        model->setParent(this);
        ui->avatarList->setModel(model);
    }
    ui->avatarList->show();
}

void ConnectionDialog::on_connectButton_clicked()
{
    QString username = ui->nameLineEdit->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    Config.UserName = username;
    Config.HostAddress = ui->hostComboBox->lineEdit()->text();

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostAddress", Config.HostAddress);
    Config.setValue("EnableReconnection", ui->reconnectionCheckBox->isChecked());

    accept();
}

void ConnectionDialog::on_changeAvatarButton_clicked()
{
    if (ui->avatarList->isVisible()) {
        QModelIndex index = ui->avatarList->currentIndex();
        if (index.isValid()) {
            on_avatarList_doubleClicked(index);
        } else {
            hideAvatarList();
#ifndef Q_OS_ANDROID
            resize(ShrinkWidth, height());
#endif
        }
    } else {
        showAvatarList();
        //Avoid violating the constraints
        //setFixedWidth(ExpandWidth);
#ifndef Q_OS_ANDROID
        resize(ExpandWidth, height());
#endif
    }
}

void ConnectionDialog::on_avatarList_doubleClicked(const QModelIndex &index)
{
    QString general_name = ui->avatarList->model()->data(index, Qt::UserRole).toString();
    QPixmap avatar(G_ROOM_SKIN.getGeneralPixmap(general_name, QSanRoomSkin::S_GENERAL_ICON_SIZE_LARGE));
    ui->avatarPixmap->setPixmap(avatar);
    Config.UserAvatar = general_name;
    Config.setValue("UserAvatar", general_name);
    hideAvatarList();
#ifndef Q_OS_ANDROID
    resize(ShrinkWidth, height());
#endif
}

void ConnectionDialog::on_clearHistoryButton_clicked()
{
    ui->hostComboBox->clear();
    ui->hostComboBox->lineEdit()->clear();

    Config.HistoryIPs.clear();
    Config.remove("HistoryIPs");
}

void ConnectionDialog::on_detectLANButton_clicked()
{
    UdpDetectorDialog *detector_dialog = new UdpDetectorDialog(this);
    connect(detector_dialog, &UdpDetectorDialog::address_chosen, ui->hostComboBox->lineEdit(), &QLineEdit::setText);

    detector_dialog->exec();
}
