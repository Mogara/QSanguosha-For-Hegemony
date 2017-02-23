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

#include "roomscene.h"
#include "settings.h"
#include "carditem.h"
#include "engine.h"
#include "cardoverview.h"
#include "distanceviewdialog.h"
#include "freechoosedialog.h"
#include "window.h"
#include "button.h"
#include "cardcontainer.h"
#include "recorder.h"
#include "indicatoritem.h"
#include "pixmapanimation.h"
#include "audio.h"
#include "skinbank.h"
#include "record-analysis.h"
#include "choosegeneralbox.h"
#include "chooseoptionsbox.h"
#include "choosetriggerorderbox.h"
#include "uiutils.h"
#include "qsanbutton.h"
#include "guanxingbox.h"
#include "bubblechatbox.h"
#include "playercardbox.h"
#include "stylehelper.h"
#include "heroskincontainer.h"
#include "flatdialog.h"
#include "choosesuitbox.h"
#include "guhuobox.h"
#include "cardchoosebox.h"
#include "pindianbox.h"
#include "transformation.h"
#include "lightboxanimation.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QCheckBox>
#include <QGraphicsLinearLayout>
#include <QMenu>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QFileDialog>
#include <QDesktopServices>
#include <QRadioButton>
#include <QApplication>
#include <QTimer>
#include <QCommandLinkButton>
#include <QFormLayout>
#include <QCoreApplication>
#include <QInputDialog>
#include <QScrollBar>

using namespace QSanProtocol;

RoomScene *RoomSceneInstance;

void RoomScene::resetPiles()
{
    // @todo: fix this...
}

RoomScene::RoomScene(QMainWindow *main_window)
    : game_started(false), main_window(main_window)
{
    setItemIndexMethod(NoIndex);
    setParent(main_window);

    m_choiceDialog = NULL;
    RoomSceneInstance = this;
    _m_last_front_item = NULL;
    _m_last_front_ZValue = 0;
    int player_count = Sanguosha->getPlayerCount(ServerInfo.GameMode);

    m_isInDragAndUseMode = false;
    m_superDragStarted = false;
    m_mousePressed = false;

    _m_roomSkin = &(QSanSkinFactory::getInstance().getCurrentSkinScheme().getRoomSkin());
    _m_roomLayout = &(G_ROOM_SKIN.getRoomLayout());
    _m_photoLayout = &(G_ROOM_SKIN.getPhotoLayout());
    _m_commonLayout = &(G_ROOM_SKIN.getCommonLayout());

    m_skillButtonSank = false;

    setBackgroundBrush(QBrush(QPixmap(Config.TableBgImage)));

    connect(this, &RoomScene::sceneRectChanged, this, &RoomScene::onSceneRectChanged);

    current_guhuo_box = NULL;

    // create photos
    for (int i = 0; i < player_count - 1; i++) {
        Photo *photo = new Photo;
        photos << photo;
        addItem(photo);
        photo->setZValue(-0.5);
    }

    // create table pile
    m_tablePile = new TablePile;
    addItem(m_tablePile);
    connect(ClientInstance, &Client::card_used, m_tablePile, (void (TablePile::*)())(&TablePile::clear));

    // create dashboard
    dashboard = new Dashboard(createDashboardButtons());
    dashboard->setObjectName("dashboard");
    dashboard->setZValue(0.8);
    addItem(dashboard);

    dashboard->setPlayer(Self);
    connect(Self, &ClientPlayer::general_changed, dashboard, &Dashboard::updateAvatar);
    connect(Self, &ClientPlayer::general2_changed, dashboard, &Dashboard::updateSmallAvatar);
    connect(dashboard, &Dashboard::card_selected, this, &RoomScene::enableTargets);
    connect(dashboard, &Dashboard::card_to_use, this, &RoomScene::doOkButton);

    connect(Self, &ClientPlayer::pile_changed, dashboard, &Dashboard::updatePile);

    // add role ComboBox
    connect(Self, &ClientPlayer::kingdom_changed, dashboard, &Dashboard::updateKingdom);

    m_replayControl = NULL;
    if (ClientInstance->getReplayer()) {
        dashboard->hideControlButtons();
        createReplayControlBar();
    }

    response_skill = new ResponseSkill;
    response_skill->setParent(this);
    showorpindian_skill = new ShowOrPindianSkill;
    showorpindian_skill->setParent(this);
    discard_skill = new DiscardSkill;
    discard_skill->setParent(this);
    yiji_skill = new YijiViewAsSkill;
    yiji_skill->setParent(this);
    choose_skill = new ChoosePlayerSkill;
    choose_skill->setParent(this);

    exchange_skill = new ExchangeSkill;
    exchange_skill->setParent(this);

    miscellaneous_menu = new QMenu(main_window);

    // do signal-slot connections
    connect(ClientInstance, &Client::player_added, this, &RoomScene::addPlayer);
    connect(ClientInstance, &Client::player_removed, this, &RoomScene::removePlayer);
    connect(ClientInstance, &Client::generals_got, this, &RoomScene::chooseGeneral);
    connect(ClientInstance, &Client::generals_viewed, this, &RoomScene::viewGenerals);
    connect(ClientInstance, &Client::suits_got, this, &RoomScene::chooseSuit);
    connect(ClientInstance, &Client::options_got, this, &RoomScene::chooseOption);
    connect(ClientInstance, &Client::cards_got, this, &RoomScene::chooseCard);
    //connect(ClientInstance, &Client::roles_got, this, &RoomScene::chooseRole);
    //connect(ClientInstance, SIGNAL(directions_got()), this, SLOT(chooseDirection()));
    //connect(ClientInstance, SIGNAL(orders_got(QSanProtocol::Game3v3ChooseOrderCommand)), this, SLOT(chooseOrder(QSanProtocol::Game3v3ChooseOrderCommand)));
    connect(ClientInstance, &Client::kingdoms_got, this, &RoomScene::chooseKingdom);
    connect(ClientInstance, &Client::triggers_got, this, &RoomScene::chooseTriggerOrder);
    connect(ClientInstance, &Client::seats_arranged, this, &RoomScene::arrangeSeats);
    connect(ClientInstance, &Client::status_changed, this, &RoomScene::updateStatus);
    connect(ClientInstance, &Client::avatars_hiden, this, &RoomScene::hideAvatars);
    connect(ClientInstance, &Client::hp_changed, this, &RoomScene::changeHp);
    connect(ClientInstance, &Client::maxhp_changed, this, &RoomScene::changeMaxHp);
    connect(ClientInstance, &Client::pile_reset, this, &RoomScene::resetPiles);
    connect(ClientInstance, &Client::player_killed, this, &RoomScene::killPlayer);
    connect(ClientInstance, &Client::player_revived, this, &RoomScene::revivePlayer);
    connect(ClientInstance, &Client::dashboard_death, this, &RoomScene::setDashboardShadow);
    connect(ClientInstance, &Client::card_shown, this, &RoomScene::showCard);
    connect(ClientInstance, &Client::gongxin, this, &RoomScene::doGongxin);
    connect(ClientInstance, &Client::focus_moved, this, &RoomScene::moveFocus);
    connect(ClientInstance, &Client::emotion_set, this, (void (RoomScene::*)(const QString &, const QString &, bool, int))(&RoomScene::setEmotion));
    connect(ClientInstance, &Client::skill_invoked, this, &RoomScene::showSkillInvocation);
    connect(ClientInstance, &Client::skill_acquired, this, &RoomScene::acquireSkill);
    connect(ClientInstance, &Client::animated, this, &RoomScene::doAnimation);
    connect(ClientInstance, &Client::event_received, this, &RoomScene::handleGameEvent);
    connect(ClientInstance, &Client::game_started, this, &RoomScene::onGameStart);
    connect(ClientInstance, &Client::game_over, this, &RoomScene::onGameOver);
    connect(ClientInstance, &Client::standoff, this, &RoomScene::onStandoff);
    connect(ClientInstance, &Client::move_cards_lost, this, &RoomScene::loseCards);
    connect(ClientInstance, &Client::move_cards_got, this, &RoomScene::getCards);
    connect(ClientInstance, &Client::nullification_asked, dashboard, &Dashboard::controlNullificationButton);
    connect(ClientInstance, &Client::start_in_xs, this, &RoomScene::startInXs);
    connect(ClientInstance, &Client::update_handcard_num, this, &RoomScene::updateHandcardNum);

    m_guanxingBox = new GuanxingBox;
    m_guanxingBox->hide();
    addItem(m_guanxingBox);
    m_guanxingBox->setZValue(20000.0);

    connect(ClientInstance, &Client::guanxing, m_guanxingBox, &GuanxingBox::doGuanxing);
    connect(ClientInstance, &Client::mirror_guanxing_start, m_guanxingBox, &GuanxingBox::mirrorGuanxingStart);
    connect(ClientInstance, &Client::mirror_guanxing_move, m_guanxingBox, &GuanxingBox::mirrorGuanxingMove);
    connect(ClientInstance, &Client::mirror_guanxing_finish, m_guanxingBox, &GuanxingBox::clear);
    m_guanxingBox->moveBy(-120, 0);

    m_cardchooseBox = new CardChooseBox;
    m_cardchooseBox->hide();
    addItem(m_cardchooseBox);
    m_cardchooseBox->setZValue(20000.0);

    connect(ClientInstance, &Client::cardchoose, m_cardchooseBox, &CardChooseBox::doCardChoose);
    connect(ClientInstance, &Client::mirror_cardchoose_start, m_cardchooseBox, &CardChooseBox::mirrorCardChooseStart);
    connect(ClientInstance, &Client::mirror_cardchoose_move, m_cardchooseBox, &CardChooseBox::mirrorCardChooseMove);
    connect(ClientInstance, &Client::mirror_cardchoose_finish, m_cardchooseBox, &CardChooseBox::clear);
    connect(ClientInstance, &Client::card_moved_incardchoosebox, this, &RoomScene::cardMovedinCardchooseBox);

    m_pindianBox = new PindianBox;
    m_pindianBox->hide();
    addItem(m_pindianBox);
    m_pindianBox->setZValue(9);
    m_pindianBox->moveBy(-120, 0);
    connect(ClientInstance, &Client::startPindian, m_pindianBox, &PindianBox::doPindian);
    connect(ClientInstance, &Client::onPindianReply, m_pindianBox, &PindianBox::onReply);
    connect(ClientInstance, &Client::pindianSuccess, this, &RoomScene::playPindianSuccess);

    m_chooseGeneralBox = new ChooseGeneralBox;
    m_chooseGeneralBox->hide();
    addItem(m_chooseGeneralBox);
    m_chooseGeneralBox->setZValue(30000.0);
    m_chooseGeneralBox->moveBy(-120, 0);

    m_chooseOptionsBox = new ChooseOptionsBox;
    m_chooseOptionsBox->hide();
    addItem(m_chooseOptionsBox);
    m_chooseOptionsBox->setZValue(30000.0);
    m_chooseOptionsBox->moveBy(-120, 0);

    m_chooseTriggerOrderBox = new ChooseTriggerOrderBox;
    m_chooseTriggerOrderBox->hide();
    addItem(m_chooseTriggerOrderBox);
    m_chooseTriggerOrderBox->setZValue(30000.0);
    m_chooseTriggerOrderBox->moveBy(-120, 0);

    m_playerCardBox = new PlayerCardBox;
    m_playerCardBox->hide();
    addItem(m_playerCardBox);
    m_playerCardBox->setZValue(30000.0);
    m_playerCardBox->moveBy(-120, 0);

    m_chooseSuitBox = new ChooseSuitBox;
    m_chooseSuitBox->hide();
    addItem(m_chooseSuitBox);
    m_chooseSuitBox->setZValue(30000.0);
    m_chooseSuitBox->moveBy(-120, 0);

    m_cardContainer = new CardContainer;
    m_cardContainer->hide();
    addItem(m_cardContainer);
    m_cardContainer->setZValue(9.0);

    pileContainer = new CardContainer;
    pileContainer->hide();
    addItem(pileContainer);
    pileContainer->setZValue(9.0);

    connect(m_cardContainer, &CardContainer::item_chosen, ClientInstance, &Client::onPlayerChooseAG);
    connect(m_cardContainer, &CardContainer::item_gongxined, ClientInstance, &Client::onPlayerReplyGongxin);
    connect(ClientInstance, &Client::ag_filled, this, &RoomScene::fillCards);
    connect(ClientInstance, &Client::ag_taken, this, &RoomScene::takeAmazingGrace);
    connect(ClientInstance, &Client::ag_cleared, m_cardContainer, &CardContainer::clear);

    m_cardContainer->moveBy(-120, 0);

    connect(ClientInstance, &Client::skill_attached, this, &RoomScene::attachSkill);
    connect(ClientInstance, &Client::skill_detached, this, &RoomScene::detachSkill);

    // chat box
    chatBox = new QTextEdit;
    chatBox->setObjectName("chat_box");
    chat_box_widget = addWidget(chatBox);
    chat_box_widget->setZValue(-2.0);
    chat_box_widget->setObjectName("chat_box_widget");
    chatBox->setReadOnly(true);
    chatBox->setTextColor(Config.TextEditColor);
    connect(ClientInstance, &Client::lineSpoken, chatBox, &QTextEdit::append);
    connect(ClientInstance, &Client::playerSpoke, this, &RoomScene::showBubbleChatBox);

    QScrollBar *bar = chatBox->verticalScrollBar();
    bar->setStyleSheet(StyleHelper::styleSheetOfScrollBar());

    // chat edit
    chatEdit = new QLineEdit;
    chatEdit->setObjectName("chat_edit");
    chatEdit->setMaxLength(500);
    chat_edit_widget = addWidget(chatEdit);
    chat_edit_widget->setObjectName("chat_edit_widget");
    chat_edit_widget->setZValue(-2.0);
    connect(chatEdit, &QLineEdit::returnPressed, this, &RoomScene::speak);
    chatEdit->setPlaceholderText(tr("Please enter text to chat ... "));

    chat_widget = new ChatWidget();
    chat_widget->setZValue(-0.1);
    addItem(chat_widget);
    connect(chat_widget, &ChatWidget::return_button_click, this, &RoomScene::speak);
    connect(chat_widget, &ChatWidget::chat_widget_msg, this, &RoomScene::appendChatEdit);

    if (ServerInfo.DisableChat)
        chat_edit_widget->hide();

    // log box
    log_box = new ClientLogBox;
    log_box->setTextColor(Config.TextEditColor);
    log_box->setObjectName("log_box");

#ifndef Q_OS_IOS
    //change font because of iphone screen's disharmony
    QFont qf1 = Config.iosLogFont;
    qf1.setPixelSize(11);
    log_box->setFont(qf1);
#endif
    log_box_widget = addWidget(log_box);
    log_box_widget->setObjectName("log_box_widget");
    log_box_widget->setZValue(-1.0);
    log_box_widget->setParent(this);
#ifdef Q_OS_ANDROID
    log_box->setStyleSheet("background-color: transparent");
    //log_box->setAttribute(Qt::WA_TranslucentBackground);
    log_box->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    log_box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    log_box->setLineWrapMode(QTextEdit::NoWrap);
#endif
    connect(ClientInstance, &Client::log_received, log_box, (void (ClientLogBox::*)(const QStringList &))(&ClientLogBox::appendLog));

    prompt_box = new Window(tr("QSanguosha"), QSize(480, 200));
    prompt_box->setOpacity(0);
    prompt_box->setFlag(QGraphicsItem::ItemIsMovable);
    prompt_box->shift();
    prompt_box->setZValue(10);
    prompt_box->keepWhenDisappear();

    prompt_box_widget = new QGraphicsTextItem(prompt_box);
    prompt_box_widget->setParent(prompt_box);
    prompt_box_widget->setPos(40, 45);
    prompt_box_widget->setDefaultTextColor(Qt::white);

    QTextDocument *prompt_doc = ClientInstance->getPromptDoc();
    prompt_doc->setTextWidth(prompt_box->boundingRect().width() - 80);
    prompt_box_widget->setDocument(prompt_doc);

    QFont qf = Config.SmallFont;
    qf.setPixelSize(21);
    qf.setStyleStrategy(QFont::PreferAntialias);
    prompt_box_widget->setFont(qf);

    addItem(prompt_box);

    m_rolesBoxBackground.load("image/system/state.png");
    m_rolesBox = new QGraphicsPixmapItem;
    addItem(m_rolesBox);
    m_pileCardNumInfoTextBox = addText("");
    m_pileCardNumInfoTextBox->setParentItem(m_rolesBox);
    m_pileCardNumInfoTextBox->setDocument(ClientInstance->getLinesDoc());
    m_pileCardNumInfoTextBox->setDefaultTextColor(Config.TextEditColor);

    add_robot = NULL;
    fill_robots = NULL;
    return_to_start_scene = NULL;
    if (!ServerInfo.ForbidAddingRobot) {
#ifdef Q_OS_ANDROID
        int width = G_DASHBOARD_LAYOUT.m_avatarArea.width() * 4;
        int height = G_DASHBOARD_LAYOUT.m_normalHeight * 2;
        control_panel = addRect(0, 0, width, height, Qt::NoPen);
        control_panel->hide();

        add_robot = new Button(tr("Add a robot"), QSizeF(width - 20, height / 3));
        add_robot->setParentItem(control_panel);
        add_robot->setPos(0, -add_robot->boundingRect().height() - 10);

        fill_robots = new Button(tr("Fill robots"), QSizeF(width - 20, height / 3));
        fill_robots->setParentItem(control_panel);
#else

        control_panel = addRect(0, 0, 500, 150, Qt::NoPen);
        control_panel->hide();

        add_robot = new Button(tr("Add a robot"), 1.0);
        add_robot->setParentItem(control_panel);
        add_robot->setTransform(QTransform::fromTranslate(-add_robot->boundingRect().width() / 2, -add_robot->boundingRect().height() / 2), true);
        add_robot->setPos(0, -add_robot->boundingRect().height() - 10);

        fill_robots = new Button(tr("Fill robots"), 1.0);
        fill_robots->setParentItem(control_panel);
        fill_robots->setTransform(QTransform::fromTranslate(-fill_robots->boundingRect().width() / 2, -fill_robots->boundingRect().height() / 2), true);
#endif
        connect(add_robot, &Button::clicked, ClientInstance, &Client::addRobot);
        connect(fill_robots, &Button::clicked, ClientInstance, &Client::fillRobots);
        connect(Self, &ClientPlayer::owner_changed, this, &RoomScene::showOwnerButtons);
    } else {
        control_panel = NULL;
    }

#ifdef Q_OS_ANDROID
    int width = G_DASHBOARD_LAYOUT.m_avatarArea.width() * 4;
    int height = G_DASHBOARD_LAYOUT.m_normalHeight * 2;
    return_to_start_scene = new Button(tr("Return to main menu"), QSizeF(width - 20, height / 3));
    addItem(return_to_start_scene);
    return_to_start_scene->setZValue(10000);
#else

    return_to_start_scene = new Button(tr("Return to main menu"), 1.0);

    addItem(return_to_start_scene);
    return_to_start_scene->setZValue(10000);
    return_to_start_scene->setTransform(QTransform::fromTranslate(-return_to_start_scene->boundingRect().width() / 2, -return_to_start_scene->boundingRect().height() / 2), true);
#endif
    connect(return_to_start_scene, &Button::clicked, this, &RoomScene::return_to_start);

    animations = new EffectAnimation();
    animations->setParent(this);

    pausing_item = new QGraphicsRectItem;
    pausing_text = new QGraphicsSimpleTextItem(tr("Paused ..."));
    addItem(pausing_item);
    addItem(pausing_text);

    pausing_item->setOpacity(0.36);
    pausing_item->setZValue(1002.0);

    QFont font = Config.BigFont;
    font.setPixelSize(100);
    pausing_text->setFont(font);
    pausing_text->setBrush(Qt::white);
    pausing_text->setZValue(1002.1);

    pausing_item->hide();
    pausing_text->hide();
}

