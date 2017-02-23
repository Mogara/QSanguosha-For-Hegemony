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

#include "serverdialog.h"
#include "package.h"
#include "settings.h"
#include "engine.h"
#include "customassigndialog.h"
#include "banlistdialog.h"
#include "scenario.h"

#include <QTabWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QSet>
#include <QLabel>
#include <QRadioButton>
#include <QHostInfo>
#include <QComboBox>
class QFont;

static QLayout *HLay(QWidget *left, QWidget *right)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(left);
    layout->addWidget(right);
    return layout;
}

ServerDialog::ServerDialog(QWidget *parent)
    : FlatDialog(parent)
{
    setWindowTitle(tr("Start server"));

    QTabWidget *tab_widget = new QTabWidget;
    //changed by SE for ios
#if ((defined Q_OS_IOS) || (defined Q_OS_ANDROID))
    tab_widget->addTab(createGameModeTab(), tr("Game mode"));
#endif
    tab_widget->addTab(createBasicTab(), tr("Basic"));
    tab_widget->addTab(createPackageTab(), tr("Game Pacakge Selection"));
    tab_widget->addTab(createAdvancedTab(), tr("Advanced"));
    tab_widget->addTab(createConversionTab(), tr("Conversion Selection"));
#if ((defined Q_OS_IOS) || (defined Q_OS_ANDROID))
    tab_widget->addTab(createAiTab(), tr("AI"));
#endif
    tab_widget->addTab(createMiscTab(), tr("Miscellaneous"));

    layout->addWidget(tab_widget);
    layout->addLayout(createButtonLayout());

    //change by SE for ios
#ifdef Q_OS_IOS
    setMinimumSize(480, 320);
#elif defined Q_OS_ANDROID
    setMinimumSize(parent->width(), parent->height());
    setStyleSheet("background-color: #F0FFF0; color: black;");
#else
    setMinimumSize(574, 380);
#endif
}

QWidget *ServerDialog::createBasicTab()
{
    server_name_edit = new QLineEdit;
    server_name_edit->setText(Config.ServerName);

    timeout_spinbox = new QSpinBox;
    timeout_spinbox->setRange(5, 60);
    timeout_spinbox->setValue(Config.OperationTimeout);

    timeout_spinbox->setSuffix(tr(" seconds"));
    nolimit_checkbox = new QCheckBox(tr("No limit"));
    nolimit_checkbox->setChecked(Config.OperationNoLimit);
    timeout_spinbox->setDisabled(Config.OperationNoLimit);
    connect(nolimit_checkbox, &QCheckBox::toggled, timeout_spinbox, &QSpinBox::setDisabled);
#ifdef Q_OS_IOS
    pile_swapping_label = new QLabel(tr("Pile-swapping limitation"));
    pile_swapping_label->setToolTip(tr("<font color=%1>-1 means no limitations</font>").arg(Config.SkillDescriptionInToolTipColor.name()));
    pile_swapping_spinbox = new QSpinBox;
    pile_swapping_spinbox->setRange(-1, 15);
    pile_swapping_spinbox->setValue(Config.value("PileSwappingLimitation", 5).toInt());

    hegemony_maxchoice_label = new QLabel(tr("Upperlimit for hegemony"));
    hegemony_maxchoice_spinbox = new QSpinBox;
    hegemony_maxchoice_spinbox->setRange(5, 7); //wait for a new extension
    hegemony_maxchoice_spinbox->setValue(Config.value("HegemonyMaxChoice", 7).toInt());
#endif

    QPushButton *edit_button = new QPushButton(tr("Banlist ..."));
    edit_button->setFixedWidth(100);
    connect(edit_button, &QPushButton::clicked, this, &ServerDialog::editBanlist);

    QFormLayout *form_layout = new QFormLayout;
    form_layout->addRow(tr("Server name"), server_name_edit);


    QHBoxLayout *lay = new QHBoxLayout;
    lay->addWidget(timeout_spinbox);
#ifdef Q_OS_ANDROID
    timeout_spinbox->setMinimumHeight(80);
    timeout_slider = new QSlider(Qt::Horizontal);
    timeout_slider->setRange(5, 60);
    timeout_slider->setValue(Config.OperationTimeout);
    QObject::connect(timeout_slider, SIGNAL(valueChanged(int)), timeout_spinbox, SLOT(setValue(int)));
    QObject::connect(timeout_spinbox, SIGNAL(valueChanged(int)), timeout_slider, SLOT(setValue(int)));
    lay->addWidget(timeout_slider);
#endif
    lay->addWidget(nolimit_checkbox);
    lay->addWidget(edit_button);
    form_layout->addRow(tr("Operation timeout"), lay);
#ifdef Q_OS_IOS
    form_layout->addRow(HLay(pile_swapping_label, pile_swapping_spinbox));
    form_layout->addRow(HLay(hegemony_maxchoice_label, hegemony_maxchoice_spinbox));
#else
#ifndef Q_OS_ANDROID
    form_layout->addRow(createGameModeBox());
#endif
#endif


    QWidget *widget = new QWidget;
    widget->setLayout(form_layout);


    return widget;
}

#if ((defined Q_OS_IOS) || (defined Q_OS_ANDROID))
QWidget *ServerDialog::createGameModeTab()
{
    QFormLayout *form_layout = new QFormLayout;
    form_layout->addRow(createGameModeBox());

    QWidget *widget = new QWidget;
    widget->setLayout(form_layout);
    return widget;
}
#endif

QWidget *ServerDialog::createPackageTab()
{
    disable_lua_checkbox = new QCheckBox(tr("Disable Lua"));
    disable_lua_checkbox->setChecked(Config.DisableLua);
    disable_lua_checkbox->setToolTip(tr("<font color=%1>The setting takes effect after reboot</font>").arg(Config.SkillDescriptionInToolTipColor.name()));

    extension_group = new QButtonGroup(this);
    extension_group->setExclusive(false);

    QSet<QString> ban_packages = Config.BanPackages.toSet();

    QGroupBox *box1 = new QGroupBox(tr("General package"));
    QGroupBox *box2 = new QGroupBox(tr("Card package"));

    QGridLayout *layout1 = new QGridLayout;
    QGridLayout *layout2 = new QGridLayout;
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    int i = 0, j = 0;
    int row = 0, column = 0;
    const QList<const Package *> &packages = Sanguosha->getPackages();
    foreach (const Package *package, packages) {
        if (package->inherits("Scenario"))
            continue;

        const QString &extension = package->objectName();
        bool forbid_package = Config.value("ForbidPackages").toStringList().contains(extension);
        QCheckBox *checkbox = new QCheckBox;
#ifdef Q_OS_ANDROID
        QFont f = checkbox->font();
        f.setPointSize(6);
        checkbox->setFont(f);
#endif
        checkbox->setObjectName(extension);
        checkbox->setText(Sanguosha->translate(extension));
        checkbox->setChecked(!ban_packages.contains(extension) && !forbid_package);
        checkbox->setEnabled(!forbid_package);

        extension_group->addButton(checkbox);

        switch (package->getType()) {
        case Package::GeneralPack: {
            row = i / 5;
            column = i % 5;
            i++;

            layout1->addWidget(checkbox, row, column + 1);
            break;
        }
        case Package::CardPack: {
            row = j / 5;
            column = j % 5;
            j++;

            layout2->addWidget(checkbox, row, column + 1);
            break;
        }
        default:
            break;
        }
    }

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(disable_lua_checkbox);
    layout->addWidget(box1);
    layout->addWidget(box2);

    widget->setLayout(layout);
    return widget;
}