void RoomScene::handleGameEvent(const QVariant &args)
{
    JsonArray arg = args.value<JsonArray>();
    if (arg.isEmpty())
        return;

    GameEventType eventType = (GameEventType)arg[0].toInt();
    switch (eventType) {
        case S_GAME_EVENT_PLAYER_DYING: {
            ClientPlayer *player = ClientInstance->getPlayer(arg[1].toString());
            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->setSaveMeIcon(true);
            Photo *photo = qobject_cast<Photo *>(container);
            if (photo) photo->setFrame(Photo::S_FRAME_SOS);

            QString sos_effect = "male_sos";
            if (!player->isMale()) {
                int index = qrand() % 2 + 1;
                sos_effect = "female_sos" + QString::number(index);
            }

            Sanguosha->playSystemAudioEffect(sos_effect);

            break;
        }
        case S_GAME_EVENT_PLAYER_QUITDYING: {
            ClientPlayer *player = ClientInstance->getPlayer(arg[1].toString());
            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->setSaveMeIcon(false);
            Photo *photo = qobject_cast<Photo *>(container);
            if (photo) photo->setFrame(Photo::S_FRAME_NO_FRAME);
            break;
        }
        case S_GAME_EVENT_HUASHEN: {
            ClientPlayer *player = ClientInstance->getPlayer(arg[1].toString());
            QStringList huashenGenerals = arg[2].toString().split("+");
            Q_ASSERT(player != NULL);
            Q_ASSERT(!huashenGenerals.isEmpty());
            PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(Player::PlaceHand, player);
            container->startHuaShen(huashenGenerals);
            player->tag["Huashens"] = huashenGenerals;
            player->changePile("huashencard", false, QList<int>());
            break;
        }
        case S_GAME_EVENT_PLAY_EFFECT: {
            QString skillName = arg[1].toString();
            QString category;
            if (JsonUtils::isBool(arg[2])) {
                bool isMale = arg[2].toBool();
                category = isMale ? "male" : "female";
            } else if (JsonUtils::isString(arg[2])) {
                category = arg[2].toString();
            }
            int type = arg[3].toInt();
            const ClientPlayer *player = NULL;
            if (arg.size() >= 5 && JsonUtils::isString(arg[4])) {
                QString playerName = arg[4].toString();
                player = ClientInstance->getPlayer(playerName);
            }
            QString postion;
            if (player && arg.size() >= 6 && JsonUtils::isString(arg[5]))
                postion = arg[5].toString();
            Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(skillName, category, type, player, postion));
            break;
        }
        case S_GAME_EVENT_JUDGE_RESULT: {
            int cardId = arg[1].toInt();
            bool takeEffect = arg[2].toBool();
            m_tablePile->showJudgeResult(cardId, takeEffect);
            break;
        }
        case S_GAME_EVENT_DETACH_SKILL: {
            QString player_name = arg[1].toString();
            QString skill_name = arg[2].toString();
            bool head = arg[3].toBool();

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            player->detachSkill(skill_name, head);
            if (player == Self) detachSkill(skill_name, head);

            // stop huashen animation
            PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(Player::PlaceHand, player);
            if (!player->hasSkill("huashen")) {
                container->stopHuaShen();
                player->tag.remove("Huashens");
                player->changePile("huashencard", false, QList<int>());
            }
            container->updateAvatarTooltip();
            break;
        }
        case S_GAME_EVENT_ACQUIRE_SKILL: {
            QString player_name = arg[1].toString();
            QString skill_name = arg[2].toString();
            bool head_skill = arg[3].toBool();

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            player->acquireSkill(skill_name, head_skill);
            acquireSkill(player, skill_name, head_skill);

            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->updateAvatarTooltip();
            break;
        }
        case S_GAME_EVENT_ADD_SKILL: {
            QString player_name = arg[1].toString();
            QString skill_name = arg[2].toString();
            bool head_skill = arg[3].toBool();

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            player->addSkill(skill_name, head_skill);

            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->updateAvatarTooltip();
            break;
        }
        case S_GAME_EVENT_LOSE_SKILL: {
            QString player_name = arg[1].toString();
            QString skill_name = arg[2].toString();
            bool head = arg[3].toBool();

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            player->loseSkill(skill_name, head);

            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->updateAvatarTooltip();
            break;
        }
        case S_GAME_EVENT_UPDATE_SKILL: {
            foreach (Photo *photo, photos)
                photo->updateAvatarTooltip();
            dashboard->updateAvatarTooltip();
            updateSkillButtons();
            break;
        }
        case S_GAME_EVENT_UPDATE_PRESHOW: {
            //Q_ASSERT(arg[1].isObject());
            bool auto_preshow_available = Self->hasFlag("AutoPreshowAvailable");
            JsonObject preshow_map = arg[1].value<JsonObject>();
            QList<QString> skill_names = preshow_map.keys();
            foreach (const QString &skill, skill_names) {
                bool showed = preshow_map[skill].toBool();

                if (Config.EnableAutoPreshow && auto_preshow_available) {
                    const Skill *s = Sanguosha->getSkill(skill);
                    if (s != NULL && s->canPreshow()) {
                        ClientInstance->preshow(skill, true, true);
                        ClientInstance->preshow(skill, true, false);
                    }
                } else {
                    Self->setSkillPreshowed(skill, showed, true);
                    Self->setSkillPreshowed(skill, showed, false);
                    if (!showed) {
                        foreach (QSanSkillButton *btn, m_skillButtons) {
                            if (btn->getSkill()->objectName() == skill) {
                                btn->QGraphicsObject::setEnabled(true);
                                btn->setState(QSanButton::S_STATE_CANPRESHOW);
                                break;
                            }
                        }
                    }

                    if (Self->inHeadSkills(skill))
                        dashboard->updateLeftHiddenMark();
                    else
                        dashboard->updateRightHiddenMark();
                }
            }
            break;
        }
        case S_GAME_EVENT_CHANGE_GENDER: {
            QString player_name = arg[1].toString();
            General::Gender gender = (General::Gender)arg[2].toInt();

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            player->setGender(gender);

            break;
        }
        case S_GAME_EVENT_CHANGE_HERO: {
            QString playerName = arg[1].toString();
            QString newHeroName = arg[2].toString();
            bool isSecondaryHero = arg[3].toBool();
            bool sendLog = arg[4].toBool();
            ClientPlayer *player = ClientInstance->getPlayer(playerName);
            PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(Player::PlaceHand, player);
            container->refresh();
            if (newHeroName == "anjiang" && player == Self) break;
            const General* oldHero = isSecondaryHero ? player->getGeneral2() : player->getGeneral();
            if (Sanguosha->getGeneral(newHeroName)) {
                if (isSecondaryHero) {
                    player->setGeneral2Name(newHeroName);
                } else {
                    player->setGeneralName(newHeroName);
                }

                if (sendLog) {
                    QString type = "#Transfigure";
                    QString arg2 = QString();
                    if (player->getGeneral2() && !isSecondaryHero) {
                        type = "#TransfigureDual";
                        arg2 = "GeneralA";
                    } else if (isSecondaryHero) {
                        type = "#TransfigureDual";
                        arg2 = "GeneralB";
                    }
                    log_box->appendLog(type, player->objectName(), QStringList(), QString(), newHeroName, arg2);
                }
            }
            const General* newHero = Sanguosha->getGeneral(newHeroName);

            if (oldHero) {
                foreach (const Skill *skill, oldHero->getVisibleSkills(true, !isSecondaryHero))
                    detachSkill(skill->objectName(), !isSecondaryHero);
                if (oldHero->hasSkill("huashen")) {
                    container->stopHuaShen();
                    player->tag.remove("Huashens");
                    player->changePile("huashencard", false, QList<int>());
                }
            }
            if (player != Self) break;
            if (newHero && !player->isDuanchang(!isSecondaryHero)) {
                foreach (const Skill *skill, newHero->getVisibleSkills(true, !isSecondaryHero))
                    attachSkill(skill->objectName(), !isSecondaryHero);
            }
            break;
        }
        case S_GAME_EVENT_PLAYER_REFORM: {
            ClientPlayer *player = ClientInstance->getPlayer(arg[1].toString());
            PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
            if (container != NULL)
                container->updateReformState();
            break;
        }
        case S_GAME_EVENT_SKILL_INVOKED: {
            QString player_name = arg[1].toString();
            QString skill_name = arg[2].toString();
            const Skill *skill = Sanguosha->getSkill(skill_name);
            if (skill && (skill->isAttachedLordSkill() || skill->inherits("SPConvertSkill"))) return;

            ClientPlayer *player = ClientInstance->getPlayer(player_name);
            if (!player || !player->hasSkill(skill_name)) return;
            if (player != Self) {
                PlayerCardContainer *container = qobject_cast<PlayerCardContainer *>(_getGenericCardContainer(Player::PlaceHand, player));
                if (container != NULL) {
                    Photo *photo = qobject_cast<Photo *>(container);
                    if (photo) photo->showSkillName(skill_name);
                }
            }
            QString skillEmotion = QString("skill/%1").arg(skill_name);
            if (QFile::exists(skillEmotion))
                setEmotion(player_name, skillEmotion);
            break;
        }
        case S_GAME_EVENT_PAUSE: {
            bool paused = arg[1].toBool();
            if (pausing_item->isVisible() != paused) {
                if (paused) {
                    QBrush pausing_brush(QColor(qrand() % 256, qrand() % 256, qrand() % 256));
                    pausing_item->setBrush(pausing_brush);
                    bringToFront(pausing_item);
                    bringToFront(pausing_text);
                }
                pausing_item->setVisible(paused);
                pausing_text->setVisible(paused);
            }
            break;
        }
        case S_GAME_EVENT_REVEAL_PINDIAN: {
            QString who = arg[1].toString();
            m_pindianBox->doPindianAnimation(who);
        }
        default:
            break;
    }
}

QGraphicsItem *RoomScene::createDashboardButtons()
{
    QGraphicsItem *widget = new QGraphicsPixmapItem(G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_DASHBOARD_BUTTON_SET_BG)
        .scaled(G_DASHBOARD_LAYOUT.m_buttonSetSize));

#ifdef Q_OS_ANDROID
    ok_button = new QSanButton("platter", "confirm_android", widget);
    int width = main_window->width() - G_DASHBOARD_LAYOUT.m_buttonSetSize.width() - G_DASHBOARD_LAYOUT.m_leftWidth - G_DASHBOARD_LAYOUT.m_rightWidth - 20;
    ok_button->setRect(QRect(-width / 2 - 160, -100, 150, 80));

    cancel_button = new QSanButton("platter", "cancel_android", widget);
    cancel_button->setRect(QRect(-width / 2 + 10, -100, 150, 80));

    discard_button = new QSanButton("platter", "discard_android", widget);
    discard_button->setScale(_m_roomLayout->scale);
#else
    ok_button = new QSanButton("platter", "confirm", widget);
    ok_button->setRect(G_DASHBOARD_LAYOUT.m_confirmButtonArea);
    cancel_button = new QSanButton("platter", "cancel", widget);
    cancel_button->setRect(G_DASHBOARD_LAYOUT.m_cancelButtonArea);
    discard_button = new QSanButton("platter", "discard", widget);
    discard_button->setRect(G_DASHBOARD_LAYOUT.m_discardButtonArea);
#endif
    connect(ok_button, &QSanButton::clicked, this, &RoomScene::doOkButton);
    connect(cancel_button, &QSanButton::clicked, this, &RoomScene::doCancelButton);
    connect(discard_button, &QSanButton::clicked, this, &RoomScene::doDiscardButton);

    // set them all disabled
    ok_button->setEnabled(false);
    cancel_button->setEnabled(false);
    discard_button->setEnabled(false);
    return widget;
}

QRectF ReplayerControlBar::boundingRect() const
{
    return QRectF(0, 0, S_BUTTON_WIDTH * 4 + S_BUTTON_GAP * 3, S_BUTTON_HEIGHT);
}

void ReplayerControlBar::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

ReplayerControlBar::ReplayerControlBar(Dashboard *dashboard)
{
    QSanButton *play, *uniform, *slow_down, *speed_up;

    uniform = new QSanButton("replay", "uniform", this);
    slow_down = new QSanButton("replay", "slow-down", this);
    play = new QSanButton("replay", "pause", this);
    speed_up = new QSanButton("replay", "speed-up", this);
    play->setStyle(QSanButton::S_STYLE_TOGGLE);
    uniform->setStyle(QSanButton::S_STYLE_TOGGLE);

    int step = S_BUTTON_GAP + S_BUTTON_WIDTH;
    uniform->setPos(0, 0);
    slow_down->setPos(step, 0);
    play->setPos(step * 2, 0);
    speed_up->setPos(step * 3, 0);

    time_label = new QLabel;
    time_label->setAttribute(Qt::WA_NoSystemBackground);
    time_label->setText("-----------------------------------------------------");
    QPalette palette;
    palette.setColor(QPalette::WindowText, Config.TextEditColor);
    time_label->setPalette(palette);

    QGraphicsProxyWidget *widget = new QGraphicsProxyWidget(this);
    widget->setWidget(time_label);
    widget->setPos(step * 4, 0);

    Replayer *replayer = ClientInstance->getReplayer();
    connect(play, &QSanButton::clicked, replayer, &Replayer::toggle);
    connect(uniform, &QSanButton::clicked, replayer, &Replayer::uniform);
    connect(slow_down, &QSanButton::clicked, replayer, &Replayer::slowDown);
    connect(speed_up, &QSanButton::clicked, replayer, &Replayer::speedUp);
    connect(replayer, &Replayer::elasped, this, &ReplayerControlBar::setTime);
    connect(replayer, &Replayer::speed_changed, this, &ReplayerControlBar::setSpeed);

    speed = replayer->getSpeed();
    setParentItem(dashboard);
    setPos(S_BUTTON_GAP, -S_BUTTON_GAP - S_BUTTON_HEIGHT);

    duration_str = FormatTime(replayer->getDuration());
}

QString ReplayerControlBar::FormatTime(int secs)
{
    int minutes = secs / 60;
    int remainder = secs % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(remainder, 2, 10, QChar('0'));
}

void ReplayerControlBar::setSpeed(qreal speed)
{
    this->speed = speed;
}

void ReplayerControlBar::setTime(int secs)
{
    time_label->setText(QString("<b>x%1 </b> [%2/%3]").arg(speed).arg(FormatTime(secs)).arg(duration_str));
}

void RoomScene::createReplayControlBar()
{
    m_replayControl = new ReplayerControlBar(dashboard);
}

void RoomScene::_getSceneSizes(QSize &minSize, QSize &maxSize)
{
    if (photos.size() >= 8) {
        minSize = _m_roomLayout->m_minimumSceneSize10Player;
        maxSize = _m_roomLayout->m_maximumSceneSize10Player;
    } else {
        minSize = _m_roomLayout->m_minimumSceneSize;
        maxSize = _m_roomLayout->m_maximumSceneSize;
    }
}

void RoomScene::onSceneRectChanged(const QRectF &rect)
{
    // switch between default & compact skin depending on scene size
    QSanSkinFactory &factory = QSanSkinFactory::getInstance();
    QString skinName = factory.getCurrentSkinName();

    QSize minSize, maxSize;
    _getSceneSizes(minSize, maxSize);
    if (skinName == factory.S_DEFAULT_SKIN_NAME) {
        if (rect.width() < minSize.width() || rect.height() < minSize.height()) {
            QThread *thread = QCoreApplication::instance()->thread();
            thread->blockSignals(true);
            factory.switchSkin(factory.S_COMPACT_SKIN_NAME);
            thread->blockSignals(false);
            foreach(Photo *photo, photos)
                photo->repaintAll();
            dashboard->repaintAll();
        }
    } else if (skinName == factory.S_COMPACT_SKIN_NAME) {
        if (rect.width() > maxSize.width() && rect.height() > maxSize.height()) {
            QThread *thread = QCoreApplication::instance()->thread();
            thread->blockSignals(true);
            factory.switchSkin(factory.S_DEFAULT_SKIN_NAME);
            thread->blockSignals(false);
            foreach(Photo *photo, photos)
                photo->repaintAll();
            dashboard->repaintAll();
        }
    }

    // update the sizes since we have reloaded the skin.
    _getSceneSizes(minSize, maxSize);

    QRectF newRect(rect);
    if (rect.height() < minSize.height() || rect.width() < minSize.width()) {
        double sy = minSize.height() / rect.height();
        double sx = minSize.width() / rect.width();
        double scale = qMax(sx, sy);
        newRect.setHeight(scale * rect.height());
        newRect.setWidth(scale * rect.width());
        disconnect(this, &RoomScene::sceneRectChanged, this, &RoomScene::onSceneRectChanged);
        setSceneRect(newRect);
        connect(this, &RoomScene::sceneRectChanged, this, &RoomScene::onSceneRectChanged);
    }

    int padding = _m_roomLayout->m_scenePadding;
    newRect.moveLeft(newRect.x() + padding);
    newRect.moveTop(newRect.y() + padding);
    newRect.setWidth(newRect.width() - padding * 2);
    newRect.setHeight(newRect.height() - padding * 2);

    // set dashboard
    dashboard->setX(newRect.x());
    dashboard->setWidth(newRect.width());
    dashboard->setY(newRect.height() - dashboard->boundingRect().height());
    dashboard->adjustCards(false);

    // set infoplane
    _m_infoPlane.setWidth(newRect.width() * _m_roomLayout->m_infoPlaneWidthPercentage);
    _m_infoPlane.moveRight(newRect.right());
    _m_infoPlane.setTop(newRect.top() + _m_roomLayout->m_roleBoxHeight);
#ifdef Q_OS_ANDROID
    _m_infoPlane.setBottom((dashboard->y() - _m_roomLayout->m_chatTextBoxHeight) * (1 - _m_roomLayout->m_logBoxHeightPercentage));
#else
    _m_infoPlane.setBottom(dashboard->y() - _m_roomLayout->m_chatTextBoxHeight);
#endif
    m_rolesBoxBackground = m_rolesBoxBackground.scaled(_m_infoPlane.width(), _m_roomLayout->m_roleBoxHeight);
    m_rolesBox->setPixmap(m_rolesBoxBackground);
    m_rolesBox->setPos(_m_infoPlane.left(), newRect.top());

#ifdef Q_OS_ANDROID
    log_box_widget->setPos(G_DASHBOARD_LAYOUT.m_leftWidth, main_window->height() - G_DASHBOARD_LAYOUT.m_normalHeight - 200);
    log_box->resize(main_window->width() - G_DASHBOARD_LAYOUT.m_leftWidth - _m_infoPlane.width(), main_window->height() / 5);
    chat_box_widget->setPos(_m_infoPlane.topLeft());
    chatBox->resize(_m_infoPlane.width(), _m_infoPlane.bottom() - chat_box_widget->y());
    chat_edit_widget->setPos(_m_infoPlane.left(), _m_infoPlane.bottom());
    chatEdit->resize(_m_infoPlane.width() - chat_widget->boundingRect().width(), _m_roomLayout->m_chatTextBoxHeight);
    chat_widget->setPos(_m_infoPlane.right() - chat_widget->boundingRect().width(),
        chat_edit_widget->y() + (_m_roomLayout->m_chatTextBoxHeight - chat_widget->boundingRect().height()) / 2);
#else
    log_box_widget->setPos(_m_infoPlane.topLeft());
    log_box->resize(_m_infoPlane.width(), _m_infoPlane.height() * _m_roomLayout->m_logBoxHeightPercentage);
    chat_box_widget->setPos(_m_infoPlane.left(), _m_infoPlane.bottom() - _m_infoPlane.height() * _m_roomLayout->m_chatBoxHeightPercentage);
    chatBox->resize(_m_infoPlane.width(), _m_infoPlane.bottom() - chat_box_widget->y());
    chat_edit_widget->setPos(_m_infoPlane.left(), _m_infoPlane.bottom());
    chatEdit->resize(_m_infoPlane.width() - chat_widget->boundingRect().width(), _m_roomLayout->m_chatTextBoxHeight);
    chat_widget->setPos(_m_infoPlane.right() - chat_widget->boundingRect().width(),
        chat_edit_widget->y() + (_m_roomLayout->m_chatTextBoxHeight - chat_widget->boundingRect().height()) / 2);
#endif

    m_tablew = newRect.width();// - infoPlane.width();
    m_tableh = newRect.height();// - dashboard->boundingRect().height();
    m_tableh -= _m_roomLayout->m_photoDashboardPadding;
    updateTable();
    updateRolesBox();
#ifdef Q_OS_ANDROID
    setChatBoxVisible(true);
#else
    setChatBoxVisible(chat_box_widget->isVisible());
#endif
    QMapIterator<QString, BubbleChatBox *> iter(bubbleChatBoxes);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->setArea(getBubbleChatBoxShowArea(iter.key()));
    }
}

void RoomScene::_dispersePhotos(QList<Photo *> &photos, QRectF fillRegion,
    Qt::Orientation orientation, Qt::Alignment align)
{
    double photoWidth = _m_photoLayout->m_normalWidth;
    double photoHeight = _m_photoLayout->m_normalHeight;
    int numPhotos = photos.size();
    if (numPhotos == 0) return;
    Qt::Alignment hAlign = align & Qt::AlignHorizontal_Mask;
    Qt::Alignment vAlign = align & Qt::AlignVertical_Mask;

    double startX = 0, startY = 0, stepX, stepY;

    if (orientation == Qt::Horizontal) {
        double maxWidth = fillRegion.width();
        stepX = qMax(photoWidth + G_ROOM_LAYOUT.m_photoHDistance, maxWidth / numPhotos);
        stepY = 0;
    } else {
        stepX = 0;
        stepY = G_ROOM_LAYOUT.m_photoVDistance + photoHeight;
    }

    switch (vAlign) {
        case Qt::AlignTop: startY = fillRegion.top() + photoHeight / 2; break;
        case Qt::AlignBottom: startY = fillRegion.bottom() - photoHeight / 2 - stepY * (numPhotos - 1); break;
        case Qt::AlignVCenter: startY = fillRegion.center().y() - stepY * (numPhotos - 1) / 2.0; break;
        default: Q_ASSERT(false);
    }
    switch (hAlign) {
        case Qt::AlignLeft: startX = fillRegion.left() + photoWidth / 2; break;
        case Qt::AlignRight: startX = fillRegion.right() - photoWidth / 2 - stepX * (numPhotos - 1); break;
        case Qt::AlignHCenter: startX = fillRegion.center().x() - stepX * (numPhotos - 1) / 2.0; break;
        default: Q_ASSERT(false);
    }

    for (int i = 0; i < numPhotos; i++) {
        Photo *photo = photos[i];
        QPointF newPos = QPointF(startX + stepX * i, startY + stepY * i);
        photo->setPos(newPos);
    }
}

void RoomScene::updateTable()
{
    int pad = _m_roomLayout->m_scenePadding + _m_roomLayout->m_photoRoomPadding;
    int tablew = chatBox->x() - pad * 2;
    int tableh = sceneRect().height() - pad * 2 - dashboard->boundingRect().height();
    int photow = _m_photoLayout->m_normalWidth;
    int photoh = _m_photoLayout->m_normalHeight;

    // Layout:
    //    col1           col2
    // _______________________
    // |_2_|______1_______|_0_| row1
    // |   |              |   |
    // | 4 |    table     | 3 |
    // |___|______________|___|
    // |      dashboard       |
    // ------------------------
    // region 5 = 0 + 3, region 6 = 2 + 4, region 7 = 0 + 1 + 2

    static int regularSeatIndex[][9] = {
        {1},
        {5, 6},
        {5, 1, 6},
        {3, 1, 1, 4},
        {3, 1, 1, 1, 4},
        {5, 5, 1, 1, 6, 6},
        {5, 5, 1, 1, 1, 6, 6},
        {3, 3, 7, 7, 7, 7, 4, 4},
        {3, 3, 7, 7, 7, 7, 7, 4, 4}
    };

    double hGap = _m_roomLayout->m_photoHDistance;
    double vGap = _m_roomLayout->m_photoVDistance;
    double col1 = photow + hGap;
    double col2 = tablew - col1;
    double row1 = photoh + vGap;
    double row2 = tableh;

    const int C_NUM_REGIONS = 8;
    QRectF seatRegions[] = {
        QRectF(col2, pad, col1, row1),
        QRectF(col1, pad, col2 - col1, row1),
        QRectF(pad, pad, col1, row1),
        QRectF(col2, row1, col1, row2 - row1),
        QRectF(pad, row1, col1, row2 - row1),
        QRectF(col2, pad, col1, row2),
        QRectF(pad, pad, col1, row2),
        QRectF(pad, pad, col1 + col2, row1)
    };

    static Qt::Alignment aligns[] = {
        Qt::AlignRight | Qt::AlignTop,
        Qt::AlignHCenter | Qt::AlignTop,
        Qt::AlignLeft | Qt::AlignTop,
        Qt::AlignRight | Qt::AlignVCenter,
        Qt::AlignLeft | Qt::AlignVCenter,
        Qt::AlignRight | Qt::AlignVCenter,
        Qt::AlignLeft | Qt::AlignVCenter,
        Qt::AlignHCenter | Qt::AlignTop,
    };

    static Qt::Alignment kofAligns[] = {
        Qt::AlignRight | Qt::AlignTop,
        Qt::AlignHCenter | Qt::AlignTop,
        Qt::AlignLeft | Qt::AlignTop,
        Qt::AlignRight | Qt::AlignBottom,
        Qt::AlignLeft | Qt::AlignBottom,
        Qt::AlignRight | Qt::AlignBottom,
        Qt::AlignLeft | Qt::AlignBottom,
        Qt::AlignHCenter | Qt::AlignTop,
    };

    Qt::Orientation orients[] = {
        Qt::Horizontal,
        Qt::Horizontal,
        Qt::Horizontal,
        Qt::Vertical,
        Qt::Vertical,
        Qt::Vertical,
        Qt::Vertical,
        Qt::Horizontal
    };

    QRectF tableRect(col1, row1, col2 - col1, row2 - row1);

    QRect tableBottomBar(0, 0, chatBox->x() - col1, G_DASHBOARD_LAYOUT.m_floatingAreaHeight);
    tableBottomBar.moveBottomLeft(QPoint((int)tableRect.left(), 0));
    dashboard->setFloatingArea(tableBottomBar);

    m_tableCenterPos = tableRect.center();
#ifdef Q_OS_ANDROID
    if (control_panel)
        control_panel->setPos(m_tableCenterPos.x() - add_robot->boundingRect().width() / 2, m_tableCenterPos.y() / 4 * 3);
    return_to_start_scene->setPos(m_tableCenterPos.x() - return_to_start_scene->boundingRect().width() / 2,
                                  m_tableCenterPos.y() + return_to_start_scene->boundingRect().height() + 10);
#else
    if (control_panel)
        control_panel->setPos(m_tableCenterPos);
    return_to_start_scene->setPos(m_tableCenterPos + QPointF(0, return_to_start_scene->boundingRect().height() + 10));
#endif
    m_tablePile->setPos(m_tableCenterPos);
    m_tablePile->setSize(qMax((int)tableRect.width() - _m_roomLayout->m_discardPilePadding * 2,
        _m_roomLayout->m_discardPileMinWidth), _m_commonLayout->m_cardNormalHeight);
    m_tablePile->adjustCards();
    m_cardContainer->setPos(m_tableCenterPos - QPointF(m_cardContainer->boundingRect().width() / 2, m_cardContainer->boundingRect().height() / 2));
    pileContainer->setPos(m_tableCenterPos - QPointF(pileContainer->boundingRect().width() / 2, pileContainer->boundingRect().height() / 2));
    m_guanxingBox->setPos(m_tableCenterPos - QPointF(m_guanxingBox->boundingRect().width() / 2, m_guanxingBox->boundingRect().height() / 2));
    m_cardchooseBox->setPos(m_tableCenterPos - QPointF(m_cardchooseBox->boundingRect().width() / 2, m_cardchooseBox->boundingRect().height() / 2));
    m_pindianBox->setPos(m_tableCenterPos - QPointF(m_pindianBox->boundingRect().width() / 2, m_pindianBox->boundingRect().height() / 2));
    m_chooseGeneralBox->setPos(m_tableCenterPos - QPointF(m_chooseGeneralBox->boundingRect().width() / 2, m_chooseGeneralBox->boundingRect().height() / 2));
    m_chooseOptionsBox->setPos(m_tableCenterPos - QPointF(m_chooseOptionsBox->boundingRect().width() / 2, m_chooseOptionsBox->boundingRect().height() / 2));
    m_chooseTriggerOrderBox->setPos(m_tableCenterPos - QPointF(m_chooseTriggerOrderBox->boundingRect().width() / 2, m_chooseTriggerOrderBox->boundingRect().height() / 2));
    m_playerCardBox->setPos(m_tableCenterPos - QPointF(m_chooseTriggerOrderBox->boundingRect().width() / 2, m_chooseTriggerOrderBox->boundingRect().height() / 2));
    m_chooseSuitBox->setPos(m_tableCenterPos - QPointF(m_chooseSuitBox->boundingRect().width() / 2, m_chooseSuitBox->boundingRect().height() / 2));
    prompt_box->setPos(m_tableCenterPos);
    pausing_text->setPos(m_tableCenterPos - pausing_text->boundingRect().center());
    pausing_item->setRect(sceneRect());
    pausing_item->setPos(0, 0);

    int *seatToRegion = regularSeatIndex[photos.length() - 1];
    bool pkMode = false;
    QList<Photo *> photosInRegion[C_NUM_REGIONS];
    int n = photos.length();
    for (int i = 0; i < n; i++) {
        int regionIndex = seatToRegion[i];
        Q_ASSERT(regionIndex < 8);
        if (regionIndex == 4 || regionIndex == 6)
            photosInRegion[regionIndex].append(photos[i]);
        else
            photosInRegion[regionIndex].prepend(photos[i]);
    }
    for (int i = 0; i < C_NUM_REGIONS; i++) {
        if (photosInRegion[i].isEmpty()) continue;
        Qt::Alignment align;
        if (pkMode) align = kofAligns[i];
        else align = aligns[i];
        Qt::Orientation orient = orients[i];

        int hDist = G_ROOM_LAYOUT.m_photoHDistance;
        QRect floatingArea(0, 0, hDist, G_PHOTO_LAYOUT.m_normalHeight);
        // if the photo is on the right edge of table
        if (i == 0 || i == 3 || i == 5 || i == 8)
            floatingArea.moveRight(0);
        else
            floatingArea.moveLeft(G_PHOTO_LAYOUT.m_normalWidth);

        foreach (Photo *photo, photosInRegion[i])
            photo->setFloatingArea(floatingArea);
        _dispersePhotos(photosInRegion[i], seatRegions[i], orient, align);
    }
}

void RoomScene::addPlayer(ClientPlayer *player)
{
    for (int i = 0; i < photos.length(); i++) {
        Photo *photo = photos[i];
        if (photo->getPlayer() == NULL) {
            photo->setPlayer(player);
            name2photo[player->objectName()] = photo;

            if (!Self->hasFlag("marshalling"))
                Sanguosha->playSystemAudioEffect("add-player");

            return;
        }
    }
}

void RoomScene::removePlayer(const QString &player_name)
{
    Photo *photo = name2photo[player_name];
    if (photo) {
        photo->setPlayer(NULL);
        name2photo.remove(player_name);
        Sanguosha->playSystemAudioEffect("remove-player");
    }
}

void RoomScene::arrangeSeats(const QList<const ClientPlayer *> &seats)
{
    // rearrange the photos
    Q_ASSERT(seats.length() == photos.length());

    for (int i = 0; i < seats.length(); i++) {
        const Player *player = seats.at(i);
        for (int j = i; j < photos.length(); j++) {
            if (photos.at(j)->getPlayer() == player) {
                photos.swap(i, j);
                break;
            }
        }
    }
    game_started = true;
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    updateTable();

    group->start(QAbstractAnimation::DeleteWhenStopped);

    // set item to player mapping
    if (item2player.isEmpty()) {
        item2player.insert(dashboard, Self);
        connect(dashboard, &Dashboard::global_selected_changed, this, &RoomScene::updateGlobalCardBox);
        connect(dashboard, &Dashboard::selected_changed, this, &RoomScene::updateSelectedTargets);

        // create global card boxes after set item to player mapping
        card_boxes[Self] = new PlayerCardBox;
        addItem(card_boxes[Self]);
        card_boxes[Self]->setZValue(-0.5);
        connect(card_boxes[Self], &PlayerCardBox::global_choose, this, &RoomScene::updateGlobalCardBox);

        foreach (Photo *photo, photos) {
            item2player.insert(photo, photo->getPlayer());
            connect(photo, &Photo::global_selected_changed, this, &RoomScene::updateGlobalCardBox);
            connect(photo, &Photo::selected_changed, this, &RoomScene::updateSelectedTargets);
            connect(photo, &Photo::enable_changed, this, &RoomScene::onEnabledChange);

            // create global card boxes after set item to player mapping
            card_boxes[photo->getPlayer()] = new PlayerCardBox;
            addItem(card_boxes[photo->getPlayer()]);
            card_boxes[photo->getPlayer()]->setZValue(-0.5);
            connect(card_boxes[photo->getPlayer()], &PlayerCardBox::global_choose, this, &RoomScene::updateGlobalCardBox);
        }
    }
#ifndef Q_OS_ANDROID
    bool all_robot = true;
    foreach (const ClientPlayer *p, ClientInstance->getPlayers()) {
        if (p != Self && p->getState() != "robot") {
            all_robot = false;
            break;
        }
    }
    if (all_robot)
        setChatBoxVisible(false);
#endif
    //update the positions of bubbles after setting seats
    QList<QString> names = name2photo.keys();
    foreach (const QString &who, names) {
        if (bubbleChatBoxes.contains(who)) {
            bubbleChatBoxes[who]->setArea(getBubbleChatBoxShowArea(who));
        }
    }
}

void RoomScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);

    m_mousePressed = true;
    bool changed = false;
    QPoint point(event->pos().x(), event->pos().y());
    foreach (Photo *photo, photos) {
        RoleComboBox *box = photo->getRoleComboBox();
        if (!box->boundingRect().contains(point) && box->isExpanding())
            changed = true;
    }
    RoleComboBox *box = dashboard->getRoleComboBox();
    if (!box->boundingRect().contains(point) && box->isExpanding())
        changed = true;
    if (changed)
        emit cancel_role_box_expanding();
}

void RoomScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    m_mousePressed = false;

    if (m_isInDragAndUseMode) {
        if ((ok_button->isEnabled() || dashboard->currentSkill())
            && (!dashboard->isUnderMouse() || dashboard->isAvatarUnderMouse())) {
            if (ok_button->isEnabled())
                ok_button->click();
            else
                dashboard->adjustCards(true);
        } else {
            enableTargets(NULL);
            dashboard->unselectAll();
        }
        m_isInDragAndUseMode = false;
    }
}

void RoomScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    if (!Config.EnableSuperDrag || !m_mousePressed)
        return;
    QGraphicsObject *obj = static_cast<QGraphicsObject *>(focusItem());
    CardItem *card_item = qobject_cast<CardItem *>(obj);
    if (!card_item || !card_item->isUnderMouse() || !dashboard->hasHandCard(card_item))
        return;
    static bool wasOutsideDashboard = false;
    const bool isOutsideDashboard = !dashboard->isUnderMouse()
        || dashboard->isAvatarUnderMouse();
    if (isOutsideDashboard != wasOutsideDashboard) {
        wasOutsideDashboard = isOutsideDashboard;
        if (wasOutsideDashboard && !m_isInDragAndUseMode) {
            if (!m_superDragStarted && !dashboard->getPendings().isEmpty())
                dashboard->clearPendings();

            dashboard->selectCard(card_item, true);
            if (dashboard->currentSkill() && !dashboard->getPendings().contains(card_item)) {
                dashboard->addPending(card_item);
                dashboard->updatePending();
            }
            m_isInDragAndUseMode = true;
            m_superDragStarted = true;
            if (!dashboard->currentSkill()
                && (ClientInstance->getStatus() == Client::Playing
                || ClientInstance->getStatus() == Client::RespondingUse)) {
                enableTargets(card_item->getCard());
            }
        }
    }

    PlayerCardContainer *victim = NULL;

    foreach (Photo *photo, photos) {
        if (photo->isUnderMouse())
            victim = photo;
    }

    if (dashboard->isAvatarUnderMouse())
        victim = dashboard;

    if (victim != NULL && victim->canBeSelected()) {
        victim->setSelected(true);
        updateSelectedTargets();
    }
}

void RoomScene::enableTargets(const Card *card)
{
    bool enabled = true;
    if (card != NULL) {
        Client::Status status = ClientInstance->getStatus();
        if (status == Client::Playing && !card->isAvailable(Self))
            enabled = false;
        if (status == Client::Responding || status == Client::RespondingUse) {
            Card::HandlingMethod method = card->getHandlingMethod();
            if (status == Client::Responding && method == Card::MethodUse)
                method = Card::MethodResponse;
            if (Self->isCardLimited(card, method))
                enabled = false;
        }
        if (status == Client::RespondingForDiscard && Self->isCardLimited(card, Card::MethodDiscard))
            enabled = false;
    }
    if (!enabled) {
        ok_button->setEnabled(false);
        return;
    }

    selected_targets.clear();

    // unset avatar and all photo
    foreach (PlayerCardContainer *item, item2player.keys())
        item->setSelected(false);

    if (card == NULL) {
        foreach (PlayerCardContainer *item, item2player.keys()) {
            QGraphicsItem *animationTarget = item->getMouseClickReceiver();
            animations->effectOut(animationTarget);
            QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
            if (animationTarget2)
                animations->effectOut(animationTarget2);

            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setEnabled(true);
        }

        ok_button->setEnabled(false);
        return;
    }

    Client::Status status = ClientInstance->getStatus();
    if (card->targetFixed()
        || ((status & Client::ClientStatusBasicMask) == Client::Responding
        && (status == Client::Responding || (card->getTypeId() != Card::TypeSkill && status != Client::RespondingUse)))
        || ClientInstance->getStatus() == Client::AskForShowOrPindian) {
        foreach (PlayerCardContainer *item, item2player.keys()) {
            QGraphicsItem *animationTarget = item->getMouseClickReceiver();
            animations->effectOut(animationTarget);
            QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
            if (animationTarget2)
                animations->effectOut(animationTarget2);

            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        }

        ok_button->setEnabled(true);
        return;
    }

    updateTargetsEnablity(card);

    if (selected_targets.isEmpty()) {
        if (card->isKindOf("Slash") && Self->hasFlag("slashTargetFixToOne")) {
            unselectAllTargets();
            foreach (Photo *photo, photos) {
                if (photo->flags() & QGraphicsItem::ItemIsSelectable) {
                    if (!photo->isSelected()) {
                        photo->setSelected(true);
                        break;
                    }
                }
            }
        } else if (Config.EnableAutoTarget) {
            if (!card->targetsFeasible(selected_targets, Self)) {
                unselectAllTargets();
                int count = 0;
                foreach(Photo *photo, photos)
                    if (photo->flags() & QGraphicsItem::ItemIsSelectable)
                        count++;
                if (dashboard->flags() & QGraphicsItem::ItemIsSelectable)
                    count++;
                if (count == 1)
                    selectNextTarget(false);
            }
        }
    }

    ok_button->setEnabled(card->targetsFeasible(selected_targets, Self));
}

void RoomScene::updateTargetsEnablity(const Card *card)
{
    QMapIterator<PlayerCardContainer *, const ClientPlayer *> itor(item2player);
    while (itor.hasNext()) {
        itor.next();

        PlayerCardContainer *item = itor.key();
        const ClientPlayer *player = itor.value();
        int maxVotes = 0;

        if (card) {
            card->targetFilter(selected_targets, player, Self, maxVotes);
            item->setMaxVotes(maxVotes);
        }

        if (item->isSelected()) continue;

        bool enabled = (card == NULL) || (!Sanguosha->isProhibited(Self, player, card, selected_targets) && maxVotes > 0);

        QGraphicsItem *animationTarget = item->getMouseClickReceiver();
        QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
        if (enabled) {
            animations->effectOut(animationTarget);
            if (animationTarget2)
                animations->effectOut(animationTarget2);
        } else if (!animationTarget->graphicsEffect()
            || !animationTarget->graphicsEffect()->inherits("SentbackEffect")) {
            animations->sendBack(animationTarget);
            if (animationTarget2)
                animations->sendBack(animationTarget2);
        }
        if (card)
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
    }
}

void RoomScene::updateSelectedTargets()
{
    PlayerCardContainer *item = qobject_cast<PlayerCardContainer *>(sender());
    if (item == NULL) return;

    const Card *card = dashboard->getSelected();
    if (card) {
        const ClientPlayer *player = item2player.value(item, NULL);
        if (item->isSelected()) {
            selected_targets.append(player);
        } else {
            selected_targets.removeAll(player);
            foreach (const Player *cp, selected_targets) {
                QList<const Player *> tempPlayers = QList<const Player *>(selected_targets);
                tempPlayers.removeAll(cp);
                if (!card->targetFilter(tempPlayers, cp, Self) || Sanguosha->isProhibited(Self, cp, card, selected_targets)) {
                    selected_targets.clear();
                    unselectAllTargets();
                    return;
                }
            }
        }
        ok_button->setEnabled(card->targetsFeasible(selected_targets, Self));
    } else {
        selected_targets.clear();
    }

    updateTargetsEnablity(card);
}

void RoomScene::keyReleaseEvent(QKeyEvent *event)
{
    if (!Config.EnableHotKey) return;
    if (chatEdit->hasFocus()) return;

    bool control_is_down = event->modifiers() & Qt::ControlModifier;
    bool alt_is_down = event->modifiers() & Qt::AltModifier;

    switch (event->key()) {
        case Qt::Key_F1: break;
        case Qt::Key_F2: chooseSkillButton(); break;
        case Qt::Key_F3: dashboard->beginSorting(); break;
        case Qt::Key_F4: dashboard->reverseSelection(); break;
        case Qt::Key_F5: {
            onSceneRectChanged(sceneRect());
            break;
        }
        case Qt::Key_F6: {
            if (!Self || !Self->isOwner() || ClientInstance->getPlayers().length() < Sanguosha->getPlayerCount(ServerInfo.GameMode)) break;
            foreach (const ClientPlayer *p, ClientInstance->getPlayers()) {
                if (p != Self && p->isAlive() && p->getState() != "robot")
                    break;
            }
            bool paused = pausing_text->isVisible();
            ClientInstance->notifyServer(S_COMMAND_PAUSE, !paused);
            break;
        }
        case Qt::Key_F7: {
            if (control_is_down) {
                if (add_robot && add_robot->isVisible())
                    ClientInstance->addRobot();
            } else if (fill_robots && fill_robots->isVisible()) {
                ClientInstance->fillRobots();
            }
            break;
        }
        case Qt::Key_F8: {
            setChatBoxVisible(!chat_box_widget->isVisible());
            break;
        }
        case Qt::Key_S: dashboard->selectCard("slash");  break;
        case Qt::Key_J: dashboard->selectCard("jink"); break;
        case Qt::Key_P: dashboard->selectCard("peach"); break;
        case Qt::Key_O: dashboard->selectCard("analeptic"); break;

        case Qt::Key_E: dashboard->selectCard("equip"); break;
        case Qt::Key_W: dashboard->selectCard("weapon"); break;
        case Qt::Key_F: dashboard->selectCard("armor"); break;
        case Qt::Key_H: dashboard->selectCard("defensive_horse+offensive_horse"); break;

        case Qt::Key_T: dashboard->selectCard("trick"); break;
        case Qt::Key_A: dashboard->selectCard("aoe"); break;
        case Qt::Key_N: dashboard->selectCard("nullification"); break;
        case Qt::Key_Q: dashboard->selectCard("snatch"); break;
        case Qt::Key_C: dashboard->selectCard("dismantlement"); break;
        case Qt::Key_U: dashboard->selectCard("duel"); break;
        case Qt::Key_L: dashboard->selectCard("lightning"); break;
        case Qt::Key_I: dashboard->selectCard("indulgence"); break;
        case Qt::Key_B: dashboard->selectCard("supply_shortage"); break;

        case Qt::Key_Left: dashboard->selectCard(".", false, control_is_down); break;
        case Qt::Key_Right: dashboard->selectCard(".", true, control_is_down); break; // iterate all cards

        case Qt::Key_Return: {
            if (ok_button->isEnabled()) doOkButton();
            break;
        }
        case Qt::Key_Escape: {
            if (ClientInstance->getStatus() == Client::Playing) {
                dashboard->unselectAll();
                enableTargets(NULL);
            } else
                dashboard->unselectAll();
            break;
        }
        case Qt::Key_Space: {
            if (cancel_button->isEnabled())
                doCancelButton();
            else if (discard_button->isEnabled())
                doDiscardButton();
        }

        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4: {
            int position = event->key() - Qt::Key_0;
            if (position != 0 && alt_is_down) {
                dashboard->selectEquip(position);
                break;
            }
        }
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9: {
            int order = event->key() - Qt::Key_0;
            selectTarget(order, control_is_down);
            break;
        }

        case Qt::Key_D: {
            if (Self == NULL) return;
            if (Self->property("distance_shown").toBool()) {
                Self->setProperty("distance_shown", false);
                foreach (Photo *photo, photos) {
                    if (photo->getPlayer() && photo->getPlayer()->isAlive())
                        photo->hideDistance();
                }
            } else {
                Self->setProperty("distance_shown", true);
                foreach (Photo *photo, photos) {
                    if (photo->getPlayer() && photo->getPlayer()->isAlive())
                        photo->showDistance();
                }
            }
            break;
        }

        case Qt::Key_Z: {
            if (dashboard) {
                m_skillButtonSank = !m_skillButtonSank;
                dashboard->updateSkillButton();
            }
            break;
        }
    }
}

void RoomScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QGraphicsScene::contextMenuEvent(event);

    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());

    if (item && item->zValue() < -99999) { // @todo_P: tableBg?
        QMenu *menu = miscellaneous_menu;
        menu->clear();
        menu->setTitle(tr("Miscellaneous"));

        QMenu *private_pile = menu->addMenu(tr("Private Piles"));

        bool enabled = false;
        foreach (PlayerCardContainer *container, item2player.keys()) {
            const ClientPlayer *player = item2player.value(container, NULL);
            QStringList piles = player->getPileNames();
            if (!piles.isEmpty()) {
                foreach (const QString &pile_name, piles) {
                    bool add = false;
                    foreach (int id, player->getPile(pile_name)) {
                        if (id != Card::S_UNKNOWN_CARD_ID) {
                            add = true;
                            break;
                        }
                    }
                    if (add) {
                        enabled = true;
                        QAction *action = private_pile->addAction(QString("%1 %2")
                            .arg(ClientInstance->getPlayerName(player->objectName()))
                            .arg(Sanguosha->translate(pile_name)));
                        action->setData(QString("%1.%2").arg(player->objectName()).arg(pile_name));
                        connect(action, &QAction::triggered, this, &RoomScene::showPlayerCards);
                    }
                }
            }
        }
        private_pile->setEnabled(enabled);
        menu->addSeparator();

        if (ServerInfo.EnableCheat) {
            QMenu *known_cards = menu->addMenu(tr("Known cards"));

            foreach (PlayerCardContainer *container, item2player.keys()) {
                const ClientPlayer *player = item2player.value(container, NULL);
                if (player == Self) continue;
                QList<const Card *> known = player->getHandcards();
                if (known.isEmpty()) {
                    known_cards->addAction(ClientInstance->getPlayerName(player->objectName()))->setEnabled(false);
                } else {
                    QMenu *submenu = known_cards->addMenu(ClientInstance->getPlayerName(player->objectName()));
                    QAction *action = submenu->addAction(tr("View in new dialog"));
                    action->setData(player->objectName());
                    connect(action, &QAction::triggered, this, &RoomScene::showPlayerCards);

                    submenu->addSeparator();
                    foreach (const Card *card, known) {
                        const Card *engine_card = Sanguosha->getEngineCard(card->getId());
                        submenu->addAction(G_ROOM_SKIN.getCardSuitPixmap(engine_card->getSuit()), engine_card->getFullName());
                    }
                }
            }
            menu->addSeparator();
        }

        QAction *distance = menu->addAction(tr("View distance"));
        connect(distance, &QAction::triggered, this, &RoomScene::viewDistance);
        QAction *discard = menu->addAction(tr("View Discard pile"));
        connect(discard, &QAction::triggered, this, &RoomScene::toggleDiscards);

        menu->popup(event->screenPos());
    }
}

void RoomScene::chooseGeneral(const QStringList &generals, const bool single_result, const bool can_convert)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("prelude");

    if (generals.isEmpty()) {
        delete m_choiceDialog;
        m_choiceDialog = new FreeChooseDialog(main_window);
    } else {
        m_chooseGeneralBox->chooseGeneral(generals, false, single_result, QString(), NULL, can_convert);
    }
}

void RoomScene::chooseSuit(const QStringList &suits)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("pop-up");

    m_chooseSuitBox->chooseSuit(suits);
}

void RoomScene::chooseKingdom(const QStringList &kingdoms)
{
    QDialog *dialog = new QDialog;
    QVBoxLayout *layout = new QVBoxLayout;

    foreach (const QString &kingdom, kingdoms) {
        QCommandLinkButton *button = new QCommandLinkButton;
        QPixmap kingdom_pixmap(QString("image/kingdom/icon/%1.png").arg(kingdom));
        QIcon kingdom_icon(kingdom_pixmap);

        button->setIcon(kingdom_icon);
        button->setIconSize(kingdom_pixmap.size());
        button->setText(Sanguosha->translate(kingdom));
        button->setObjectName(kingdom);

        layout->addWidget(button);

        connect(button, &QCommandLinkButton::clicked, ClientInstance, &Client::onPlayerChooseKingdom);
        connect(button, &QCommandLinkButton::clicked, dialog, &QDialog::accept);
    }

    //dialog->setObjectName("."); //duplicate??
    connect(dialog, &QDialog::rejected, ClientInstance, &Client::onPlayerChooseKingdom);

    dialog->setObjectName(".");
    dialog->setWindowTitle(tr("Please choose a kingdom"));
    dialog->setLayout(layout);
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

void RoomScene::chooseOption(const QString &skillName, const QStringList &options)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("pop-up");

    m_chooseOptionsBox->setSkillName(skillName);
    m_chooseOptionsBox->chooseOption(options);
}

void RoomScene::chooseCard(const ClientPlayer *player, const QString &flags, const QString &reason,
    bool handcard_visible, Card::HandlingMethod method, QList<int> disabled_ids, QList<int> handcards)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("pop-up");

    m_playerCardBox->chooseCard(Sanguosha->translate(reason), player, flags,
        handcard_visible, method, disabled_ids, handcards);
}

/*void RoomScene::chooseOrder(QSanProtocol::Game3v3ChooseOrderCommand reason) {
    QDialog *dialog = new QDialog;
    if (reason == S_REASON_CHOOSE_ORDER_SELECT)
    dialog->setWindowTitle(tr("The order who first choose general"));
    else if (reason == S_REASON_CHOOSE_ORDER_TURN)
    dialog->setWindowTitle(tr("The order who first in turn"));

    QLabel *prompt = new QLabel(tr("Please select the order"));
    OptionButton *warm_button = new OptionButton("image/system/3v3/warm.png", tr("Warm"));
    warm_button->setObjectName("warm");
    OptionButton *cool_button = new OptionButton("image/system/3v3/cool.png", tr("Cool"));
    cool_button->setObjectName("cool");

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(warm_button);
    hlayout->addWidget(cool_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(prompt);
    layout->addLayout(hlayout);
    dialog->setLayout(layout);

    connect(warm_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
    connect(cool_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
    connect(warm_button, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(cool_button, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseOrder()));
    delete m_choiceDialog;
    m_choiceDialog = dialog;
    }*/
/*
void RoomScene::chooseRole(const QString &scheme, const QStringList &roles) {
QDialog *dialog = new QDialog;
dialog->setWindowTitle(tr("Select role in 3v3 mode"));

QLabel *prompt = new QLabel(tr("Please select a role"));
QVBoxLayout *layout = new QVBoxLayout;

layout->addWidget(prompt);

static QMap<QString, QString> jargon;
if (jargon.isEmpty()) {
jargon["lord"] = tr("Warm leader");
jargon["loyalist"] = tr("Warm guard");
jargon["renegade"] = tr("Cool leader");
jargon["rebel"] = tr("Cool guard");

jargon["leader1"] = tr("Leader of Team 1");
jargon["guard1"] = tr("Guard of Team 1");
jargon["leader2"] = tr("Leader of Team 2");
jargon["guard2"] = tr("Guard of Team 2");
}

foreach (const QString &role, roles) {
QCommandLinkButton *button = new QCommandLinkButton(jargon[role]);
if (scheme == "AllRoles")
button->setIcon(QIcon(QString("image/system/roles/%1.png").arg(role)));
layout->addWidget(button);
button->setObjectName(role);
connect(button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseRole3v3()));
connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
}

QCommandLinkButton *abstain_button = new QCommandLinkButton(tr("Abstain"));
connect(abstain_button, SIGNAL(clicked()), dialog, SLOT(reject()));
layout->addWidget(abstain_button);

dialog->setObjectName("abstain");
connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseRole3v3()));

dialog->setLayout(layout);
delete m_choiceDialog;
m_choiceDialog = dialog;
}
*/
/*void RoomScene::chooseDirection() {
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle(tr("Please select the direction"));

    QLabel *prompt = new QLabel(dialog->windowTitle());

    OptionButton *cw_button = new OptionButton("image/system/3v3/cw.png", tr("CW"));
    cw_button->setObjectName("cw");

    OptionButton *ccw_button = new OptionButton("image/system/3v3/ccw.png", tr("CCW"));
    ccw_button->setObjectName("ccw");

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(cw_button);
    hlayout->addWidget(ccw_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(prompt);
    layout->addLayout(hlayout);
    dialog->setLayout(layout);

    dialog->setObjectName("ccw");
    connect(ccw_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerMakeChoice()));
    connect(ccw_button, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(cw_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerMakeChoice()));
    connect(cw_button, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerMakeChoice()));
    delete m_choiceDialog;
    m_choiceDialog = dialog;
    }*/

void RoomScene::chooseTriggerOrder(const QString &reason, const QStringList &options, const bool optional)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("pop-up");

    m_chooseTriggerOrderBox->chooseOption(reason, options, optional);
}

void RoomScene::toggleDiscards()
{
    CardOverview *overview = new CardOverview;
    overview->setWindowTitle(tr("Discarded pile"));
    QList<const Card *> cards;
    foreach(const Card *card, ClientInstance->discarded_list)
        cards << Sanguosha->getEngineCard(card->getId());
    overview->loadFromList(cards);
    overview->show();
}

GenericCardContainer *RoomScene::_getGenericCardContainer(Player::Place place, Player *player)
{
    if (place == Player::DiscardPile || place == Player::PlaceJudge
        || place == Player::DrawPile || place == Player::PlaceTable
        || place == Player::DrawPileBottom)
        return m_tablePile;
    // @todo: AG must be a pile with name rather than simply using the name special...
    else if (player == NULL && place == Player::PlaceSpecial)
        return pileContainer;
    else if (player == Self)
        return dashboard;
    else if (player != NULL)
        return name2photo.value(player->objectName(), NULL);
    else Q_ASSERT(false);
    return NULL;
}

bool RoomScene::_shouldIgnoreDisplayMove(CardsMoveStruct &movement)
{
    Player::Place from = movement.from_place, to = movement.to_place;
    QString from_pile = movement.from_pile_name, to_pile = movement.to_pile_name;
    if ((from == Player::PlaceSpecial && !from_pile.isEmpty() && from_pile.startsWith('#'))
        || (to == Player::PlaceSpecial && !to_pile.isEmpty() && to_pile.startsWith('#')))
        return true;
    else {
        static QList<Player::Place> ignore_place;
        if (ignore_place.isEmpty())
            ignore_place << Player::DiscardPile << Player::PlaceTable << Player::PlaceJudge;
        return ignore_place.contains(from) && ignore_place.contains(to);
    }
}

bool RoomScene::_processCardsMove(CardsMoveStruct &move, bool isLost)
{
    _MoveCardsClassifier cls(move);
    // delayed trick processed;
    if (move.from_place == Player::PlaceDelayedTrick && move.to_place == Player::PlaceTable) {
        if (isLost) m_move_cache[cls] = move;
        return true;
    }
    CardsMoveStruct tmpMove = m_move_cache.value(cls, CardsMoveStruct());
    if (tmpMove.from_place != Player::PlaceUnknown) {
        move.from = tmpMove.from;
        move.from_place = tmpMove.from_place;
        move.from_pile_name = tmpMove.from_pile_name;
    }
    if (!isLost) m_move_cache.remove(cls);
    return false;
}

void RoomScene::getCards(int moveId, QList<CardsMoveStruct> card_moves)
{
    int count = 0;
    for (int i = 0; i < card_moves.size(); i++) {
        CardsMoveStruct &movement = card_moves[i];
        bool skipMove = _processCardsMove(movement, false);
        if (skipMove) continue;
        if (_shouldIgnoreDisplayMove(movement)) continue;
        m_cardContainer->m_currentPlayer = qobject_cast<ClientPlayer *>(movement.to);
        GenericCardContainer *to_container = _getGenericCardContainer(movement.to_place, movement.to);
        QList<CardItem *> cards = _m_cardsMoveStash[moveId][count];
        CardMoveReason reason = movement.reason;
        count++;
        for (int j = 0; j < cards.size(); j++) {
            CardItem *card = cards[j];
            card->setFlag(QGraphicsItem::ItemIsMovable, false);
            if (!reason.m_skillName.isEmpty() && movement.from && movement.to_place != Player::PlaceHand && movement.to_place != Player::PlaceSpecial
                    && movement.to_place != Player::PlaceEquip && movement.to_place != Player::PlaceDelayedTrick) {
                ClientPlayer *target = ClientInstance->getPlayer(movement.from->objectName());
                if (!reason.m_playerId.isEmpty() && reason.m_playerId != movement.from->objectName()) target = ClientInstance->getPlayer(reason.m_playerId);
                if (!reason.m_eventName.isEmpty() && reason.m_eventName == target->getActualGeneral1Name()
                        || reason.m_eventName == target->getActualGeneral2Name())
                    card->showAvatar(reason.m_eventName == target->getActualGeneral1Name() ? target->getActualGeneral1() : target->getActualGeneral2(), reason.m_skillName);
                else if (target->hasSkill(reason.m_skillName) && !target->getSkillList().contains(Sanguosha->getSkill(reason.m_skillName)))
                    card->showAvatar(target->hasShownGeneral1() ? target->getGeneral() : target->getGeneral2(), reason.m_skillName);
                else if (target->inHeadSkills(reason.m_skillName) || (target->getActualGeneral1() ? target->getActualGeneral1()->hasSkill(reason.m_skillName) : false))
                    card->showAvatar(target->getActualGeneral1(), reason.m_skillName);
                else if (target->inDeputySkills(reason.m_skillName) || (target->getActualGeneral2() ? target->getActualGeneral2()->hasSkill(reason.m_skillName) : false))
                    card->showAvatar(target->getActualGeneral2(), reason.m_skillName);
            }

            int card_id = card->getId();
            if (!card_moves[i].card_ids.contains(card_id)) {
                card->setVisible(false);
                card->deleteLater();
                cards.removeAt(j);
                j--;
            } else {
                card->setEnabled(true);
            }
            card->setFootnote(_translateMovement(movement));
            card->hideFootnote();
            if (Self->hasFlag("marshalling") && movement.to_place == Player::PlaceHand && movement.to->objectName() == Self->objectName())
                card->setOpacity(1);
        }
        bringToFront(to_container);
        to_container->addCardItems(cards, movement);
        keepGetCardLog(movement);
    }
    _m_cardsMoveStash[moveId].clear();
}

void RoomScene::loseCards(int moveId, QList<CardsMoveStruct> card_moves)
{
    for (int i = 0; i < card_moves.size(); i++) {
        CardsMoveStruct &movement = card_moves[i];
        bool skipMove = _processCardsMove(movement, true);
        if (skipMove) continue;
        if (_shouldIgnoreDisplayMove(movement)) continue;
        m_cardContainer->m_currentPlayer = qobject_cast<ClientPlayer *>(movement.to);
        GenericCardContainer *from_container = _getGenericCardContainer(movement.from_place, movement.from);
        QList<CardItem *> cards = from_container->removeCardItems(movement.card_ids, movement.from_place);
        foreach(CardItem *card, cards)
            card->setEnabled(false);

        _m_cardsMoveStash[moveId].append(cards);
        keepLoseCardLog(movement);
    }
}

QString RoomScene::_translateMovement(const CardsMoveStruct &move)
{
    CardMoveReason reason = move.reason;
    if (reason.m_reason == CardMoveReason::S_REASON_UNKNOWN) return QString();
    Photo *srcPhoto = name2photo[reason.m_playerId];
    Photo *dstPhoto = name2photo[reason.m_targetId];
    QString playerName, targetName;

    if (srcPhoto != NULL)
        playerName = Sanguosha->translate(srcPhoto->getPlayer()->getFootnoteName());
    else if (reason.m_playerId == Self->objectName())
        playerName = QString("%1(%2)").arg(Sanguosha->translate(Self->getFootnoteName())).arg(tr("yourself"));

    if (dstPhoto != NULL) {
        targetName = tr("use upon %1").arg(Sanguosha->translate(dstPhoto->getPlayer()->getFootnoteName()));
    } else if (reason.m_targetId == Self->objectName()) {
        targetName = tr("use upon %1(%2)")
            .arg(Sanguosha->translate(Self->getFootnoteName()))
            .arg(tr("yourself"));
    }

    QString result(playerName + targetName);
    result.append(Sanguosha->translate(reason.m_eventName));
    //result.append(Sanguosha->translate(reason.m_skillName));
    if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE && reason.m_skillName.isEmpty()) {
        result.append(Sanguosha->translate("use"));
    } else if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
        if (reason.m_reason == CardMoveReason::S_REASON_RETRIAL)
            result.append(Sanguosha->translate("retrial"));
        else if (reason.m_skillName.isEmpty())
            result.append(Sanguosha->translate("response"));
    } else if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
        if (reason.m_reason == CardMoveReason::S_REASON_RULEDISCARD)
            result.append(Sanguosha->translate("discard"));
        else if (reason.m_reason == CardMoveReason::S_REASON_THROW)
            result.append(Sanguosha->translate("throw"));
        else if (reason.m_reason == CardMoveReason::S_REASON_CHANGE_EQUIP)
            result.append(Sanguosha->translate("change equip"));
        else if (reason.m_reason == CardMoveReason::S_REASON_DISMANTLE)
            result.append(Sanguosha->translate("throw"));
    } else if (reason.m_reason == CardMoveReason::S_REASON_RECAST) {
        result.append(Sanguosha->translate("recast"));
    } else if (reason.m_reason == CardMoveReason::S_REASON_PINDIAN) {
        result.append(Sanguosha->translate("pindian"));
    } else if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_SHOW) {
        if (reason.m_reason == CardMoveReason::S_REASON_JUDGE)
            result.append(Sanguosha->translate("judge"));
        else if (reason.m_reason == CardMoveReason::S_REASON_TURNOVER)
            result.append(Sanguosha->translate("turnover"));
        else if (reason.m_reason == CardMoveReason::S_REASON_DEMONSTRATE)
            result.append(Sanguosha->translate("show"));
        else if (reason.m_reason == CardMoveReason::S_REASON_PREVIEW)
            result.append(Sanguosha->translate("preview"));
    } else if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_PUT) {
        if (reason.m_reason == CardMoveReason::S_REASON_PUT) {
            result.append(Sanguosha->translate("put"));
            if (move.to_place == Player::DiscardPile)
                result.append(Sanguosha->translate("discardPile"));
            else if (move.to_place == Player::DrawPile)
                result.append(Sanguosha->translate("drawPileTop"));
            else if (move.to_place == Player::DrawPileBottom)
                result.append(Sanguosha->translate("drawPileBottom"));
        } else if (reason.m_reason == CardMoveReason::S_REASON_NATURAL_ENTER) {
            result.append(Sanguosha->translate("enter"));
            if (move.to_place == Player::DiscardPile)
                result.append(Sanguosha->translate("discardPile"));
            else if (move.to_place == Player::DrawPile)
                result.append(Sanguosha->translate("drawPileTop"));
            else if (move.to_place == Player::DrawPileBottom)
                result.append(Sanguosha->translate("drawPileBottom"));
        } else if (reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE) {
            result.append(Sanguosha->translate("judgedone"));
        } else if (reason.m_reason == CardMoveReason::S_REASON_REMOVE_FROM_PILE) {
            result.append(Sanguosha->translate("backinto"));
        }
    }
    return result;
}

void RoomScene::keepLoseCardLog(const CardsMoveStruct &move)
{
    if (move.from && (move.to_place == Player::DrawPile || move.to_place == Player::DrawPileBottom)) {
        if (move.reason.m_reason == CardMoveReason::S_REASON_PUT && move.reason.m_skillName == "luck_card") return;
        bool hidden = false;
        foreach (int id, move.card_ids) {
            if (id == Card::S_UNKNOWN_CARD_ID) {
                hidden = true;
                break;
            }
        }

        QString type(hidden ? '#' : '$');
        type.append(move.to_place == Player::DrawPile ? "PutCard" : "PutCardToDrawPileBottom");
        QString from_general = move.from->objectName();
        if (hidden)
            log_box->appendLog(type, from_general, QStringList(), QString(), QString::number(move.card_ids.length()));
        else
            log_box->appendLog(type, from_general, QStringList(), IntList2StringList(move.card_ids).join("+"));
    }
}