QWidget *ServerDialog::createAdvancedTab()
{
    QVBoxLayout *layout = new QVBoxLayout;

    forbid_same_ip_checkbox = new QCheckBox(tr("Forbid same IP with multiple connection"));
    forbid_same_ip_checkbox->setChecked(Config.ForbidSIMC);

    disable_chat_checkbox = new QCheckBox(tr("Disable chat"));
    disable_chat_checkbox->setChecked(Config.DisableChat);

    random_seat_checkbox = new QCheckBox(tr("Arrange the seats randomly"));
    random_seat_checkbox->setChecked(Config.RandomSeat);

    enable_cheat_checkbox = new QCheckBox(tr("Enable cheat"));
    enable_cheat_checkbox->setToolTip(tr("<font color=%1>This option enables the cheat menu</font>").arg(Config.SkillDescriptionInToolTipColor.name()));
    enable_cheat_checkbox->setChecked(Config.EnableCheat);

    free_choose_checkbox = new QCheckBox(tr("Choose generals and cards freely"));
    free_choose_checkbox->setChecked(Config.FreeChoose);
    free_choose_checkbox->setVisible(Config.EnableCheat);

    free_kingdom_checkbox = new QCheckBox(tr("Choose kingdom freely"));
    free_kingdom_checkbox->setChecked(Config.FreeKingdom);
    free_kingdom_checkbox->setVisible(Config.EnableCheat);

    connect(enable_cheat_checkbox, &QCheckBox::toggled, free_choose_checkbox, &QCheckBox::setVisible);
    connect(enable_cheat_checkbox, &QCheckBox::toggled, free_kingdom_checkbox, &QCheckBox::setVisible);

#if !defined(Q_OS_IOS)
    pile_swapping_label = new QLabel(tr("Pile-swapping limitation"));
    pile_swapping_label->setToolTip(tr("<font color=%1>-1 means no limitations</font>").arg(Config.SkillDescriptionInToolTipColor.name()));
    pile_swapping_spinbox = new QSpinBox;
#ifdef Q_OS_ANDROID
    pile_swapping_spinbox->setMinimumHeight(80);
#endif
    pile_swapping_spinbox->setRange(-1, 15);
    pile_swapping_spinbox->setValue(Config.value("PileSwappingLimitation", 5).toInt());

    hegemony_maxchoice_label = new QLabel(tr("Upperlimit for hegemony"));
    hegemony_maxchoice_spinbox = new QSpinBox;
#ifdef Q_OS_ANDROID
    hegemony_maxchoice_spinbox->setMinimumHeight(80);
#endif
    hegemony_maxchoice_spinbox->setRange(5, 7); //wait for a new extension
    hegemony_maxchoice_spinbox->setValue(Config.value("HegemonyMaxChoice", 7).toInt());
#endif
    address_edit = new QLineEdit;
    address_edit->setText(Config.Address);
    address_edit->setPlaceholderText(tr("Public IP or domain"));

    QPushButton *detect_button = new QPushButton(tr("Detect my WAN IP"));
    connect(detect_button, &QPushButton::clicked, this, &ServerDialog::onDetectButtonClicked);

    port_edit = new QLineEdit;
    port_edit->setText(QString::number(Config.ServerPort));
    port_edit->setValidator(new QIntValidator(1000, 65535, port_edit));

    layout->addLayout(HLay(forbid_same_ip_checkbox, disable_chat_checkbox));
    layout->addWidget(random_seat_checkbox);
    layout->addWidget(enable_cheat_checkbox);
    layout->addWidget(free_choose_checkbox);
    layout->addWidget(free_kingdom_checkbox);
#if !defined(Q_OS_IOS)
    layout->addLayout(HLay(pile_swapping_label, pile_swapping_spinbox));
    layout->addLayout(HLay(hegemony_maxchoice_label, hegemony_maxchoice_spinbox));
#endif
#ifndef Q_OS_ANDROID
    layout->addLayout(HLay(new QLabel(tr("Address")), address_edit));
    layout->addWidget(detect_button);
    layout->addLayout(HLay(new QLabel(tr("Port")), port_edit));
    layout->addStretch();
#endif
    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}

QWidget *ServerDialog::createConversionTab()
{
    QVBoxLayout *layout = new QVBoxLayout;

    bool enable_lord = Config.value("EnableLordConvertion", true).toBool();
    convert_lord = new QCheckBox(tr("Enable Lord Convertion"));
    convert_lord->setChecked(enable_lord);

    convert_ds_to_dp = new QCheckBox(tr("Convert DoubleSword to DragonPhoenix"));
    convert_ds_to_dp->setChecked(Config.value("CardConversions").toStringList().contains("DragonPhoenix") || enable_lord);
    convert_ds_to_dp->setDisabled(enable_lord);

    convert_jf_to_ps = new QCheckBox(tr("Convert JingFan to PeaceSpell"));
    convert_jf_to_ps->setChecked(Config.value("CardConversions").toStringList().contains("PeaceSpell") || enable_lord);
    convert_jf_to_ps->setDisabled(enable_lord);

    connect(convert_lord, &QCheckBox::toggled, convert_ds_to_dp, &QCheckBox::setChecked);
    connect(convert_lord, &QCheckBox::toggled, convert_ds_to_dp, &QCheckBox::setDisabled);
    connect(convert_lord, &QCheckBox::toggled, convert_jf_to_ps, &QCheckBox::setChecked);
    connect(convert_lord, &QCheckBox::toggled, convert_jf_to_ps, &QCheckBox::setDisabled);

    QWidget *widget = new QWidget;
    layout->addWidget(convert_lord);
    layout->addWidget(convert_ds_to_dp);
    layout->addWidget(convert_jf_to_ps);
    widget->setLayout(layout);
    return widget;
}

QWidget *ServerDialog::createMiscTab()
{
    game_start_spinbox = new QSpinBox;
    game_start_spinbox->setRange(0, 10);
    game_start_spinbox->setValue(Config.CountDownSeconds);
    game_start_spinbox->setSuffix(tr(" seconds"));

    nullification_spinbox = new QSpinBox;
    nullification_spinbox->setRange(5, 15);
    nullification_spinbox->setValue(Config.NullificationCountDown);
    nullification_spinbox->setSuffix(tr(" seconds"));

    minimize_dialog_checkbox = new QCheckBox(tr("Minimize the dialog when server runs"));
    minimize_dialog_checkbox->setChecked(Config.EnableMinimizeDialog);

    surrender_at_death_checkbox = new QCheckBox(tr("Surrender at the time of Death"));
    surrender_at_death_checkbox->setChecked(Config.SurrenderAtDeath);

    luck_card_label = new QLabel(tr("Upperlimit for use time of luck card"));
    luck_card_spinbox = new QSpinBox;
    luck_card_spinbox->setRange(0, 3);
    luck_card_spinbox->setValue(Config.LuckCardLimitation);

#ifdef Q_OS_ANDROID
    game_start_spinbox->setMinimumHeight(80);
    nullification_spinbox->setMinimumHeight(80);
    luck_card_spinbox->setMinimumHeight(80);
#endif

    reward_the_first_showing_player_checkbox = new QCheckBox(tr("The first player to show general can draw 2 cards"));
    reward_the_first_showing_player_checkbox->setChecked(Config.RewardTheFirstShowingPlayer);
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    QGroupBox *ai_groupbox = new QGroupBox(tr("Artificial intelligence"));
    ai_groupbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QVBoxLayout *layout = new QVBoxLayout;

    forbid_adding_robot_checkbox = new QCheckBox(tr("Forbid adding robot"));
    forbid_adding_robot_checkbox->setChecked(Config.ForbidAddingRobot);

    ai_chat_checkbox = new QCheckBox(tr("Enable AI chat"));
    ai_chat_checkbox->setChecked(Config.value("AIChat", true).toBool());
    ai_chat_checkbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_chat_checkbox, &QCheckBox::setDisabled);

    ai_delay_spinbox = new QSpinBox;
    ai_delay_spinbox->setMinimum(0);
    ai_delay_spinbox->setMaximum(5000);
    ai_delay_spinbox->setRange(0, 5000);
    ai_delay_spinbox->setValue(Config.OriginAIDelay);
    ai_delay_spinbox->setSuffix(tr(" millisecond"));
    ai_delay_spinbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_spinbox, &QSpinBox::setDisabled);

    ai_delay_altered_checkbox = new QCheckBox(tr("Alter AI Delay After Death"));
    ai_delay_altered_checkbox->setChecked(Config.AlterAIDelayAD);
    ai_delay_altered_checkbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_altered_checkbox, &QCheckBox::setDisabled);

    ai_delay_ad_spinbox = new QSpinBox;
    ai_delay_ad_spinbox->setMinimum(0);
    ai_delay_ad_spinbox->setMaximum(5000);
    ai_delay_ad_spinbox->setValue(Config.AIDelayAD);
    ai_delay_ad_spinbox->setSuffix(tr(" millisecond"));
    ai_delay_ad_spinbox->setEnabled(ai_delay_altered_checkbox->isChecked());
    ai_delay_ad_spinbox->setDisabled(Config.ForbidAddingRobot);
    connect(ai_delay_altered_checkbox, &QCheckBox::toggled, ai_delay_ad_spinbox, &QSpinBox::setEnabled);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_ad_spinbox, &QSpinBox::setDisabled);

    layout->addLayout(HLay(forbid_adding_robot_checkbox, ai_chat_checkbox));
    layout->addLayout(HLay(new QLabel(tr("AI delay")), ai_delay_spinbox));
    layout->addWidget(ai_delay_altered_checkbox);
    layout->addLayout(HLay(new QLabel(tr("AI delay After Death")), ai_delay_ad_spinbox));

    ai_groupbox->setLayout(layout);