void RoomScene::keepGetCardLog(const CardsMoveStruct &move)
{
    if (move.card_ids.isEmpty()) return;
    if (move.to
        && (move.to_place == Player::PlaceHand
        || move.to_place == Player::PlaceEquip
        || move.to_place == Player::PlaceSpecial)
        && move.from_place != Player::DrawPile) {
        foreach(const QString &flag, move.to->getFlagList())
            if (flag.endsWith("_InTempMoving"))
                return;
    }

    // private pile
    if (move.to_place == Player::PlaceSpecial && !move.to_pile_name.isNull() && !move.to_pile_name.startsWith('#')) {
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (hidden)
            log_box->appendLog("#RemoveFromGame", QString(), QStringList(), QString(), move.to_pile_name, QString::number(move.card_ids.length()));
        else
            log_box->appendLog("$AddToPile", QString(), QStringList(), IntList2StringList(move.card_ids).join("+"), move.to_pile_name);
    }
    if (move.from_place == Player::PlaceSpecial && move.to
        && move.reason.m_reason == CardMoveReason::S_REASON_EXCHANGE_FROM_PILE) {
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (!hidden)
            log_box->appendLog("$GotCardFromPile", move.to->objectName(), QStringList(), IntList2StringList(move.card_ids).join("+"), move.from_pile_name);
        else
            log_box->appendLog("#GotNCardFromPile", move.to->objectName(), QStringList(), QString(), move.from_pile_name, QString::number(move.card_ids.length()));
    }
    //DrawNCards
    if (move.from_place == Player::DrawPile && move.to_place == Player::PlaceHand) {
        QString to_general = move.to->objectName();
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (!hidden)
            log_box->appendLog("$DrawCards", to_general, QStringList(), IntList2StringList(move.card_ids).join("+"),
            QString::number(move.card_ids.length()));
        else
            log_box->appendLog("#DrawNCards", to_general, QStringList(), QString(),
            QString::number(move.card_ids.length()));

    }
    if ((move.from_place == Player::PlaceTable || move.from_place == Player::PlaceJudge)
        && move.to_place == Player::PlaceHand
        && move.reason.m_reason != CardMoveReason::S_REASON_PREVIEW) {
        QString to_general = move.to->objectName();
        QList<int> ids = move.card_ids;
        ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        if (!ids.isEmpty()) {
            QString card_str = IntList2StringList(ids).join("+");
            log_box->appendLog("$GotCardBack", to_general, QStringList(), card_str);
        }
    }
    if (move.from_place == Player::DiscardPile && move.to_place == Player::PlaceHand) {
        QString to_general = move.to->objectName();
        QString card_str = IntList2StringList(move.card_ids).join("+");
        log_box->appendLog("$RecycleCard", to_general, QStringList(), card_str);
    }
    if (move.from && move.from_place != Player::PlaceHand && move.to_place != Player::PlaceDelayedTrick
        && move.from_place != Player::PlaceJudge && move.to && move.from != move.to) {
        QString from_general = move.from->objectName();
        QStringList tos;
        tos << move.to->objectName();
        QList<int> ids = move.card_ids;
        ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        int hide = move.card_ids.length() - ids.length();
        if (!ids.isEmpty()) {
            QString card_str = IntList2StringList(ids).join("+");
            log_box->appendLog("$MoveCard", from_general, tos, card_str);
        }
        if (hide > 0)
            log_box->appendLog("#MoveNCards", from_general, tos, QString(), QString::number(hide));
    }
    if (move.from_place == Player::PlaceHand && move.to_place == Player::PlaceHand && move.from && move.to) {
        QString from_general = move.from->objectName();
        QStringList tos;
        tos << move.to->objectName();
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (hidden)
            log_box->appendLog("#MoveNCards", from_general, tos, QString(), QString::number(move.card_ids.length()));
        else
            log_box->appendLog("$MoveCard", from_general, tos, IntList2StringList(move.card_ids).join("+"));
    }
    if (move.from && move.to) {
        // both src and dest are player
        QString type;
        if (move.to_place == Player::PlaceDelayedTrick) {
            if (move.from_place == Player::PlaceDelayedTrick && move.from != move.to)
                type = "$LightningMove";
            else
                type = "$PasteCard";
        }
        if (!type.isNull()) {
            QString from_general = move.from->objectName();
            QStringList tos;
            tos << move.to->objectName();
            log_box->appendLog(type, from_general, tos, QString::number(move.card_ids.first()));
        }
    }
    if (move.from && move.to && move.from_place == Player::PlaceEquip && move.to_place == Player::PlaceEquip) {
        QString type = "$Install";
        QString to_general = move.to->objectName();
        foreach(int card_id, move.card_ids)
            log_box->appendLog(type, to_general, QStringList(), QString::number(card_id));
    }
    if (move.reason.m_reason == CardMoveReason::S_REASON_TURNOVER)
        log_box->appendLog("$TurnOver", move.reason.m_playerId, QStringList(), IntList2StringList(move.card_ids).join("+"));
}

void RoomScene::addSkillButton(const Skill *skill, const bool &head)
{
    // check duplication
    QSanSkillButton *btn = dashboard->addSkillButton(skill->objectName(), head);
    if (btn == NULL) return;

    if (btn->getViewAsSkill() != NULL  && !m_replayControl) {
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_activated), dashboard, &Dashboard::skillButtonActivated);
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_activated), this, &RoomScene::onSkillActivated);
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_deactivated), dashboard, &Dashboard::skillButtonDeactivated);
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_deactivated), this, &RoomScene::onSkillDeactivated);
    }
    QDialog *dialog = skill->getDialog();
    if (dialog && !m_replayControl) {
        dialog->setParent(main_window, Qt::Dialog);
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_activated), dialog, &QDialog::exec); //???????????????????????
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_deactivated), dialog, &QDialog::reject);
        // design for guhuo and qice. now it is just for DIY.
    }

    QString guhuo_type = skill->getGuhuoBox();
    if (guhuo_type != "" && !m_replayControl) {
        GuhuoBox *guhuo = new GuhuoBox(btn->getSkill()->objectName(), guhuo_type, !guhuo_type.startsWith("!"));
        guhuo->setParent(this);
        addItem(guhuo);
        guhuo->hide();
        guhuo->setZValue(30001);
        guhuo_items[guhuo->getSkillName()] = guhuo;
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_activated), guhuo, &GuhuoBox::popup);
        connect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_deactivated), guhuo, &GuhuoBox::clear);
        disconnect(btn, (void (QSanSkillButton::*)())(&QSanSkillButton::skill_activated), this, &RoomScene::onSkillActivated);
        connect(guhuo, &GuhuoBox::onButtonClick, this, &RoomScene::onSkillActivated);
    }
    m_skillButtons.append(btn);
}

void RoomScene::acquireSkill(const ClientPlayer *player, const QString &skill_name, const bool &head)
{
    QString type = "#AcquireSkill";
    QString from_general = player->objectName();
    QString arg = skill_name;
    log_box->appendLog(type, from_general, QStringList(), QString(), arg);

    if (player == Self) addSkillButton(Sanguosha->getSkill(skill_name), head);
}

void RoomScene::updateSkillButtons()
{
    foreach (const Skill *skill, Self->getHeadSkillList(true, true)) {
        if (Self->isDuanchang(Self->inHeadSkills(skill->objectName())))
            continue;

        addSkillButton(skill, true);
    }
    foreach (const Skill *skill, Self->getDeputySkillList(true, true)) {
        if (Self->isDuanchang(Self->inDeputySkills(skill->objectName())))
            continue;

        addSkillButton(skill, false);
    }

    // Do not disable all skill buttons for we need to preshow some skills
    //foreach (QSanSkillButton *button, m_skillButtons)
    //    button->setEnabled(false);

    foreach (QSanSkillButton *button, m_skillButtons) {
        const Skill *skill = button->getSkill();
        bool head = button->objectName() == "left";
        button->setEnabled(skill->canPreshow()
            && !Self->hasShownSkill(skill));
        if (skill->canPreshow() && Self->ownSkill(skill) && (head ? !Self->hasShownGeneral1() : !Self->hasShownGeneral2())) {
            if (Self->hasPreshowedSkill(skill->objectName(), head))
                button->setState(QSanButton::S_STATE_DISABLED);
            else
                button->setState(QSanButton::S_STATE_CANPRESHOW);
        }
    }
}

void RoomScene::useSelectedCard()
{
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
        case Client::Playing: {
            const Card *card = dashboard->getSelected();
            if (card) useCard(card);
            break;
        }
        case Client::Responding: {
            const Card *card = dashboard->getSelected();
            if (card) {
                if (ClientInstance->getStatus() == Client::Responding) {
                    Q_ASSERT(selected_targets.isEmpty());
                    selected_targets.clear();
                }
                ClientInstance->onPlayerResponseCard(card, selected_targets);
                prompt_box->disappear();
            }

            dashboard->unselectAll();
            break;
        }
        case Client::AskForShowOrPindian: {
            const Card *card = dashboard->getSelected();
            if (card) {
                ClientInstance->onPlayerResponseCard(card);
                prompt_box->disappear();
            }
            dashboard->unselectAll();
            break;
        }
        case Client::Discarding:{
            const Card *card = dashboard->getPendingCard();
            if (card) {
                ClientInstance->onPlayerDiscardCards(card);
                dashboard->stopPending();
                prompt_box->disappear();
            }
            break;
        }
        case Client::Exchanging: {
            const Card *card = dashboard->getPendingCard();
            if (card) {
                ClientInstance->onPlayerDiscardCards(card);
                dashboard->stopPending();
                prompt_box->disappear();
            }
            break;
        }
        case Client::NotActive: {
            QMessageBox::warning(main_window, tr("Warning"),
                tr("The OK button should be disabled when client is not active!"));
            return;
        }
        case Client::AskForAG: {
            ClientInstance->onPlayerChooseAG(-1);
            return;
        }
        case Client::ExecDialog: {
            QMessageBox::warning(main_window, tr("Warning"),
                tr("The OK button should be disabled when client is in executing dialog"));
            return;
        }
        case Client::AskForSkillInvoke: {
            prompt_box->disappear();
            QString skill_name = ClientInstance->getSkillNameToInvoke();
            dashboard->highlightEquip(skill_name, false);
            ClientInstance->onPlayerInvokeSkill(true);
            break;
        }
        case Client::AskForPlayerChoose: {
            ClientInstance->onPlayerChoosePlayer(selected_targets);
            prompt_box->disappear();
            break;
        }
        case Client::GlobalCardChosen: {
            enableTargets(NULL);
            foreach (const ClientPlayer *p, card_boxes.keys())
                card_boxes[p]->clear();
            ClientInstance->onPlayerChooseCards(selected_ids);
            prompt_box->disappear();
            break;
        }
        case Client::AskForYiji: {
            const Card *card = dashboard->getPendingCard();
            if (card) {
                ClientInstance->onPlayerReplyYiji(card, selected_targets.first());
                dashboard->stopPending();
                prompt_box->disappear();
            }
            break;
        }
        case Client::AskForGuanxing: {
            m_guanxingBox->reply();
            break;
        }
        case Client::AskForMoveCards: {
            m_cardchooseBox->reply();
            break;
        }
        case Client::AskForGongxin: {
            ClientInstance->onPlayerReplyGongxin();
            m_cardContainer->clear();
            break;
        }
    }

    const ViewAsSkill *skill = dashboard->currentSkill();
    if (skill)
        dashboard->stopPending();
    else {
        foreach (const QString &pile, Self->getHandPileList(false))
            dashboard->retractPileCards(pile);
    }
}

void RoomScene::onEnabledChange()
{
    QGraphicsItem *photo = qobject_cast<QGraphicsItem *>(sender());
    if (!photo) return;
    if (photo->isEnabled())
        animations->effectOut(photo);
    else
        animations->sendBack(photo);
}

void RoomScene::useCard(const Card *card)
{
    if (card->targetFixed() || card->targetsFeasible(selected_targets, Self))
        ClientInstance->onPlayerResponseCard(card, selected_targets);
    enableTargets(NULL);
}

void RoomScene::callViewAsSkill()
{
    const Card *card = dashboard->getPendingCard();
    if (card == NULL) return;

    if (card->isAvailable(Self)) {
        // use card
        dashboard->stopPending();
        useCard(card);
    }
}

void RoomScene::cancelViewAsSkill()
{
    dashboard->stopPending();
    Client::Status status = ClientInstance->getStatus();
    updateStatus(status, status);
}

void RoomScene::highlightSkillButton(const QString &skillName, const CardUseStruct::CardUseReason reason, const QString &pattern, const QString &head)
{
    const bool isViewAsSkill = reason != CardUseStruct::CARD_USE_REASON_UNKNOWN && !pattern.isEmpty();
    if (Self->hasSkill(skillName, true)) {
        foreach (QSanSkillButton *button, m_skillButtons) {
            Q_ASSERT(button != NULL);
            if (!head.isEmpty() && button->objectName() != head) continue;
            if (isViewAsSkill) {
                const ViewAsSkill *vsSkill = button->getViewAsSkill();
                if (vsSkill != NULL && vsSkill->objectName() == skillName
                    && vsSkill->isAvailable(Self, reason, pattern)) {
                    button->setEnabled(true);
                    button->setState(QSanButton::S_STATE_DOWN);
                    button->skill_activated();
                    break;
                }
            } else {
                const Skill *skill = button->getSkill();
                if (skill != NULL && skill->objectName() == skillName) {
                    button->setEnabled(true);
                    button->setState(QSanButton::S_STATE_DOWN);
                    break;
                }
            }
        }
    }
}

void RoomScene::selectTarget(int order, bool multiple)
{
    if (!multiple) unselectAllTargets();

    QGraphicsItem *to_select = NULL;
    if (order == 0)
        to_select = dashboard;
    else if (order > 0 && order <= photos.length())
        to_select = photos.at(order - 1);

    if (!to_select) return;
    if (!(to_select->isSelected()
        || (!to_select->isSelected() && (to_select->flags() & QGraphicsItem::ItemIsSelectable))))
        return;

    to_select->setSelected(!to_select->isSelected());
}

void RoomScene::selectNextTarget(bool multiple)
{
    if (!multiple)
        unselectAllTargets();

    QList<QGraphicsItem *> targets;
    foreach (Photo *photo, photos) {
        if (photo->flags() & QGraphicsItem::ItemIsSelectable)
            targets << photo;
    }

    if (dashboard->flags() & QGraphicsItem::ItemIsSelectable)
        targets << dashboard;

    for (int i = 0; i < targets.length(); i++) {
        if (targets.at(i)->isSelected()) {
            for (int j = i + 1; j < targets.length(); j++) {
                if (!targets.at(j)->isSelected()) {
                    targets.at(j)->setSelected(true);
                    return;
                }
            }
        }
    }

    foreach (QGraphicsItem *target, targets) {
        if (!target->isSelected()) {
            target->setSelected(true);
            break;
        }
    }
}

void RoomScene::unselectAllTargets(const QGraphicsItem *except)
{
    if (dashboard != except) dashboard->setSelected(false);

    foreach (Photo *photo, photos) {
        if (photo != except && photo->isSelected())
            photo->setSelected(false);
    }
}

void RoomScene::doTimeout()
{
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
        case Client::Playing: {
            discard_button->click();
            break;
        }
        case Client::Responding:
        case Client::Discarding:
        case Client::Exchanging:
        case Client::ExecDialog:
        case Client::AskForShowOrPindian: {
            doCancelButton();
            break;
        }
        case Client::AskForPlayerChoose: {
            QList<const Player*> null;
            ClientInstance->onPlayerChoosePlayer(null);
            dashboard->stopPending();
            prompt_box->disappear();
            dashboard->highlightEquip(ClientInstance->skill_name, false);
            break;
        }
        case Client::GlobalCardChosen: {
            enableTargets(NULL);
            foreach (const ClientPlayer *p, card_boxes.keys())
                card_boxes[p]->clear();
            if (ok_button->isEnabled())
                ok_button->click();
            else
                ClientInstance->onPlayerChooseCards(QList<int>());
            prompt_box->disappear();
            dashboard->highlightEquip(ClientInstance->skill_name, false);
            break;
        }
        case Client::AskForAG: {
            int card_id = m_cardContainer->getFirstEnabled();
            if (card_id != -1)
                ClientInstance->onPlayerChooseAG(card_id);
            break;
        }
        case Client::AskForSkillInvoke: {
            cancel_button->click();
            break;
        }
        case Client::AskForYiji: {
            if (cancel_button->isEnabled())
                cancel_button->click();
            else {
                prompt_box->disappear();
                doCancelButton();
            }
            break;
        }
        case Client::AskForMoveCards:{
            if (ClientInstance->m_isDiscardActionRefusable)
                ok_button->click();
            else {
                QList<int> empty;
                dashboard->highlightEquip(ClientInstance->skill_name, false);
                m_cardchooseBox->clear();
                ClientInstance->onPlayerReplyMoveCards(empty, empty);
            }
            break;
        }
        case Client::AskForGuanxing:
        case Client::AskForGongxin: {
            ok_button->click();
            break;
        }
        case Client::AskForArrangement: {
            arrange_items << down_generals.mid(0, 3 - arrange_items.length());
            finishArrange();
        }
        default:
            break;
    }
}

void RoomScene::showPromptBox()
{
    bringToFront(prompt_box);
    prompt_box->appear();
}

void RoomScene::updateStatus(Client::Status oldStatus, Client::Status newStatus)
{
    //--------------------------------------------
    if (!dashboard->getTransferButtons().isEmpty()) {
        bool enabled = false;
        if (newStatus == Client::Playing) {
            enabled = Sanguosha->getTransfer()->isAvailable(Self,
                CardUseStruct::CARD_USE_REASON_PLAY,
                Sanguosha->currentRoomState()->getCurrentCardUsePattern());
        }
        foreach (TransferButton *button, dashboard->getTransferButtons()) {
            button->setEnabled(enabled);
            if (enabled)
                button->getCardItem()->setTransferable(true);
        }
    }
    //---------------------------------------------------
    foreach (QSanSkillButton *button, m_skillButtons) {
        Q_ASSERT(button != NULL);
        const ViewAsSkill *vsSkill = button->getViewAsSkill();
        if (vsSkill != NULL) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            QRegExp rx("@@?([_A-Za-z]+)(\\d+)?!?");
            CardUseStruct::CardUseReason reason = CardUseStruct::CARD_USE_REASON_UNKNOWN;
            if ((newStatus & Client::ClientStatusBasicMask) == Client::Responding) {
                if (newStatus == Client::RespondingUse) {
                    reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
                } else if (newStatus == Client::Responding || rx.exactMatch(pattern))
                    reason = CardUseStruct::CARD_USE_REASON_RESPONSE;
            } else if (newStatus == Client::Playing) {
                reason = CardUseStruct::CARD_USE_REASON_PLAY;
            }
            if (!ClientInstance->getSkillToHighLight().isEmpty() && pattern.startsWith("@@")) {
                if (ClientInstance->getSkillToHighLight() == button->objectName())
                    button->setEnabled(vsSkill->isAvailable(Self, reason, pattern) && !pattern.endsWith("!"));
                else
                    button->setEnabled(false);
            } else
                button->setEnabled(vsSkill->isAvailable(Self, reason, pattern) && !pattern.endsWith("!"));
        } else {
            const Skill *skill = button->getSkill();
            if (skill->getFrequency() == Skill::Wake)
                button->setEnabled(Self->getMark(skill->objectName()) > 0);
            else if (QSanButton::S_STATE_CANPRESHOW != button->getState())
                button->setState(QSanButton::S_STATE_DISABLED);
        }
    }

    m_superDragStarted = false;

    switch (newStatus & Client::ClientStatusBasicMask) {
        case Client::NotActive: {
            switch (oldStatus) {
                case Client::ExecDialog: {
                    if (m_choiceDialog != NULL && m_choiceDialog->isVisible())
                        m_choiceDialog->hide();
                    break;
                }
                case Client::AskForMoveCards:{
                    m_cardchooseBox->clear();
                    if (!m_cardContainer->retained())
                        m_cardContainer->clear();
                    break;
                }
                case Client::AskForGuanxing:
                case Client::AskForGongxin: {
                    m_guanxingBox->clear();
                    if (!m_cardContainer->retained())
                        m_cardContainer->clear();
                    break;
                }
                case Client::AskForGeneralChosen: {
                    m_chooseGeneralBox->clear();
                    break;
                }
                case Client::AskForChoice: {
                    m_chooseOptionsBox->clear();
                    break;
                }
                case Client::AskForCardChosen: {
                    m_playerCardBox->clear();
                    break;
                }
                case Client::AskForSuit: {
                    m_chooseSuitBox->clear();
                    break;
                }
                case Client::AskForTriggerOrder: {
                    m_chooseTriggerOrderBox->clear();
                    break;
                }
                case Client::RespondingUse: {
                    QRegExp promptRegExp("@@?([_A-Za-z]+)");
                    QString prompt = Sanguosha->currentRoomState()->getCurrentCardResponsePrompt();
                    Sanguosha->currentRoomState()->setCurrentCardResponsePrompt(QString());
                    if (promptRegExp.exactMatch(prompt))
                        dashboard->highlightEquip(promptRegExp.capturedTexts().at(1), false);
                }
                default:
                    break;
            }

            prompt_box->disappear();
            ClientInstance->getPromptDoc()->clear();

            dashboard->disableAllCards();
            selected_targets.clear();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(false);

            if (dashboard->currentSkill())
                dashboard->stopPending();

            dashboard->hideProgressBar();

            break;
        }
        case Client::Responding: {
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);

            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            QRegExp rx("@@?([_A-Za-z]+)(\\d+)?!?");
            if (rx.exactMatch(pattern)) {
                QString skill_name = rx.capturedTexts().at(1);
                const ViewAsSkill *skill = Sanguosha->getViewAsSkill(skill_name);
                if (skill) {
                    CardUseStruct::CardUseReason reason = CardUseStruct::CARD_USE_REASON_RESPONSE;
                    if (newStatus == Client::RespondingUse)
                        reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
                    if (!Self->hasFlag(skill_name))
                        Self->setFlags(skill_name);
                    bool available = skill->isAvailable(Self, reason, pattern);
                    Self->setFlags("-" + skill_name);
                    if (!available) {
                        ClientInstance->onPlayerResponseCard(NULL);
                        break;
                    }
                    highlightSkillButton(skill_name, reason, pattern, ClientInstance->getSkillToHighLight());
                    dashboard->startPending(skill);
                    if (skill->inherits("OneCardViewAsSkill") && Config.EnableIntellectualSelection)
                        dashboard->selectOnlyCard();
                }
            } else {
                if (pattern.endsWith("!"))
                    pattern = pattern.mid(0, pattern.length() - 1);
                response_skill->setPattern(pattern);
                if (newStatus == Client::RespondingForDiscard)
                    response_skill->setRequest(Card::MethodDiscard);
                else if (newStatus == Client::RespondingNonTrigger)
                    response_skill->setRequest(Card::MethodNone);
                else if (newStatus == Client::RespondingUse)
                    response_skill->setRequest(Card::MethodUse);
                else
                    response_skill->setRequest(Card::MethodResponse);

                QRegExp promptRegExp("@@?([_A-Za-z]+)");
                QString prompt = Sanguosha->currentRoomState()->getCurrentCardResponsePrompt();
                if (promptRegExp.exactMatch(prompt))
                    dashboard->highlightEquip(promptRegExp.capturedTexts().at(1), true);

                dashboard->startPending(response_skill);
                if (Config.EnableIntellectualSelection)
                    dashboard->selectOnlyCard();
            }
            break;
        }
        case Client::AskForShowOrPindian: {
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(false);

            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            showorpindian_skill->setPattern(pattern);
            dashboard->startPending(showorpindian_skill);

            break;
        }
        case Client::Playing: {
            dashboard->enableCards();
            bringToFront(dashboard);
            ok_button->setEnabled(false);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(true);
            break;
        }
        case Client::Discarding: {
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);

            discard_skill->setNum(ClientInstance->discard_num);
            discard_skill->setMinNum(ClientInstance->min_num);
            discard_skill->setIncludeEquip(ClientInstance->m_canDiscardEquip);
            discard_skill->setIsDiscard(true);
            highlightSkillButton(ClientInstance->discard_reason, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            dashboard->startPending(discard_skill);
            break;
        }
        case Client::Exchanging: {
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);
            exchange_skill->initialize(ClientInstance->exchange_max, ClientInstance->exchange_min,
                ClientInstance->exchange_expand_pile, ClientInstance->exchange_pattern);
            highlightSkillButton(ClientInstance->exchange_reason, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            dashboard->startPending(exchange_skill);
            break;
        }
        case Client::ExecDialog: {
            if (m_choiceDialog != NULL) {
                m_choiceDialog->setParent(main_window, m_choiceDialog->windowFlags() | Qt::Dialog);
                m_choiceDialog->show();
                ok_button->setEnabled(false);
                cancel_button->setEnabled(true);
                discard_button->setEnabled(false);
            }
            break;
        }
        case Client::AskForSkillInvoke: {
            QString skill_name = ClientInstance->getSkillNameToInvoke();
            dashboard->highlightEquip(skill_name, true);
            highlightSkillButton(skill_name, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            showPromptBox();
            ok_button->setEnabled(!ClientInstance->getSkillNameToInvoke().endsWith("!"));
            cancel_button->setEnabled(true);
            discard_button->setEnabled(false);
            break;
        }
        case Client::AskForPlayerChoose: {
            QString skill_name = ClientInstance->getSkillNameToInvoke();
            dashboard->highlightEquip(skill_name, true);
            highlightSkillButton(skill_name, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);

            choose_skill->setPlayerNames(ClientInstance->players_to_choose, ClientInstance->choose_max_num, ClientInstance->choose_min_num);
            dashboard->startPending(choose_skill);

            break;
        }
        case Client::GlobalCardChosen: {
            QString skill_name = ClientInstance->getSkillNameToInvoke();
            dashboard->highlightEquip(skill_name, true);
            highlightSkillButton(skill_name, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            showPromptBox();

            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);

            QStringList targets = ClientInstance->players_to_choose;
            global_targets.clear();
            selected_ids.clear();
            selected_targets_ids.clear();
            foreach (const QString &name, targets) {
                const ClientPlayer *p = ClientInstance->getPlayer(name);
                card_boxes[p]->globalchooseCard(p, skill_name, ClientInstance->skill_name,
                    ClientInstance->handcard_visible, ClientInstance->disabled_ids, ClientInstance->targets_cards.value(name));
                if (!card_boxes[p]->items.isEmpty()) global_targets << p;
            }

            foreach (PlayerCardContainer *item, item2player.keys()) {
                const ClientPlayer *target = item2player.value(item, NULL);
                if (global_targets.contains(target)) {
                    item->setMaxVotes(1);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                }
                if (!global_targets.contains(target)) {
                    QGraphicsItem *animationTarget = item->getMouseClickReceiver();
                    animations->sendBack(animationTarget);
                    QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
                    if (animationTarget2)
                        animations->sendBack(animationTarget2);
                }
            }
            if (global_targets.length() == 1) {
                const ClientPlayer *target = global_targets.first();
                Photo *photo = name2photo.value(target->objectName(), NULL);
                if (photo)
                    photo->setSelected(true);
                else
                    dashboard->setSelected(true);
                updateGlobalCardBox(target);
            }

            break;
        }
        case Client::AskForAG: {
            dashboard->disableAllCards();

            ok_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(false);

            m_cardContainer->startChoose();

            break;
        }
        case Client::AskForYiji: {
            ok_button->setEnabled(false);
            cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            discard_button->setEnabled(false);

            QStringList yiji_info = Sanguosha->currentRoomState()->getCurrentCardUsePattern().split("=");
            yiji_skill->initialize(yiji_info.at(1), yiji_info.first().toInt(), yiji_info.at(2).split("+"), yiji_info.length() == 4 ? yiji_info.at(3) : "");
            dashboard->startPending(yiji_skill);

            showPromptBox();

            break;
        }
        case Client::AskForMoveCards:{
            ok_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
            cancel_button->setEnabled(ClientInstance->m_canDiscardEquip);
            discard_button->setEnabled(false);
            highlightSkillButton(ClientInstance->skill_name, CardUseStruct::CARD_USE_REASON_UNKNOWN, QString(), ClientInstance->getSkillToHighLight());
            break;
        }
        case Client::AskForGuanxing:
        case Client::AskForGongxin: {
            ok_button->setEnabled(true);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(false);

            break;
        }
        case Client::AskForGeneralChosen:
        case Client::AskForArrangement:
        case Client::AskForChoice:
        case Client::AskForTriggerOrder:
        case Client::AskForSuit:
        case Client::AskForCardChosen: {
            ok_button->setEnabled(false);
            cancel_button->setEnabled(false);
            discard_button->setEnabled(false);

            break;
        }
    }
    if (newStatus != oldStatus) {
        if (!ClientInstance->_m_race) _cancelAllFocus();

        if (newStatus != Client::Playing && newStatus != Client::NotActive)
            QApplication::alert(QApplication::focusWidget());
    }

    if (ServerInfo.OperationTimeout == 0)
        return;

    // do timeout
    if ((newStatus & Client::ClientStatusFeatureMask) != Client::StatusHasOwnProgressBar
        && newStatus != oldStatus) {
        QApplication::alert(main_window);
        connect(dashboard, &Dashboard::progressBarTimedOut, this, &RoomScene::doTimeout);
        dashboard->showProgressBar(ClientInstance->getCountdown());
    }
}