#endif
    QVBoxLayout *tablayout = new QVBoxLayout;
    tablayout->addLayout(HLay(new QLabel(tr("Game start count down")), game_start_spinbox));
    tablayout->addLayout(HLay(new QLabel(tr("Nullification count down")), nullification_spinbox));
    tablayout->addLayout(HLay(minimize_dialog_checkbox, surrender_at_death_checkbox));
    tablayout->addLayout(HLay(luck_card_label, luck_card_spinbox));
    tablayout->addWidget(reward_the_first_showing_player_checkbox);
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    tablayout->addWidget(ai_groupbox);
#endif
    tablayout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(tablayout);
    return widget;
}

#if ((defined Q_OS_IOS) || (defined Q_OS_ANDROID))
QWidget *ServerDialog::createAiTab()
{
    QVBoxLayout *layout = new QVBoxLayout;

    forbid_adding_robot_checkbox = new QCheckBox(tr("Forbid adding robot"));
    forbid_adding_robot_checkbox->setChecked(Config.ForbidAddingRobot);

    ai_chat_checkbox = new QCheckBox(tr("Enable AI chat"));
    ai_chat_checkbox->setChecked(Config.value("AIChat", true).toBool());
    ai_chat_checkbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_chat_checkbox, &QCheckBox::setDisabled);

    ai_delay_spinbox = new QSpinBox;
    ai_delay_spinbox->setMinimum(0);
    ai_delay_spinbox->setMinimumHeight(80);
    ai_delay_spinbox->setMaximum(5000);
    ai_delay_spinbox->setValue(Config.OriginAIDelay);
    ai_delay_spinbox->setSuffix(tr(" millisecond"));
    ai_delay_spinbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_spinbox, &QSpinBox::setDisabled);

    ai_deley_slider = new QSlider(Qt::Horizontal);
    ai_deley_slider->setRange(0, 5000);
    ai_deley_slider->setValue(Config.OriginAIDelay);
    QObject::connect(ai_deley_slider, SIGNAL(valueChanged(int)), ai_delay_spinbox, SLOT(setValue(int)));
    QObject::connect(ai_delay_spinbox, SIGNAL(valueChanged(int)), ai_deley_slider, SLOT(setValue(int)));

    ai_delay_altered_checkbox = new QCheckBox(tr("Alter AI Delay After Death"));
    ai_delay_altered_checkbox->setChecked(Config.AlterAIDelayAD);
    ai_delay_altered_checkbox->setDisabled(Config.ForbidAddingRobot);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_altered_checkbox, &QCheckBox::setDisabled);

    ai_delay_ad_spinbox = new QSpinBox;
    ai_delay_ad_spinbox->setMinimumHeight(80);
    ai_delay_ad_spinbox->setMinimum(0);
    ai_delay_ad_spinbox->setMaximum(5000);
    ai_delay_ad_spinbox->setValue(Config.AIDelayAD);
    ai_delay_ad_spinbox->setSuffix(tr(" millisecond"));
    ai_delay_ad_spinbox->setEnabled(ai_delay_altered_checkbox->isChecked());
    ai_delay_ad_spinbox->setDisabled(Config.ForbidAddingRobot);
    connect(ai_delay_altered_checkbox, &QCheckBox::toggled, ai_delay_ad_spinbox, &QSpinBox::setEnabled);
    connect(forbid_adding_robot_checkbox, &QCheckBox::toggled, ai_delay_ad_spinbox, &QSpinBox::setDisabled);

    ai_delay_ad_slider = new QSlider(Qt::Horizontal);
    ai_delay_ad_slider->setRange(0, 5000);
    ai_delay_ad_slider->setValue(Config.AIDelayAD);
    QObject::connect(ai_delay_ad_slider, SIGNAL(valueChanged(int)), ai_delay_ad_spinbox, SLOT(setValue(int)));
    QObject::connect(ai_delay_ad_spinbox, SIGNAL(valueChanged(int)), ai_delay_ad_slider, SLOT(setValue(int)));

    layout->addLayout(HLay(forbid_adding_robot_checkbox, ai_chat_checkbox));
    layout->addLayout(HLay(new QLabel(tr("AI delay")), ai_delay_spinbox));
    layout->addWidget(ai_deley_slider);
    layout->addWidget(ai_delay_altered_checkbox);
    layout->addLayout(HLay(new QLabel(tr("AI delay After Death")), ai_delay_ad_spinbox));
    layout->addWidget(ai_delay_ad_slider);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    return widget;
}
#endif


QGroupBox *ServerDialog::createGameModeBox()
{
    QGroupBox *mode_box = new QGroupBox(tr("Game mode"));
    mode_box->setParent(this);
    mode_group = new QButtonGroup(this);

    QObjectList item_list;

    // normal modes
    QMap<QString, QString> modes = Sanguosha->getAvailableModes();
    QMapIterator<QString, QString> itor(modes);
    while (itor.hasNext()) {
        itor.next();

        QRadioButton *button = new QRadioButton(itor.value());
        button->setObjectName(itor.key());
        mode_group->addButton(button);

        item_list << button;

        if (itor.key() == Config.GameMode)
            button->setChecked(true);
    }

    // add scenario modes
    QRadioButton *scenario_button = new QRadioButton(tr("Scenario mode"));
    scenario_button->setObjectName("scenario");
    mode_group->addButton(scenario_button);
    item_list << scenario_button;

    scenario_ComboBox = new QComboBox;
    scenario_ComboBox->setFocusPolicy(Qt::WheelFocus);
    scenario_ComboBox->setEnabled(scenario_button->isDown());
    connect(scenario_button,&QRadioButton::toggled,scenario_ComboBox,&QComboBox::setEnabled);
    QStringList names = Sanguosha->getModScenarioNames();
    foreach (QString name, names) {
        QString scenario_name = Sanguosha->translate(name);
        const Scenario *scenario = Sanguosha->getScenario(name);
        if (scenario){ //crash,sometimes.
            QString text = tr("%1 (%2 persons)").arg(scenario_name).arg(scenario->getPlayerCount());
            scenario_ComboBox->addItem(text,name);
        }
    }
    item_list << scenario_ComboBox;

    if (mode_group->checkedButton() == NULL) {
        int index = names.indexOf(Config.GameMode);
        if (index != -1) {
            scenario_button->setChecked(true);
            scenario_ComboBox->setCurrentIndex(index);
        } else 
            mode_group->buttons().first()->setChecked(true); // for Lua Scenario.
    }
    
    // ============

    QVBoxLayout *left = new QVBoxLayout;
    QVBoxLayout *right = new QVBoxLayout;

    for (int i = 0; i < item_list.length(); i++) {
        QObject *item = item_list.at(i);

        QVBoxLayout *side = i < (item_list.length() + 1) / 2 ? left : right;

        if (item->isWidgetType()) {
            QWidget *widget = qobject_cast<QWidget *>(item);
            side->addWidget(widget);
        } else {
            QLayout *item_layout = qobject_cast<QLayout *>(item);
            side->addLayout(item_layout);
        }
        if (i == item_list.length() / 2)
            side->addStretch();
    }

    right->addStretch();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addLayout(left);
    layout->addLayout(right);

    mode_box->setLayout(layout);

    return mode_box;
}

QLayout *ServerDialog::createButtonLayout()
{
    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addStretch();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    QPushButton *cancel_button = new QPushButton(tr("Cancel"));

    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);

    connect(ok_button, &QPushButton::clicked, this, &ServerDialog::onOkButtonClicked);
    connect(cancel_button, &QPushButton::clicked, this, &ServerDialog::reject);

    return button_layout;
}

void ServerDialog::onDetectButtonClicked()
{
    QHostInfo vHostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    QList<QHostAddress> vAddressList = vHostInfo.addresses();
    foreach (const QHostAddress &address, vAddressList) {
        if (!address.isNull() && address != QHostAddress::LocalHost
            && address.protocol() == QAbstractSocket::IPv4Protocol) {
            address_edit->setText(address.toString());
            return;
        }
    }
}

void ServerDialog::onOkButtonClicked()
{
    accept();
}