void RoomScene::cardMovedinCardchooseBox(const bool enable)
{
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
        case Client::AskForMoveCards: {
            ok_button->setEnabled(enable);
            break;
        }
    }
}

void RoomScene::playPindianSuccess(int type, int index)
{
    if (!m_pindianBox->isVisible()) return;
    setEmotion(m_pindianBox->getRequestor(), type == 1 ? "success" : "no-success");
    m_pindianBox->playSuccess(type, index);
}

void RoomScene::onSkillDeactivated()
{
    const ViewAsSkill *current = dashboard->currentSkill();
    if (current) cancel_button->click();
}

void RoomScene::onSkillActivated()
{
    QSanSkillButton *button = qobject_cast<QSanSkillButton *>(sender());
    const ViewAsSkill *skill = NULL;
    if (button)
        skill = button->getViewAsSkill();
    else { //by Xusine
        GuhuoBox *guhuo = qobject_cast<GuhuoBox *>(sender());
        if (guhuo) {
            skill = Sanguosha->getViewAsSkill(guhuo->getSkillName());
        }
    }

    if (skill) {
        if (current_guhuo_box && current_guhuo_box->getSkillName() != skill->objectName())
            current_guhuo_box->clear();
        dashboard->startPending(skill);
        //ok_button->setEnabled(false);
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern.endsWith("!"))
            cancel_button->setEnabled(false);
        else
            cancel_button->setEnabled(true);

        const Card *card = dashboard->getPendingCard();
        if (card && card->targetFixed() && card->isAvailable(Self)) {
            useSelectedCard();
        } else if (skill->inherits("OneCardViewAsSkill") && !skill->getDialog() && skill->getGuhuoBox() == "" && Config.EnableIntellectualSelection)
            dashboard->selectOnlyCard(ClientInstance->getStatus() == Client::Playing);
    }
}

void RoomScene::onTransferButtonActivated()
{
    TransferButton *button = qobject_cast<TransferButton *>(sender());

    Sanguosha->getTransfer()->setToSelect(button->getCardId());

    foreach (TransferButton *btn, dashboard->getTransferButtons()) {
        if (btn != button && btn->isDown())
            btn->setState(QSanButton::S_STATE_UP);
    }

    dashboard->startPending(Sanguosha->getTransfer());
    dashboard->unselectAll();
    dashboard->addPending(button->getCardItem());
    dashboard->selectCard(button->getCardItem(), true);
    dashboard->updatePending();
    dashboard->adjustCards();
    cancel_button->setEnabled(true);
}

void RoomScene::doOkButton()
{
    if (!ok_button->isEnabled()) return;
    if (m_cardContainer->retained()) m_cardContainer->clear();
    useSelectedCard();
}

void RoomScene::resetButton()
{
    ok_button->setEnabled(false);
}

void RoomScene::doCancelButton()
{
    if (m_cardContainer->retained()) m_cardContainer->clear();
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
        case Client::Playing: {
            dashboard->skillButtonDeactivated();
            const ViewAsSkill *skill = dashboard->currentSkill();
            dashboard->unselectAll();
            if (skill)
                cancelViewAsSkill();
            else
                dashboard->stopPending();
            dashboard->enableCards();
            break;
        }
        case Client::Responding: {
            dashboard->skillButtonDeactivated();
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern.isEmpty()) return;

            dashboard->unselectAll();

            if (!pattern.startsWith("@")) {
                const ViewAsSkill *skill = dashboard->currentSkill();
                if (!skill->inherits("ResponseSkill")) {
                    cancelViewAsSkill();
                    break;
                }
            }

            ClientInstance->onPlayerResponseCard(NULL);
            prompt_box->disappear();
            dashboard->stopPending();
            break;
        }
        case Client::AskForShowOrPindian: {
            dashboard->unselectAll();
            ClientInstance->onPlayerResponseCard(NULL);
            prompt_box->disappear();
            dashboard->stopPending();
            break;
        }
        case Client::Discarding: {
            dashboard->unselectAll();
            dashboard->stopPending();
            ClientInstance->onPlayerDiscardCards(NULL);
            prompt_box->disappear();
            break;
        }
        case Client::Exchanging: {
            dashboard->unselectAll();
            dashboard->stopPending();
            ClientInstance->onPlayerDiscardCards(NULL);
            prompt_box->disappear();
            break;
        }
        case Client::ExecDialog: {
            m_choiceDialog->reject();
            break;
        }
        case Client::AskForMoveCards: {
            QList<int> empty;
            dashboard->highlightEquip(ClientInstance->skill_name, false);
            m_cardchooseBox->clear();
            ClientInstance->onPlayerReplyMoveCards(empty, empty);
            break;
        }
        case Client::AskForSkillInvoke: {
            QString skill_name = ClientInstance->getSkillNameToInvoke();
            dashboard->highlightEquip(skill_name, false);
            ClientInstance->onPlayerInvokeSkill(false);
            prompt_box->disappear();
            break;
        }
        case Client::AskForYiji: {
            dashboard->stopPending();
            ClientInstance->onPlayerReplyYiji(NULL, NULL);
            prompt_box->disappear();
            break;
        }
        case Client::AskForPlayerChoose: {
            dashboard->highlightEquip(ClientInstance->skill_name, false);
            dashboard->stopPending();
            QList<const Player *> null;
            ClientInstance->onPlayerChoosePlayer(null);
            prompt_box->disappear();
            break;
        }
        case Client::GlobalCardChosen: {
            enableTargets(NULL);
            foreach (const ClientPlayer *p, card_boxes.keys())
                card_boxes[p]->clear();
            dashboard->highlightEquip(ClientInstance->skill_name, false);
            ClientInstance->onPlayerChooseCards(QList<int>());
            prompt_box->disappear();
            break;
        }
        default:
            break;
    }
}

void RoomScene::doDiscardButton()
{
    dashboard->stopPending();
    dashboard->unselectAll();

    if (m_cardContainer->retained()) m_cardContainer->clear();
    if (ClientInstance->getStatus() == Client::Playing)
        ClientInstance->onPlayerResponseCard(NULL);
}

void RoomScene::hideAvatars()
{
    if (control_panel) control_panel->hide();
}

void RoomScene::startInXs()
{
    if (add_robot) add_robot->hide();
    if (fill_robots) fill_robots->hide();
    if (return_to_start_scene) return_to_start_scene->hide();
}

void RoomScene::stopHeroSkinChangingAnimations()
{
    foreach (Photo *const &photo, photos) {
        photo->stopHeroSkinChangingAnimation();
    }

    dashboard->stopHeroSkinChangingAnimation();
}

void RoomScene::addHeroSkinContainer(HeroSkinContainer *heroSkinContainer)
{
    m_heroSkinContainers.insert(heroSkinContainer);
}

HeroSkinContainer *RoomScene::findHeroSkinContainer(const QString &generalName) const
{
    foreach (HeroSkinContainer *heroSkinContainer, m_heroSkinContainers) {
        if (heroSkinContainer->getGeneralName() == generalName)
            return heroSkinContainer;
    }

    return NULL;
}

Dashboard *RoomScene::getDasboard() const
{
    return dashboard;
}

void RoomScene::changeHp(const QString &who, int delta, DamageStruct::Nature nature, bool losthp)
{
    // update
    Photo *photo = name2photo.value(who, NULL);
    if (photo)
        photo->updateHp();
    else
        dashboard->update();

    if (delta < 0) {
        if (losthp) {
            Sanguosha->playSystemAudioEffect("hplost");
            QString from_general = ClientInstance->getPlayer(who)->objectName();
            log_box->appendLog("#GetHp", from_general, QStringList(), QString(),
                QString::number(ClientInstance->getPlayer(who)->getHp()), QString::number(ClientInstance->getPlayer(who)->getMaxHp()));
            return;
        }

        QString damage_effect;
        QString from_general = ClientInstance->getPlayer(who)->objectName();
        log_box->appendLog("#GetHp", from_general, QStringList(), QString(),
            QString::number(ClientInstance->getPlayer(who)->getHp()), QString::number(ClientInstance->getPlayer(who)->getMaxHp()));
        switch (delta) {
            case -1: damage_effect = "injure1"; break;
            case -2: damage_effect = "injure2"; break;
            case -3:
            default: damage_effect = "injure3"; break;
        }

        Sanguosha->playSystemAudioEffect(damage_effect);

        if (photo) {
            setEmotion(who, "damage");
            photo->tremble();
        }

        if (nature == DamageStruct::Fire)
            doAnimation(S_ANIMATE_FIRE, QStringList() << who);
        else if (nature == DamageStruct::Thunder)
            doAnimation(S_ANIMATE_LIGHTNING, QStringList() << who);
    } else {
        QString type = "#Recover";
        QString from_general = ClientInstance->getPlayer(who)->objectName();
        QString n = QString::number(delta);

        log_box->appendLog(type, from_general, QStringList(), QString(), n);
        log_box->appendLog("#GetHp", from_general, QStringList(), QString(),
            QString::number(ClientInstance->getPlayer(who)->getHp()), QString::number(ClientInstance->getPlayer(who)->getMaxHp()));
    }
}

void RoomScene::changeMaxHp(const QString &, int delta)
{
    if (delta < 0)
        Sanguosha->playSystemAudioEffect("maxhplost");
}

void RoomScene::onStandoff()
{
    return_to_start_scene->show();

    log_box->append(QString(tr("<font color='%1'>---------- Game Finish ----------</font>").arg(Config.TextEditColor.name())));

    freeze();
    Sanguosha->playSystemAudioEffect("standoff");

    FlatDialog *dialog = new FlatDialog(main_window);
    dialog->resize(500, 600);
    dialog->setWindowTitle(tr("Standoff"));

    QVBoxLayout *layout = new QVBoxLayout;

    QTableWidget *table = new QTableWidget;
    fillTable(table, ClientInstance->getPlayers());

    layout->addWidget(table);
    dialog->mainLayout()->addLayout(layout);

    addRestartButton(dialog);

    dialog->exec();
}

void RoomScene::onGameOver()
{
    return_to_start_scene->show();

    log_box->append(QString(tr("<font color='%1'>---------- Game Finish ----------</font>").arg(Config.TextEditColor.name())));

    m_roomMutex.lock();
    freeze();

    bool victory = Self->property("win").toBool();
#ifdef AUDIO_SUPPORT
    QString win_effect;
    if (victory) {
        win_effect = "win";
        foreach (const Player *player, ClientInstance->getPlayers()) {
            if (player->property("win").toBool() && player->getGeneralName().contains("caocao")) {
                Audio::stop();
                win_effect = "win-cc";
                break;
            }
        }
    } else {
        win_effect = "lose";
    }

    Sanguosha->playSystemAudioEffect(win_effect);
#endif
    FlatDialog *dialog = new FlatDialog(main_window);
    dialog->resize(800, 600);
    dialog->setWindowTitle(victory ? tr("Victory") : tr("Failure"));

    QGroupBox *winner_box = new QGroupBox(tr("Winner(s)"));
    QGroupBox *loser_box = new QGroupBox(tr("Loser(s)"));

    QString style = StyleHelper::styleSheetOfScrollBar();

    QTableWidget *winner_table = new QTableWidget;
    winner_table->verticalScrollBar()->setStyleSheet(style);
    winner_table->horizontalScrollBar()->setStyleSheet(style);
    QTableWidget *loser_table = new QTableWidget;
    loser_table->verticalScrollBar()->setStyleSheet(style);
    loser_table->horizontalScrollBar()->setStyleSheet(style);

    QVBoxLayout *winner_layout = new QVBoxLayout;
    winner_layout->addWidget(winner_table);
    winner_box->setLayout(winner_layout);

    QVBoxLayout *loser_layout = new QVBoxLayout;
    loser_layout->addWidget(loser_table);
    loser_box->setLayout(loser_layout);

    QVBoxLayout *layout = dialog->mainLayout();
    layout->addWidget(winner_box);
    layout->addWidget(loser_box);

    QList<const ClientPlayer *> winner_list, loser_list;
    foreach (const ClientPlayer *player, ClientInstance->getPlayers()) {
        bool win = player->property("win").toBool();
        if (win)
            winner_list << player;
        else
            loser_list << player;
    }

    fillTable(winner_table, winner_list);
    fillTable(loser_table, loser_list);

    if (!ClientInstance->getReplayer() && Config.value("EnableAutoSaveRecord", false).toBool())
        saveReplayRecord(true, Config.value("NetworkOnly", false).toBool());

    addRestartButton(dialog);
    m_roomMutex.unlock();
    dialog->exec();
}

void RoomScene::addRestartButton(QDialog *dialog)
{
#ifdef Q_OS_ANDROID
    dialog->resize(sceneRect().width(), sceneRect().height());
    dialog->setStyleSheet("background-color: #F0FFF0; color: black;");
#else
    dialog->resize(main_window->width() / 2, dialog->height());
#endif

    QPushButton *restartButton = new QPushButton(tr("Restart Game"));
    QPushButton *returnButton = new QPushButton(tr("Return to main menu"));
    QPushButton *saveButton = new QPushButton(tr("Save record"));
    QPushButton *closeButton = new QPushButton(tr("Close"));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch();
    hLayout->addWidget(restartButton);

    hLayout->addWidget(saveButton);
    hLayout->addWidget(returnButton);
    hLayout->addWidget(closeButton);

    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(dialog->layout());
    if (layout) layout->addLayout(hLayout);

    connect(restartButton, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(returnButton, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(saveButton, &QPushButton::clicked, this, (void (RoomScene::*)())(&RoomScene::saveReplayRecord));
    connect(dialog, &QDialog::accepted, this, &RoomScene::restart);
    connect(returnButton, &QPushButton::clicked, this, &RoomScene::return_to_start);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::reject);
}

void RoomScene::saveReplayRecord()
{
    saveReplayRecord(false);
}

void RoomScene::saveReplayRecord(const bool auto_save, const bool network_only)
{
    if (auto_save) {
        bool is_network = false;
        foreach (const ClientPlayer *player, ClientInstance->getPlayers()) {
            if (player == Self) continue;
            if (player->getState() != "robot") {
                is_network = true;
                break;
            }
        }
        if (network_only && !is_network) return;
        QString location = Config.value("RecordSavePaths", "records/").toString();
        if (!location.startsWith(":")) {
            location.replace("\\", "/");
            if (!location.endsWith("/"))
                location.append("/");
            if (!QDir(location).exists())
                QDir().mkdir(location);
            location.append(QString("%1%2-").arg(Sanguosha->translate(Self->getActualGeneral1()->objectName()))
                .arg(Sanguosha->translate(Self->getActualGeneral2()->objectName())));
            location.append(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
            location.append(".qsgs");
            ClientInstance->save(location);
        }
        return;
    }

    QString location = Config.value("LastReplayDir").toString();
    if (location.isEmpty())
        location = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QString filename = QFileDialog::getSaveFileName(main_window,
        tr("Save replay record"),
        location,
        tr("QSanguosha Replay File(*.qsgs)"));

    if (!filename.isEmpty()) {
        ClientInstance->save(filename);

        QFileInfo file_info(filename);
        QString last_dir = file_info.absoluteDir().path();
        Config.setValue("LastReplayDir", last_dir);
    }
}

ScriptExecutor::ScriptExecutor(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Script execution"));
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel(tr("Please input the script that should be executed at server side:\n P = you, R = your room")));

    QTextEdit *box = new QTextEdit;
    box->setObjectName("scriptBox");
    vlayout->addWidget(box);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    hlayout->addWidget(ok_button);

    vlayout->addLayout(hlayout);

    connect(ok_button, &QPushButton::clicked, this, &ScriptExecutor::accept);
    connect(this, &ScriptExecutor::accepted, this, &ScriptExecutor::doScript);

    setLayout(vlayout);
}

void ScriptExecutor::doScript()
{
    QTextEdit *box = findChild<QTextEdit *>("scriptBox");
    if (box == NULL) return;

    QString script = box->toPlainText();
    QByteArray data = script.toLatin1();
    data = qCompress(data);
    script = data.toBase64();

    ClientInstance->requestCheatRunScript(script);
}

DeathNoteDialog::DeathNoteDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Death note"));

    killer = new QComboBox;
    RoomScene::FillPlayerNames(killer, true);

    victim = new QComboBox;
    RoomScene::FillPlayerNames(victim, false);

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, &QPushButton::clicked, this, &DeathNoteDialog::accept);

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Killer"), killer);
    layout->addRow(tr("Victim"), victim);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);
    layout->addRow(hlayout);

    setLayout(layout);
}

void DeathNoteDialog::accept()
{
    QDialog::accept();
    ClientInstance->requestCheatKill(killer->itemData(killer->currentIndex()).toString(),
        victim->itemData(victim->currentIndex()).toString());
}

DamageMakerDialog::DamageMakerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Damage maker"));

    damage_source = new QComboBox;
    RoomScene::FillPlayerNames(damage_source, true);

    damage_target = new QComboBox;
    RoomScene::FillPlayerNames(damage_target, false);

    damage_nature = new QComboBox;
    damage_nature->addItem(tr("Normal"), S_CHEAT_NORMAL_DAMAGE);
    damage_nature->addItem(tr("Thunder"), S_CHEAT_THUNDER_DAMAGE);
    damage_nature->addItem(tr("Fire"), S_CHEAT_FIRE_DAMAGE);
    damage_nature->addItem(tr("Recover HP"), S_CHEAT_HP_RECOVER);
    damage_nature->addItem(tr("Lose HP"), S_CHEAT_HP_LOSE);
    damage_nature->addItem(tr("Lose Max HP"), S_CHEAT_MAX_HP_LOSE);
    damage_nature->addItem(tr("Reset Max HP"), S_CHEAT_MAX_HP_RESET);

    damage_point = new QSpinBox;
    damage_point->setRange(1, INT_MAX);
    damage_point->setValue(1);

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, &QPushButton::clicked, this, &DamageMakerDialog::accept);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);

    QFormLayout *layout = new QFormLayout;

    layout->addRow(tr("Damage source"), damage_source);
    layout->addRow(tr("Damage target"), damage_target);
    layout->addRow(tr("Damage nature"), damage_nature);
    layout->addRow(tr("Damage point"), damage_point);
    layout->addRow(hlayout);

    setLayout(layout);

    connect(damage_nature, (void (QComboBox::*)(const QString &))(&QComboBox::currentIndexChanged), this, &DamageMakerDialog::disableSource);
}

void DamageMakerDialog::disableSource(const QString &currentNature)
{
    damage_source->setEnabled(currentNature.startsWith('L'));
}

void RoomScene::FillPlayerNames(QComboBox *ComboBox, bool add_none)
{
    if (add_none) ComboBox->addItem(tr("None"), ".");
    ComboBox->setIconSize(G_COMMON_LAYOUT.m_tinyAvatarSize);
    foreach (const ClientPlayer *player, ClientInstance->getPlayers()) {
        QString general_name = Sanguosha->translate(player->getGeneralName());
        if (!player->getGeneral()) continue;
        QPixmap pixmap = G_ROOM_SKIN.getGeneralPixmap(player->getGeneralName(),
            QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY,
            player->getHeadSkinId());
        ComboBox->addItem(QIcon(pixmap),
            QString("%1 [%2]").arg(general_name).arg(player->screenName()),
            player->objectName());
    }
}

void DamageMakerDialog::accept()
{
    QDialog::accept();

    ClientInstance->requestCheatDamage(damage_source->itemData(damage_source->currentIndex()).toString(),
        damage_target->itemData(damage_target->currentIndex()).toString(),
        (DamageStruct::Nature)damage_nature->itemData(damage_nature->currentIndex()).toInt(),
        damage_point->value());
}

void RoomScene::makeDamage()
{
    if (Self->getPhase() != Player::Play) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    DamageMakerDialog *damage_maker = new DamageMakerDialog(main_window);
    damage_maker->exec();
}

void RoomScene::makeKilling()
{
    if (Self->getPhase() != Player::Play) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    DeathNoteDialog *dialog = new DeathNoteDialog(main_window);
    dialog->exec();
}

void RoomScene::makeReviving()
{
    if (Self->getPhase() != Player::Play) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    QStringList items;
    QList<const ClientPlayer *> victims;
    foreach (const ClientPlayer *player, ClientInstance->getPlayers()) {
        if (player->isDead()) {
            QString general_name = Sanguosha->translate(player->getGeneralName());
            items << QString("%1 [%2]").arg(player->screenName()).arg(general_name);
            victims << player;
        }
    }

    if (items.isEmpty()) {
        QMessageBox::warning(main_window, tr("Warning"), tr("No victims now!"));
        return;
    }

    bool ok;
    QString item = QInputDialog::getItem(main_window, tr("Reviving wand"),
        tr("Please select a player to revive"), items, 0, false, &ok);
    if (ok) {
        int index = items.indexOf(item);
        ClientInstance->requestCheatRevive(victims.at(index)->objectName());
    }
}

void RoomScene::doScript()
{
    ScriptExecutor *dialog = new ScriptExecutor(main_window);
    dialog->exec();
}

void RoomScene::viewGenerals(const QString &reason, const QStringList &names)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Sanguosha->playSystemAudioEffect("pop-up");
    m_chooseGeneralBox->chooseGeneral(names, true, false,
        Sanguosha->translate(reason));
}

void RoomScene::fillTable(QTableWidget *table, const QList<const ClientPlayer *> &players)
{
    table->setColumnCount(11);
    table->setRowCount(players.length());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    RecAnalysis record(ClientInstance->getReplayPath());
    QMap<QString, PlayerRecordStruct *> record_map = record.getRecordMap();

    static QStringList labels;
    if (labels.isEmpty()) {
        labels << tr("General") << tr("Name") << tr("Alive");
        labels << tr("Nationality");

        labels << tr("TurnCount");
        labels << tr("Recover") << tr("Damage") << tr("Damaged") << tr("Kill") << tr("Designation");
        labels << tr("Handcards");

    }
    table->setHorizontalHeaderLabels(labels);
    table->setSelectionBehavior(QTableWidget::SelectRows);

    for (int i = 0; i < players.length(); i++) {
        const ClientPlayer *player = players[i];

        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(ClientInstance->getPlayerName(player->objectName()));
        table->setItem(i, 0, item);

        item = new QTableWidgetItem;
        item->setText(player->screenName());
        table->setItem(i, 1, item);

        item = new QTableWidgetItem;
        if (player->isAlive())
            item->setText(tr("Alive"));
        else
            item->setText(tr("Dead"));
        table->setItem(i, 2, item);

        item = new QTableWidgetItem;

        QString str = player->getRole() == "careerist" ? "careerist" : player->getKingdom();
        QIcon icon(QString("image/system/roles/%1.png").arg(str));
        item->setIcon(icon);
        item->setText(Sanguosha->translate(str));

        if (!player->isAlive())
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        table->setItem(i, 3, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(player->getMark("Global_TurnCount")));
        table->setItem(i, 4, item);

        PlayerRecordStruct *rec = record_map.value(player->objectName());
        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_recover));
        table->setItem(i, 5, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damage));
        table->setItem(i, 6, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_damaged));
        table->setItem(i, 7, item);

        item = new QTableWidgetItem;
        item->setText(QString::number(rec->m_kill));
        table->setItem(i, 8, item);

        item = new QTableWidgetItem;
        item->setText(rec->m_designation.join(", "));
        table->setItem(i, 9, item);

        item = new QTableWidgetItem;
        QString handcards = player->property("last_handcards").toString();
        handcards.replace("<img src='image/system/log/spade.png' height = 12/>", tr("Spade"));
        handcards.replace("<img src='image/system/log/heart.png' height = 12/>", tr("Heart"));
        handcards.replace("<img src='image/system/log/club.png' height = 12/>", tr("Club"));
        handcards.replace("<img src='image/system/log/diamond.png' height = 12/>", tr("Diamond"));
        item->setText(handcards);
        table->setItem(i, 10, item);
    }

    for (int i = 2; i <= 10; i++)
        table->resizeColumnToContents(i);
}

void RoomScene::killPlayer(const QString &who)
{
    const General *general = NULL;
    m_roomMutex.lock();

    ClientPlayer *player = ClientInstance->getPlayer(who);
    if (player) {
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(Player::PlaceHand, player);
        container->stopHuaShen();
    }

    if (who == Self->objectName()) {
        dashboard->killPlayer();
        dashboard->update();
        general = Self->getGeneral();
        item2player.remove(dashboard);
        if (general->objectName().contains("sujiang") && Self->getMark("cunsi") > 0)
            general = Sanguosha->getGeneral("mifuren");
    } else {
        Photo *photo = name2photo[who];
        photo->killPlayer();
        photo->setFrame(Photo::S_FRAME_NO_FRAME);
        photo->update();
        item2player.remove(photo);
        general = photo->getPlayer()->getGeneral();
        if (general->objectName().contains("sujiang") && photo->getPlayer()->getMark("cunsi") > 0)
            general = Sanguosha->getGeneral("mifuren");
    }

    if (Config.EnableLastWord && !Self->hasFlag("marshalling")) {
        ClientPlayer *player = ClientInstance->getPlayer(who);
        general->lastWord(player->getHeadSkinId());
    }

    m_roomMutex.unlock();
}

void RoomScene::revivePlayer(const QString &who)
{
    if (who == Self->objectName()) {
        dashboard->revivePlayer();
        item2player.insert(dashboard, Self);
        updateSkillButtons();
    } else {
        Photo *photo = name2photo[who];
        photo->revivePlayer();
        item2player.insert(photo, photo->getPlayer());
    }
}

void RoomScene::setDashboardShadow(const QString &who)
{
    if (who != Self->objectName()) return;
    dashboard->setDeathColor();
}

void RoomScene::takeAmazingGrace(ClientPlayer *taker, int card_id, bool move_cards)
{
    QList<int> card_ids;
    card_ids.append(card_id);
    m_tablePile->clear();

    m_cardContainer->m_currentPlayer = taker;
    CardItem *copy = m_cardContainer->removeCardItems(card_ids, Player::PlaceHand).first();
    if (copy == NULL) return;
    if (!m_cardContainer->isVisible()) return;

    QList<CardItem *> items;
    items << copy;

    if (taker) {
        GenericCardContainer *container = _getGenericCardContainer(Player::PlaceHand, taker);
        bringToFront(container);
        if (move_cards) {
            QString type = "$TakeAG";
            QString from_general = taker->objectName();
            QString card_str = QString::number(card_id);
            log_box->appendLog(type, from_general, QStringList(), card_str);
            CardsMoveStruct move;
            move.card_ids.append(card_id);
            move.from_place = Player::PlaceWuGu;
            move.to_place = Player::PlaceHand;
            move.to = taker;
            container->addCardItems(items, move);
        } else {
            delete copy;
        }
    } else
        delete copy;
}

void RoomScene::showCard(const QString &player_name, int card_id)
{
    QList<int> card_ids;
    card_ids << card_id;
    ClientPlayer *player = ClientInstance->getPlayer(player_name);

    GenericCardContainer *container = _getGenericCardContainer(Player::PlaceHand, player);
    QList<CardItem *> card_items = container->cloneCardItems(card_ids);
    CardMoveReason reason(CardMoveReason::S_REASON_DEMONSTRATE, player->objectName());
    bringToFront(m_tablePile);
    CardsMoveStruct move;
    move.from_place = Player::PlaceHand;
    move.to_place = Player::PlaceTable;
    move.reason = reason;
    card_items[0]->setFootnote(_translateMovement(move));
    m_tablePile->addCardItems(card_items, move);

    QString card_str = QString::number(card_id);
    log_box->appendLog("$ShowCard", player->objectName(), QStringList(), card_str);
}

void RoomScene::chooseSkillButton()
{
    QList<QSanSkillButton *> enabled_buttons;
    foreach (QSanSkillButton *btn, m_skillButtons) {
        if (btn->isEnabled())
            enabled_buttons << btn;
    }

    if (enabled_buttons.isEmpty()) return;

    QDialog *dialog = new QDialog(main_window);
    dialog->setWindowTitle(tr("Select skill"));

    QVBoxLayout *layout = new QVBoxLayout;

    foreach (QSanSkillButton *btn, enabled_buttons) {
        Q_ASSERT(btn->getSkill());
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(btn->getSkill()->objectName()));
        connect(button, &QCommandLinkButton::clicked, btn, &QSanSkillButton::click);
        connect(button, &QCommandLinkButton::clicked, dialog, &QDialog::accept);
        layout->addWidget(button);
    }

    dialog->setLayout(layout);
    dialog->exec();
}

void RoomScene::attachSkill(const QString &skill_name, const bool &head)
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill)
        addSkillButton(skill, head);
}

void RoomScene::detachSkill(const QString &skill_name, bool head)
{
    // for shuangxiong { Client::updateProperty(const Json::Value &) }
    // this is an EXTREMELY DIRTY HACK!!! for we should prevent shuangxiong skill button been removed temporily by duanchang
    if (Self != NULL && Self->hasFlag("shuangxiong")
        && skill_name == "shuangxiong" && Self->getPhase() <= Player::Discard) {
        Self->setFlags("shuangxiong_postpone");
    } else {
        QSanSkillButton *btn = dashboard->removeSkillButton(skill_name, head);
        if (btn == NULL) return;    //be care LordSkill and SPConvertSkill
        m_skillButtons.removeAll(btn);
        btn->deleteLater();
        if (guhuo_items[skill_name] != NULL && !Self->hasSkill(skill_name))
            guhuo_items[skill_name]->deleteLater(); //added by Xusine for GuhuoBox
    }
}

void RoomScene::viewDistance()
{
    DistanceViewDialog *dialog = new DistanceViewDialog(main_window);
    dialog->show();
}

void RoomScene::speak()
{

    bool broadcast = true;
    QString text = chatEdit->text();
    if (text == ".StartBgMusic") {
        broadcast = false;
        Config.EnableBgMusic = true;
        Config.setValue("EnableBgMusic", true);
#ifdef AUDIO_SUPPORT
        Audio::stopBGM();
        QString bgmusic_path = Config.value("BackgroundMusic", "audio/system/background.ogg").toString();
        Audio::playBGM(bgmusic_path);
#endif
    } else if (text.startsWith(".StartBgMusic=")) {
        broadcast = false;
        Config.EnableBgMusic = true;
        Config.setValue("EnableBgMusic", true);
        QString path = text.mid(14);
        if (path.startsWith("|")) {
            path = path.mid(1);
            Config.setValue("BackgroundMusic", path);
        }
#ifdef AUDIO_SUPPORT
        Audio::stopBGM();
        Audio::playBGM(path);
#endif
    } else if (text == ".StopBgMusic") {
        broadcast = false;
        Config.EnableBgMusic = false;
        Config.setValue("EnableBgMusic", false);
#ifdef AUDIO_SUPPORT
        Audio::stopBGM();
#endif
    }
    if (broadcast) {
        if (game_started && ServerInfo.DisableChat)
            chatBox->append(tr("This room does not allow chatting!"));
        else
            ClientInstance->speakToServer(text);
    } else {
        QString title;
        if (Self) {
            title = Self->getGeneralName();
            title = Sanguosha->translate(title);
            title.append(QString("(%1)").arg(Self->screenName()));
            title = QString("<b>%1</b>").arg(title);
        }
        QString line = tr("<font color='%1'>[%2] said: %3 </font>")
            .arg(Config.TextEditColor.name()).arg(title).arg(text);
        chatBox->append(QString("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
    }
    chatEdit->clear();
}

void RoomScene::fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids)
{
    bringToFront(m_cardContainer);
    m_cardContainer->fillCards(card_ids, disabled_ids);
    m_cardContainer->setPos(m_tableCenterPos - QPointF(m_cardContainer->boundingRect().width() / 2, m_cardContainer->boundingRect().height() / 2));
    m_cardContainer->show();
}

void RoomScene::doGongxin(const QList<int> &card_ids, bool enable_heart, QList<int> enabled_ids)
{
    fillCards(card_ids);
    if (enable_heart)
        m_cardContainer->startGongxin(enabled_ids);
    else if (!m_cardContainer->retained())
        m_cardContainer->addConfirmButton();
}

void RoomScene::showPile(const QList<int> &card_ids, const QString &name, const ClientPlayer *target)
{
    pileContainer->clear();
    bringToFront(pileContainer);
    pileContainer->setObjectName(name);
    if (name == "huashencard" && target->hasSkill("huashen")) {
        QStringList huashens = target->tag["Huashens"].toStringList();
        QList<CardItem *> generals;
        foreach (QString arg, huashens) {
            CardItem *item = new CardItem(arg);
            addItem(item);
            item->setParentItem(pileContainer);
            generals.append(item);
        }
        pileContainer->fillGeneralCards(generals);
    } else {
        pileContainer->fillCards(card_ids);
    }
    pileContainer->setPos(m_tableCenterPos - QPointF(pileContainer->boundingRect().width() / 2, pileContainer->boundingRect().height() / 2));
    pileContainer->show();
}

QString RoomScene::getCurrentShownPileName()
{
    if (pileContainer->isVisible())
        return pileContainer->objectName();
    else
        return NULL;
}

void RoomScene::hidePile()
{
    pileContainer->clear();
}

void RoomScene::showOwnerButtons(bool owner)
{
    if (control_panel && !game_started)
        control_panel->setVisible(owner);
}

void RoomScene::showPlayerCards()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        QStringList names = action->data().toString().split(".");
        QString player_name = names.first();
        const ClientPlayer *player = ClientInstance->getPlayer(player_name);
        if (names.length() > 1) {
            QString pile_name = names.last();
            QList<const Card *> cards;
            foreach (int id, player->getPile(pile_name)) {
                const Card *card = Sanguosha->getEngineCard(id);
                if (card) cards << card;
            }

            CardOverview *overview = new CardOverview;
            overview->setWindowTitle(QString("%1 %2")
                .arg(ClientInstance->getPlayerName(player_name))
                .arg(Sanguosha->translate(pile_name)));
            overview->loadFromList(cards);
            overview->show();
        } else {
            QList<const Card *> cards;
            foreach (const Card *card, player->getHandcards()) {
                const Card *engine_card = Sanguosha->getEngineCard(card->getId());
                if (engine_card) cards << engine_card;
            }

            CardOverview *overview = new CardOverview;
            overview->setWindowTitle(QString("%1 %2")
                .arg(ClientInstance->getPlayerName(player_name))
                .arg(tr("Known cards")));
            overview->loadFromList(cards);
            overview->show();
        }
    }
}

void RoomScene::onGameStart()
{
    if (ClientInstance->getReplayer() != NULL)
        m_chooseGeneralBox->clear();

    main_window->activateWindow();

    if (control_panel)
        control_panel->hide();

    if (!Self->hasFlag("marshalling"))
        log_box->append(QString(tr("<font color='%1'>---------- Game Start ----------</font>").arg(Config.TextEditColor.name())));

#ifdef AUDIO_SUPPORT
    if (Config.EnableBgMusic) {
        // start playing background music
        QString bgmusic_path = Config.value("BackgroundMusic", "audio/system/background.ogg").toString();

        Audio::playBGM(bgmusic_path);
    }
#endif
    game_started = true;
    dashboard->refresh();
    if (!ClientInstance->getReplayer())
        dashboard->showControlButtons();
    dashboard->showSeat();
    foreach(Photo *photo, photos)
        photo->showSeat();
}

void RoomScene::freeze()
{
    stopHeroSkinChangingAnimations();

    dashboard->setEnabled(false);
    dashboard->stopHuaShen();
    foreach (Photo *photo, photos) {
        photo->hideProgressBar();
        photo->stopHuaShen();
        photo->setEnabled(false);
    }
    item2player.clear();
    chatEdit->setEnabled(false);
#ifdef AUDIO_SUPPORT
    Audio::stopBGM();
#endif
    dashboard->hideProgressBar();
    main_window->setStatusBar(NULL);
}

void RoomScene::_cancelAllFocus()
{
    foreach (Photo *photo, photos) {
        photo->hideProgressBar();
        if (photo->getPlayer()->getPhase() == Player::NotActive)
            photo->setFrame(Photo::S_FRAME_NO_FRAME);
    }
}

void RoomScene::moveFocus(const QStringList &players, Countdown countdown)
{
    _cancelAllFocus();
    foreach (const QString &player, players) {
        Photo *photo = name2photo[player];
        if (!photo) {
            Q_ASSERT(player == Self->objectName());
            continue;
        }

        if (ServerInfo.OperationTimeout > 0)
            photo->showProgressBar(countdown);
        else if (photo->getPlayer()->getPhase() == Player::NotActive)
            photo->setFrame(Photo::S_FRAME_RESPONDING);
    }
}

void RoomScene::setEmotion(const QString &who, const QString &emotion, bool playback, int duration)
{
    bool permanent = false;
    if (emotion == "question" || emotion == "no-question")
        permanent = true;
    setEmotion(who, emotion, permanent, playback, duration);
}

void RoomScene::setEmotion(const QString &who, const QString &emotion, bool permanent, bool playback, int duration)
{
    if (emotion.startsWith("weapon/") || emotion.startsWith("armor/")) {
        if (Config.value("NoEquipAnim", false).toBool()) return;
        QString name = emotion.split("/").last();
        Sanguosha->playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(name, QString("equip"), -1));
    }
    Photo *photo = name2photo[who];
    if (photo) {
        photo->setEmotion(emotion, permanent, playback, duration);
    } else {
        QString path = QString("image/system/emotion/%1.png").arg(emotion);
        if (QFile::exists(path)) {
            QSanSelectableItem *item = new QSanSelectableItem(path);
            addItem(item);
            item->setZValue(20002.0);
            item->setPos(dashboard->boundingRect().width() / 2 - item->boundingRect().width(),
                dashboard->scenePos().y() - item->boundingRect().height() / 1.5);
            QPropertyAnimation *appear = new QPropertyAnimation(item, "opacity");
            appear->setEndValue(0.0);
            appear->setDuration(2000);
            appear->start(QAbstractAnimation::DeleteWhenStopped);
            connect(appear, &QPropertyAnimation::finished, item, &QSanSelectableItem::deleteLater);
        } else {
            PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(dashboard, emotion, playback, duration);
            if (pma) {
                pma->moveBy(0, -dashboard->boundingRect().height() / 1.5);
                pma->setZValue(20002.0);
            }
        }
    }
}