bool ServerDialog::config()
{
    exec();

    if (result() != Accepted)
        return false;

    Config.ServerName = server_name_edit->text();
    Config.OperationTimeout = timeout_spinbox->value();
    Config.OperationNoLimit = nolimit_checkbox->isChecked();
    Config.RandomSeat = random_seat_checkbox->isChecked();
    Config.EnableCheat = enable_cheat_checkbox->isChecked();
    Config.FreeChoose = Config.EnableCheat && free_choose_checkbox->isChecked();
    Config.FreeKingdom = Config.EnableCheat && free_kingdom_checkbox->isChecked();
    Config.ForbidSIMC = forbid_same_ip_checkbox->isChecked();
    Config.DisableChat = disable_chat_checkbox->isChecked();
    Config.Address = address_edit->text();
    Config.CountDownSeconds = game_start_spinbox->value();
    Config.NullificationCountDown = nullification_spinbox->value();
    Config.EnableMinimizeDialog = minimize_dialog_checkbox->isChecked();
    Config.RewardTheFirstShowingPlayer = reward_the_first_showing_player_checkbox->isChecked();
    Config.ForbidAddingRobot = forbid_adding_robot_checkbox->isChecked();
    Config.OriginAIDelay = ai_delay_spinbox->value();
    Config.AIDelay = Config.OriginAIDelay;
    Config.AIDelayAD = ai_delay_ad_spinbox->value();
    Config.AlterAIDelayAD = ai_delay_altered_checkbox->isChecked();
    Config.ServerPort = port_edit->text().toUShort();
    Config.DisableLua = disable_lua_checkbox->isChecked();
    Config.SurrenderAtDeath = surrender_at_death_checkbox->isChecked();
    Config.LuckCardLimitation = luck_card_spinbox->value();

    // game mode

    if (mode_group->checkedButton()) {
        QString objname = mode_group->checkedButton()->objectName();
        if (objname == "scenario")
            Config.GameMode = scenario_ComboBox->itemData(scenario_ComboBox->currentIndex()).toString();
        else
            Config.GameMode = objname;
    }

    Config.setValue("ServerName", Config.ServerName);
    Config.setValue("GameMode", Config.GameMode);
    Config.setValue("OperationTimeout", Config.OperationTimeout);
    Config.setValue("OperationNoLimit", Config.OperationNoLimit);
    Config.setValue("RandomSeat", Config.RandomSeat);
    Config.setValue("EnableCheat", Config.EnableCheat);
    Config.setValue("FreeChoose", Config.FreeChoose);
    Config.setValue("FreeKingdom", Config.FreeKingdom);
    Config.setValue("PileSwappingLimitation", pile_swapping_spinbox->value());
    Config.setValue("ForbidSIMC", Config.ForbidSIMC);
    Config.setValue("DisableChat", Config.DisableChat);
    Config.setValue("HegemonyMaxChoice", hegemony_maxchoice_spinbox->value());
    Config.setValue("CountDownSeconds", game_start_spinbox->value());
    Config.setValue("NullificationCountDown", nullification_spinbox->value());
    Config.setValue("EnableMinimizeDialog", Config.EnableMinimizeDialog);
    Config.setValue("RewardTheFirstShowingPlayer", Config.RewardTheFirstShowingPlayer);
    Config.setValue("ForbidAddingRobot", Config.ForbidAddingRobot);
    Config.setValue("OriginAIDelay", Config.OriginAIDelay);
    Config.setValue("AlterAIDelayAD", ai_delay_altered_checkbox->isChecked());
    Config.setValue("AIDelayAD", Config.AIDelayAD);
    Config.setValue("SurrenderAtDeath", Config.SurrenderAtDeath);
    Config.setValue("LuckCardLimitation", Config.LuckCardLimitation);
    Config.setValue("ServerPort", Config.ServerPort);
    Config.setValue("Address", Config.Address);
    Config.setValue("DisableLua", disable_lua_checkbox->isChecked());
    Config.setValue("AIChat", ai_chat_checkbox->isChecked());

    QSet<QString> ban_packages;
    QList<QAbstractButton *> checkboxes = extension_group->buttons();
    foreach (QAbstractButton *checkbox, checkboxes) {
        if (!checkbox->isChecked()) {
            QString package_name = checkbox->objectName();
            Sanguosha->addBanPackage(package_name);
            ban_packages.insert(package_name);
        }
    }

    Config.BanPackages = ban_packages.toList();
    Config.setValue("BanPackages", Config.BanPackages);
    Config.setValue("EnableLordConvertion", convert_lord->isChecked());

    QStringList card_conversions;
    if (convert_ds_to_dp->isChecked()) card_conversions << "DragonPhoenix";
    if (convert_jf_to_ps->isChecked()) card_conversions << "PeaceSpell";
    Config.setValue("CardConversions", card_conversions);

    return true;
}

void ServerDialog::editBanlist()
{
    BanListDialog *dialog = new BanListDialog(this);
    dialog->exec();
}