void RoomScene::showSkillInvocation(const QString &who, const QString &skill_name)
{
    const ClientPlayer *player = ClientInstance->findChild<const ClientPlayer *>(who);
    if (!player->hasSkill(skill_name) && !player->hasEquipSkill(skill_name)) return;
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill && (skill->inherits("SPConvertSkill") || !skill->isVisible())) return;
    QString type = "#InvokeSkill";
    QString from_general = player->objectName();
    QString arg = skill_name;
    log_box->appendLog(type, from_general, QStringList(), QString(), arg);
}

void RoomScene::removeLightBox()
{
    LightboxAnimation *lightbox = qobject_cast<LightboxAnimation *>(sender());
    if (lightbox) {
        removeItem(lightbox);
        lightbox->deleteLater();
    } else {
        PixmapAnimation *pma = qobject_cast<PixmapAnimation *>(sender());
        if (pma) {
            removeItem(pma->parentItem());
        } else {
            QPropertyAnimation *animation = qobject_cast<QPropertyAnimation *>(sender());
            QGraphicsTextItem *line = qobject_cast<QGraphicsTextItem *>(animation->targetObject());
            if (line) {
                removeItem(line->parentItem());
            } else {
                QSanSelectableItem *line = qobject_cast<QSanSelectableItem *>(animation->targetObject());
                removeItem(line->parentItem());
            }
        }
    }
}

QGraphicsObject *RoomScene::getAnimationObject(const QString &name) const
{
    if (name == Self->objectName())
        return dashboard;
    else
        return name2photo.value(name);
}

void RoomScene::doMovingAnimation(const QString &name, const QStringList &args)
{
    QSanSelectableItem *item = new QSanSelectableItem(QString("image/system/animation/%1.png").arg(name));
    item->setZValue(10086.0);
    addItem(item);

    QGraphicsObject *fromItem = getAnimationObject(args.at(0));
    QGraphicsObject *toItem = getAnimationObject(args.at(1));

    QPointF from = fromItem->scenePos();
    QPointF to = toItem->scenePos();
    if (fromItem == dashboard)
        from.setX(fromItem->boundingRect().width() / 2);
    if (toItem == dashboard)
        to.setX(toItem->boundingRect().width() / 2);

    QSequentialAnimationGroup *group = new QSequentialAnimationGroup;

    QPropertyAnimation *move = new QPropertyAnimation(item, "pos");
    move->setStartValue(from);
    move->setEndValue(to);
    move->setDuration(1000);

    QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
    disappear->setEndValue(0.0);
    disappear->setDuration(1000);

    group->addAnimation(move);
    group->addAnimation(disappear);

    group->start(QAbstractAnimation::DeleteWhenStopped);
    connect(group, &QSequentialAnimationGroup::finished, item, &QSanSelectableItem::deleteLater);
}

void RoomScene::doAppearingAnimation(const QString &name, const QStringList &args)
{
    QGraphicsObject *object = getAnimationObject(args.at(0));
    if (object == NULL)
        return;

    QSanSelectableItem *item = new QSanSelectableItem(QString("image/system/animation/%1.png").arg(name));
    addItem(item);
    item->setZValue(20002.0);

    if (object == dashboard) {
        item->setX((dashboard->boundingRect().width() - item->boundingRect().width()) / 2);
        item->setY(dashboard->scenePos().y() - item->boundingRect().height() / 1.5);
    } else {
        item->setPos(object->scenePos());
    }

    QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
    disappear->setEndValue(0.0);
    disappear->setDuration(1000);

    disappear->start(QAbstractAnimation::DeleteWhenStopped);
    connect(disappear, &QPropertyAnimation::finished, item, &QSanSelectableItem::deleteLater);
}

void RoomScene::doLightboxAnimation(const QString &, const QStringList &args)
{
    QString word = args.first();
    bool reset_size = word.startsWith("_mini_");

    QRect rect = main_window->rect();
    QGraphicsRectItem *lightbox = addRect(rect);

    if (!word.startsWith("skill=")) {
        lightbox->setBrush(QColor(32, 32, 32, 204));
        lightbox->setZValue(20001.0);
        word = Sanguosha->translate(word);
    }

    if (word.startsWith("image=")) {
        QSanSelectableItem *line = new QSanSelectableItem(word.mid(6));
        addItem(line);

        QRectF line_rect = line->boundingRect();
        line->setParentItem(lightbox);
        line->setPos(m_tableCenterPos - line_rect.center());

        QPropertyAnimation *appear = new QPropertyAnimation(line, "opacity");
        appear->setStartValue(0.0);
        appear->setKeyValueAt(0.7, 1.0);
        appear->setEndValue(0.0);

        int duration = args.value(1, "2000").toInt();
        appear->setDuration(duration);

        appear->start(QAbstractAnimation::DeleteWhenStopped);

        connect(appear, &QPropertyAnimation::finished, line, &QSanSelectableItem::deleteLater);
        connect(appear, &QPropertyAnimation::finished, this, &RoomScene::removeLightBox);
    } else if (word.startsWith("anim=")) {
        PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(lightbox, word.mid(5));
        if (pma) {
            pma->setZValue(20002.0);
            pma->moveBy(-sceneRect().width() * _m_roomLayout->m_infoPlaneWidthPercentage / 2, 0);
            connect(pma, &PixmapAnimation::finished, this, &RoomScene::removeLightBox);
        }
    }
    else if (word.startsWith("skill=")) {
        const QString hero = word.mid(6);
        const QString skill = args.value(1, QString());
        LightboxAnimation *animation = new LightboxAnimation(hero, skill, rect);
        animation->setZValue(20001.0);
        addItem(animation);
        connect(animation, &LightboxAnimation::finished, this, &RoomScene::removeLightBox);
    }
    else {
        QFont font = Config.BigFont;
        if (reset_size) font.setPixelSize(100);
        QGraphicsTextItem *line = addText(word, font);
        line->setDefaultTextColor(Qt::white);

        QRectF line_rect = line->boundingRect();
        line->setParentItem(lightbox);
        line->setPos(m_tableCenterPos - line_rect.center());

        QPropertyAnimation *appear = new QPropertyAnimation(line, "opacity");
        appear->setStartValue(0.0);
        appear->setKeyValueAt(0.7, 1.0);
        appear->setEndValue(0.0);

        int duration = args.value(1, "2000").toInt();
        appear->setDuration(duration);

        appear->start(QAbstractAnimation::DeleteWhenStopped);

        connect(appear, &QPropertyAnimation::finished, this, &RoomScene::removeLightBox);
    }
}

void RoomScene::doHuashen(const QString &, const QStringList &args)
{
    Q_ASSERT(args.length() >= 2);

    QStringList hargs = args;
    QString name = hargs.first();
    QStringList general_names = hargs.last().split(":");
    ClientPlayer *player = ClientInstance->getPlayer(name);
    QList<CardItem *> generals;
    if (general_names.first().startsWith("-")) {
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(Player::PlaceHand, player);
        container->stopHuaShen();
        player->tag.remove("Huashens");
        return;
    }
    QStringList huashen_list = player->tag["Huashens"].toStringList();
    foreach (QString arg, general_names) {
        huashen_list << arg;
        CardItem *item = new CardItem(arg);
        item->setPos(this->m_tableCenterPos);
        addItem(item);
        generals.append(item);
    }
    player->tag["Huashens"] = huashen_list;
    CardsMoveStruct move;
    move.to = player;
    move.from_place = Player::DrawPile;
    move.to_place = Player::PlaceSpecial;

    GenericCardContainer *container = _getGenericCardContainer(Player::PlaceHand, player);
    container->addCardItems(generals, move);
    player->changePile("huashencard", false, QList<int>());
}

void RoomScene::showIndicator(const QString &from, const QString &to)
{
    if (Config.value("NoIndicator", false).toBool())
        return;

    QGraphicsObject *obj1 = getAnimationObject(from);
    QGraphicsObject *obj2 = getAnimationObject(to);

    if (obj1 == NULL || obj2 == NULL || obj1 == obj2)
        return;

    QPointF start = obj1->sceneBoundingRect().center();
    QPointF finish = obj2->sceneBoundingRect().center();

    IndicatorItem *indicator = new IndicatorItem(start, finish, ClientInstance->getPlayer(from));

    qreal x = qMin(start.x(), finish.x());
    qreal y = qMin(start.y(), finish.y());
    indicator->setPos(x, y);
    indicator->setZValue(30000.0);

    addItem(indicator);
    indicator->doAnimation();
}

void RoomScene::doIndicate(const QString &, const QStringList &args)
{
    showIndicator(args.first(), args.last());
}

void RoomScene::doBattleArray(const QString &, const QStringList &args)
{
    QStringList names = args.last().split("+");
    if (names.contains(Self->objectName())) dashboard->playBattleArrayAnimations();
    foreach (Photo *p, photos) {
        const ClientPlayer *target = p->getPlayer();
        if (names.contains(target->objectName()))
            p->playBattleArrayAnimations();
    }
}

void RoomScene::doAnimation(int name, const QStringList &args)
{
    static QMap<AnimateType, AnimationFunc> map;
    if (map.isEmpty()) {
        map[S_ANIMATE_NULLIFICATION] = &RoomScene::doMovingAnimation;

        map[S_ANIMATE_FIRE] = &RoomScene::doAppearingAnimation;
        map[S_ANIMATE_LIGHTNING] = &RoomScene::doAppearingAnimation;

        map[S_ANIMATE_LIGHTBOX] = &RoomScene::doLightboxAnimation;
        map[S_ANIMATE_INDICATE] = &RoomScene::doIndicate;
        map[S_ANIMATE_HUASHEN] = &RoomScene::doHuashen;
        map[S_ANIMATE_BATTLEARRAY] = &RoomScene::doBattleArray;
    }

    static QMap<AnimateType, QString> anim_name;
    if (anim_name.isEmpty()) {
        anim_name[S_ANIMATE_NULLIFICATION] = "nullification";

        anim_name[S_ANIMATE_FIRE] = "fire";
        anim_name[S_ANIMATE_LIGHTNING] = "lightning";

        anim_name[S_ANIMATE_LIGHTBOX] = "lightbox";
        anim_name[S_ANIMATE_HUASHEN] = "huashen";
        anim_name[S_ANIMATE_INDICATE] = "indicate";
        anim_name[S_ANIMATE_BATTLEARRAY] = "battlearray";
    }

    AnimationFunc func = map.value((AnimateType)name, NULL);
    if (func) (this->*func)(anim_name.value((AnimateType)name, QString()), args);
}

void RoomScene::showServerInformation()
{
    QDialog *dialog = new QDialog(main_window);
    dialog->setWindowTitle(tr("Server information"));

    QHBoxLayout *layout = new QHBoxLayout;
    ServerInfoWidget *widget = new ServerInfoWidget;
    widget->fill(ServerInfo, Config.HostAddress);
    layout->addWidget(widget);
    dialog->setLayout(layout);

    dialog->show();
}

void RoomScene::surrender()
{
    if (Self->getPhase() != Player::Play) {
        QMessageBox::warning(main_window, tr("Warning"), tr("You can only initiate a surrender poll at your play phase!"));
        return;
    }

    QMessageBox::StandardButton button;
    button = QMessageBox::question(main_window, tr("Surrender"), tr("Are you sure to surrender ?"));
    if (button == QMessageBox::Ok || button == QMessageBox::Yes)
        ClientInstance->requestSurrender();
}

void RoomScene::fillGenerals(const QStringList &)
{
    // 3v3 and 1v1 method here?
}

void RoomScene::bringToFront(QGraphicsItem *front_item)
{
    m_zValueMutex.lock();
    if (_m_last_front_item != NULL)
        _m_last_front_item->setZValue(_m_last_front_ZValue);
    _m_last_front_item = front_item;
    _m_last_front_ZValue = front_item->zValue();
    if (m_pindianBox && front_item != m_pindianBox && front_item != prompt_box && m_pindianBox->isVisible()) {
        front_item->setZValue(8);
    } else {
        front_item->setZValue(10000);
    }
    m_zValueMutex.unlock();
}

void RoomScene::takeGeneral(const QString &who, const QString &name, const QString &)
{
    bool self_taken;
    if (who == "warm")
        self_taken = Self->getRole().startsWith("l");
    else
        self_taken = Self->getRole().startsWith("r");
    QList<CardItem *> *to_add = self_taken ? &down_generals : &up_generals;

    CardItem *general_item = NULL;
    foreach (CardItem *item, general_items) {
        if (item->objectName() == name) {
            general_item = item;
            break;
        }
    }

    Q_ASSERT(general_item);
    if (general_item == NULL)
        return;

    general_item->disconnect(this);
    general_items.removeOne(general_item);
    to_add->append(general_item);

    int x, y;
    x = 43 + (to_add->length() - 1) * 86;
    y = self_taken ? 60 + 120 * 3 : 60;
    x = x + G_COMMON_LAYOUT.m_cardNormalWidth / 2;
    y = y + G_COMMON_LAYOUT.m_cardNormalHeight / 2;
    general_item->setHomePos(QPointF(x, y));
    general_item->goBack(true);
}

void RoomScene::recoverGeneral(int index, const QString &name)
{
    QString obj_name = QString("x%1").arg(index);
    foreach (CardItem *item, general_items) {
        if (item->objectName() == obj_name) {
            item->changeGeneral(name);
            break;
        }
    }
}

void RoomScene::trust()
{
    if (Self->getState() != "trust")
        doCancelButton();
    ClientInstance->trust();
}

void RoomScene::startArrange(const QString &)
{
    arrange_items.clear();
    QString mode;
    QList<QPointF> positions;

    QString suffix = (mode == "1v1" && down_generals.length() == 6) ? "2" : QString();
    QString path = QString("image/system/%1/arrange%2.png").arg(mode).arg(suffix);
    selector_box->load(path);
    selector_box->setPos(m_tableCenterPos);

    foreach (CardItem *item, down_generals) {
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        item->setAutoBack(false);
        connect(item, &CardItem::released, this, &RoomScene::toggleArrange);
    }

    QRect rect(0, 0, 80, 120);

    foreach (const QPointF &pos, positions) {
        QGraphicsRectItem *rect_item = new QGraphicsRectItem(rect, selector_box);
        rect_item->setPos(pos);
        rect_item->setPen(Qt::NoPen);
        arrange_rects << rect_item;
    }

    arrange_button = new Button(tr("Complete"), 0.8);
    arrange_button->setParentItem(selector_box);
    arrange_button->setPos(600, 330);
    connect(arrange_button, &Button::clicked, this, &RoomScene::finishArrange);
}

void RoomScene::toggleArrange()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    QGraphicsItem *arrange_rect = NULL;
    int index = -1;
    for (int i = 0; i < 3; i++) {
        QGraphicsItem *rect = arrange_rects.at(i);
        if (item->collidesWithItem(rect)) {
            arrange_rect = rect;
            index = i;
        }
    }

    if (arrange_rect == NULL) {
        if (arrange_items.contains(item)) {
            arrange_items.removeOne(item);
            down_generals << item;
        }
    } else {
        arrange_items.removeOne(item);
        down_generals.removeOne(item);
        arrange_items.insert(index, item);
    }

    int n = qMin(arrange_items.length(), 3);
    for (int i = 0; i < n; i++) {
        QPointF pos = arrange_rects.at(i)->pos();
        CardItem *item = arrange_items.at(i);
        item->setHomePos(pos);
        item->goBack(true);
    }

    while (arrange_items.length() > 3) {
        CardItem *last = arrange_items.takeLast();
        down_generals << last;
    }

    for (int i = 0; i < down_generals.length(); i++) {
        QPointF pos = QPointF(43 + G_COMMON_LAYOUT.m_cardNormalWidth / 2 + i * 86,
            60 + G_COMMON_LAYOUT.m_cardNormalHeight / 2 + 3 * 120);
        CardItem *item = down_generals.at(i);
        item->setHomePos(pos);
        item->goBack(true);
    }
}

void RoomScene::finishArrange()
{
    if (arrange_items.length() != 3) return;

    arrange_button->deleteLater();
    arrange_button = NULL;

    QStringList names;
    foreach(CardItem *item, arrange_items)
        names << item->objectName();

    if (selector_box) {
        selector_box->deleteLater();
        selector_box = NULL;
    }
    arrange_rects.clear();

    ClientInstance->replyToServer(S_COMMAND_ARRANGE_GENERAL, JsonUtils::toJsonArray(names));
    ClientInstance->setStatus(Client::NotActive);
}

void RoomScene::updateRolesBox()
{
    double centerX = m_rolesBox->boundingRect().width() / 2;
    int n = role_items.length();
    for (int i = 0; i < n; i++) {
        QGraphicsPixmapItem *item = role_items[i];
        item->setParentItem(m_rolesBox);
        item->setPos(21 * (i - n / 2) + centerX, 6);
    }
    m_pileCardNumInfoTextBox->setTextWidth(m_rolesBox->boundingRect().width());
    m_pileCardNumInfoTextBox->setPos(0, 35);
}

void RoomScene::appendChatEdit(QString txt)
{
    chatEdit->setText(chatEdit->text() + txt);
    chatEdit->setFocus();
}

void RoomScene::showBubbleChatBox(const QString &who, const QString &line)
{
    if (!bubbleChatBoxes.contains(who)) {
        BubbleChatBox *bubbleChatBox = new BubbleChatBox(getBubbleChatBoxShowArea(who));
        addItem(bubbleChatBox);
        bubbleChatBox->setZValue(INT_MAX);
        bubbleChatBoxes.insert(who, bubbleChatBox);
    }

    bubbleChatBoxes[who]->setText(line);
}

static const QSize BubbleChatBoxShowAreaSize(138, 64);

QRect RoomScene::getBubbleChatBoxShowArea(const QString &who) const
{
    Photo *photo = name2photo.value(who, NULL);
    if (photo) {
        QRectF rect = photo->sceneBoundingRect();
        QPoint center = rect.center().toPoint();
        return QRect(QPoint(center.x(), center.y() - 90), BubbleChatBoxShowAreaSize);
    } else {
        QRectF rect = dashboard->getAvatarAreaSceneBoundingRect();
        return QRect(QPoint(rect.left() + 45, rect.top() - 20), BubbleChatBoxShowAreaSize);
    }
}

void RoomScene::setChatBoxVisible(bool show)
{
    if (!show) {
        chat_box_widget->hide();
        chatEdit->hide();
        chat_widget->hide();
#ifndef Q_OS_ANDROID
        log_box->resize(_m_infoPlane.width(),
            _m_infoPlane.height() * (_m_roomLayout->m_logBoxHeightPercentage
            + _m_roomLayout->m_chatBoxHeightPercentage)
            + _m_roomLayout->m_chatTextBoxHeight);
#endif
    } else {
        chat_box_widget->show();
        chatEdit->show();
        chat_widget->show();
#ifndef Q_OS_ANDROID
        log_box->resize(_m_infoPlane.width(),
            _m_infoPlane.height() * _m_roomLayout->m_logBoxHeightPercentage);
#endif
    }
}

void RoomScene::updateHandcardNum()
{
    dashboard->updateHandcardNum();
    foreach (Photo *p, photos) {
        p->updateHandcardNum();
    }
}

void RoomScene::updateGlobalCardBox(const ClientPlayer *player, int id)
{
    prompt_box->disappear();
    ok_button->setEnabled(false);
    foreach (const ClientPlayer *p, card_boxes.keys())
        card_boxes[p]->reset();

    if (id == -1) {
        foreach (const ClientPlayer *p, card_boxes.keys())
            card_boxes[p]->hide();

        card_boxes[player]->show();
        GraphicsBox::moveToCenter(card_boxes[player]);
        bringToFront(card_boxes[player]);
    } else {
        if (!selected_ids.contains(id)) {
            selected_ids.append(id);
            selected_targets_ids.insert(id, player);
        } else {
            selected_ids.removeAll(id);
            selected_targets_ids.remove(id);
        }
    }

    foreach (PlayerCardContainer *item, item2player.keys()) {
        const ClientPlayer *target = item2player.value(item, NULL);
        if (target != player) item->setSelected(false);
    }

    int type = ClientInstance->type;
    QList<const ClientPlayer *> targets;
    foreach (const int &key, selected_targets_ids.keys())
        foreach (const ClientPlayer *p, selected_targets_ids.values(key))
            if (!targets.contains(p)) targets << p;

    if (type == 0)
        foreach (const ClientPlayer *p, card_boxes.keys())
            if (targets.contains(p)) card_boxes[p]->setfalse();

    if (selected_ids.length() >= ClientInstance->choose_max_num && ClientInstance->choose_max_num > 0) {
        foreach (const ClientPlayer *p, card_boxes.keys())
            card_boxes[p]->setfalse();

        foreach (PlayerCardContainer *item, item2player.keys()) {
            const ClientPlayer *target = item2player.value(item, NULL);
            if (!targets.contains(target) && (item->flags() & QGraphicsItem::ItemIsSelectable)) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                QGraphicsItem *animationTarget = item->getMouseClickReceiver();
                animations->sendBack(animationTarget);
                QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
                if (animationTarget2)
                    animations->sendBack(animationTarget2);
             }
        }
    } else {
        foreach (PlayerCardContainer *item, item2player.keys()) {
            const ClientPlayer *target = item2player.value(item, NULL);
            if (global_targets.contains(target)) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                QGraphicsItem *animationTarget = item->getMouseClickReceiver();
                animations->effectOut(animationTarget);
                QGraphicsItem *animationTarget2 = item->getMouseClickReceiver2();
                if (animationTarget2)
                    animations->effectOut(animationTarget2);
            }
        }
    }
    if (selected_ids.length() > 0 && (selected_ids.length() <= ClientInstance->choose_max_num || ClientInstance->choose_max_num == 0)
            && (selected_ids.length() >= ClientInstance->choose_min_num || ClientInstance->choose_min_num == 0))
        ok_button->setEnabled(true);
}
