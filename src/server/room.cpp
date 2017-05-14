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

#include "room.h"
#include "engine.h"
#include "settings.h"
#include "standard.h"
#include "ai.h"
#include "scenario.h"
#include "gamerule.h"
#include "scenerule.h"
#include "server.h"
#include "structs.h"
#include "miniscenarios.h"
#include "generalselector.h"
#include "json.h"
#include "clientstruct.h"
#include "roomthread.h"

#include <lua.hpp>
#include <QStringList>
#include <QMessageBox>
#include <QHostAddress>
#include <QTimer>
#include <QMetaEnum>
#include <QTimerEvent>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>

#ifndef QT_NO_DEBUG
#include <QApplication>
#endif

#ifdef QSAN_UI_LIBRARY_AVAILABLE
#pragma message WARN("UI elements detected in server side!!!")
#endif

using namespace QSanProtocol;




Room::Room(QObject *parent, const QString &mode)
    : QThread(parent), mode(mode), current(NULL), pile1(Sanguosha->getRandomCards()),
    m_drawPile(&pile1), m_discardPile(&pile2),
    game_started(false), game_finished(false), game_paused(false), L(NULL), thread(NULL),
    _m_semRaceRequest(0), _m_semRoomMutex(1),
    _m_isFirstSurrenderRequest(true),
    _m_raceStarted(false), provided(NULL), has_provided(false),
    m_surrenderRequestReceived(false), _virtual(false), _m_roomState(false)
{
    static int s_global_room_id = 0;
    _m_Id = s_global_room_id++;
    _m_lastMovementId = 0;
    player_count = Sanguosha->getPlayerCount(mode);
    scenario = Sanguosha->getScenario(mode);

    initCallbacks();

    L = CreateLuaState();

    DoLuaScript(L, "lua/sanguosha.lua");
    DoLuaScript(L, QFile::exists("lua/ai/private-smart-ai.lua") ?
        "lua/ai/private-smart-ai.lua" : "lua/ai/smart-ai.lua");

    m_generalSelector = GeneralSelector::getInstance();
}

Room::~Room()
{
    lua_close(L);
    if (thread != NULL)
    {
        thread->terminate();
        //delete thread;
        thread->deleteLater();
    }
}

void Room::initCallbacks()
{
    // init request response pair
    m_requestResponsePair[S_COMMAND_PLAY_CARD] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_NULLIFICATION] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_SHOW_CARD] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_ASK_PEACH] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_PINDIAN] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_EXCHANGE_CARD] = S_COMMAND_DISCARD_CARD;
    m_requestResponsePair[S_COMMAND_CHOOSE_DIRECTION] = S_COMMAND_MULTIPLE_CHOICE;
    m_requestResponsePair[S_COMMAND_LUCK_CARD] = S_COMMAND_INVOKE_SKILL;

    // client request handlers
    interactions[S_COMMAND_SURRENDER] = &Room::processRequestSurrender;
    interactions[S_COMMAND_CHEAT] = &Room::processRequestCheat;
    interactions[S_COMMAND_PRESHOW] = &Room::processRequestPreshow;

    // Client notifications
    callbacks[S_COMMAND_TOGGLE_READY] = &Room::toggleReadyCommand;
    callbacks[S_COMMAND_ADD_ROBOT] = &Room::addRobotCommand;
    callbacks[S_COMMAND_FILL_ROBOTS] = &Room::fillRobotsCommand;
    callbacks[S_COMMAND_SPEAK] = &Room::speakCommand;
    callbacks[S_COMMAND_TRUST] = &Room::trustCommand;
    callbacks[S_COMMAND_PAUSE] = &Room::pauseCommand;
    callbacks[S_COMMAND_NETWORK_DELAY_TEST] = &Room::networkDelayTestCommand;
    callbacks[S_COMMAND_MIRROR_GUANXING_STEP] = &Room::mirrorGuanxingStepCommand;
    callbacks[S_COMMAND_MIRROR_MOVECARDS_STEP] = &Room::mirrorMoveCardsStepCommand;
    callbacks[S_COMMAND_PINDIAN] = &Room::onPindianReply;
    callbacks[S_COMMAND_CHANGE_SKIN] = &Room::changeSkinCommand;

    // Cheat commands
    cheatCommands[".BroadcastRoles"] = &Room::broadcastRoles;
    cheatCommands[".ShowHandCards"] = &Room::showHandCards;
    cheatCommands[".ShowPrivatePile"] = &Room::showPrivatePile;
    cheatCommands[".SetAIDelay"] = &Room::setAIDelay;
    cheatCommands[".SetGameMode"] = &Room::setGameMode;
    cheatCommands[".Pause"] = &Room::pause;
    cheatCommands[".Resume"] = &Room::resume;
}

ServerPlayer *Room::getCurrent() const
{
    return current;
}

void Room::setCurrent(ServerPlayer *current)
{
    this->current = current;
}

int Room::alivePlayerCount() const
{
    return m_alivePlayers.count();
}

bool Room::notifyUpdateCard(ServerPlayer *player, int cardId, const Card *newCard)
{
    JsonArray val;
    Q_ASSERT(newCard);
    QString className = newCard->getClassName();
    val << cardId;
    val << (int)newCard->getSuit();
    val << newCard->getNumber();
    val << className;
    val << newCard->getSkillName();
    val << newCard->objectName();
    val << JsonUtils::toJsonArray(newCard->getFlags());
    doNotify(player, S_COMMAND_UPDATE_CARD, val);
    return true;
}

bool Room::broadcastUpdateCard(const QList<ServerPlayer *> &players, int cardId, const Card *newCard)
{
    foreach (ServerPlayer *player, players)
        notifyUpdateCard(player, cardId, newCard);
    return true;
}

bool Room::notifyResetCard(ServerPlayer *player, int cardId)
{
    doNotify(player, S_COMMAND_UPDATE_CARD, cardId);
    return true;
}

bool Room::broadcastResetCard(const QList<ServerPlayer *> &players, int cardId)
{
    foreach (ServerPlayer *player, players)
        notifyResetCard(player, cardId);
    return true;
}

QList<ServerPlayer *> Room::getPlayers() const
{
    return m_players;
}

QList<ServerPlayer *> Room::getAllPlayers(bool include_dead) const
{
    QList<ServerPlayer *> count_players = m_players;
    if (current == NULL)
        return count_players;

    ServerPlayer *starter = current;
    int index = count_players.indexOf(starter);
    if (index == -1)
        return count_players;

    QList<ServerPlayer *> all_players;
    for (int i = index; i < count_players.length(); i++) {
        if (include_dead || count_players[i]->isAlive())
            all_players << count_players[i];
    }

    for (int i = 0; i < index; i++) {
        if (include_dead || count_players[i]->isAlive())
            all_players << count_players[i];
    }

    if (current->getPhase() == Player::NotActive && all_players.contains(current)) {
        all_players.removeOne(current);
        all_players.append(current);
    }

    return all_players;
}

QList<ServerPlayer *> Room::getOtherPlayers(ServerPlayer *except, bool include_dead) const
{
    QList<ServerPlayer *> other_players = getAllPlayers(include_dead);
    if (except && (except->isAlive() || include_dead))
        other_players.removeOne(except);
    return other_players;
}

QList<ServerPlayer *> Room::getAlivePlayers() const
{
    return m_alivePlayers;
}

void Room::output(const QString &message)
{
    emit room_message(message);
}

void Room::outputEventStack()
{
    QString msg = "End of Event Stack.";
    foreach (EventTriplet triplet, *thread->getEventStack())
        msg.prepend(triplet.toString());
    msg.prepend("Event Stack:\n");
    output(msg);
}

void Room::enterDying(ServerPlayer *player, DamageStruct *reason)
{
    setPlayerFlag(player, "Global_Dying");
    QStringList currentdying = getTag("CurrentDying").toStringList();
    currentdying << player->objectName();
    setTag("CurrentDying", QVariant::fromValue(currentdying));


    JsonArray arg;
    arg << (int)QSanProtocol::S_GAME_EVENT_PLAYER_DYING;
    arg << player->objectName();
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

    DyingStruct dying;
    dying.who = player;
    dying.damage = reason;

    QVariant dying_data = QVariant::fromValue(dying);
    foreach (ServerPlayer *p, getAllPlayers()) {
        if (thread->trigger(Dying, this, p, dying_data) || player->getHp() > 0 || player->isDead())
            break;
    }

    if (player->isAlive()) {
        if (player->getHp() > 0) {
            setPlayerFlag(player, "-Global_Dying");
        } else {
            LogMessage log;
            log.type = "#AskForPeaches";
            log.from = player;
            log.to = getAllPlayers();
            log.arg = QString::number(1 - player->getHp());
            sendLog(log);

            foreach (ServerPlayer *saver, getAllPlayers()) {
                if (player->getHp() > 0 || player->isDead())
                    break;

                QString cd = saver->property("currentdying").toString();
                setPlayerProperty(saver, "currentdying", player->objectName());
                thread->trigger(AskForPeaches, this, saver, dying_data);
                setPlayerProperty(saver, "currentdying", cd);
            }
            thread->trigger(AskForPeachesDone, this, player, dying_data);

            setPlayerFlag(player, "-Global_Dying");
        }
    }
    currentdying = getTag("CurrentDying").toStringList();
    currentdying.removeOne(player->objectName());
    setTag("CurrentDying", QVariant::fromValue(currentdying));

    if (player->isAlive()) {
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING;
        arg << player->objectName();
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
    }
    thread->trigger(QuitDying, this, player, dying_data);
}

ServerPlayer *Room::getCurrentDyingPlayer() const
{
    QStringList currentdying = getTag("CurrentDying").toStringList();
    if (currentdying.isEmpty()) return NULL;
    QString dyingobj = currentdying.last();
    ServerPlayer *who = findPlayer(dyingobj);
    return who;
}

void Room::revivePlayer(ServerPlayer *player)
{
    player->setAlive(true);
    player->throwAllMarks(false);
    broadcastProperty(player, "alive");
    //setEmotion(player, "revive");

    m_alivePlayers.clear();
    foreach (ServerPlayer *player, m_players) {
        if (player->isAlive())
            m_alivePlayers << player;
    }

    for (int i = 0; i < m_alivePlayers.length(); i++) {
        m_alivePlayers.at(i)->setSeat(i + 1);
        broadcastProperty(m_alivePlayers.at(i), "seat");
    }

    doBroadcastNotify(S_COMMAND_REVIVE_PLAYER, player->objectName());
    updateStateItem();
}

static bool CompareByRole(ServerPlayer *player1, ServerPlayer *player2)
{
    int role1 = player1->getRoleEnum();
    int role2 = player2->getRoleEnum();

    if (role1 != role2)
        return role1 < role2;
    else
        return player1->isAlive();
}

void Room::updateStateItem()
{
    QList<ServerPlayer *> players = this->m_players;
    qSort(players.begin(), players.end(), CompareByRole);
    QString roles;
    foreach (ServerPlayer *p, players) {
        QChar c = "ZCFN"[p->getRoleEnum()];
        if (p->isDead())
            c = c.toLower();

        roles.append(c);
    }

    doBroadcastNotify(S_COMMAND_UPDATE_STATE_ITEM, roles);
}

void Room::killPlayer(ServerPlayer *victim, DamageStruct *reason)
{
    ServerPlayer *killer = reason ? reason->from : NULL;
    QList<ServerPlayer *> players_with_victim = getAllPlayers();

    victim->setAlive(false);

    int index = m_alivePlayers.indexOf(victim);
    for (int i = index + 1; i < m_alivePlayers.length(); i++) {
        ServerPlayer *p = m_alivePlayers.at(i);
        p->setSeat(p->getSeat() - 1);
        broadcastProperty(p, "seat");
    }

    m_alivePlayers.removeOne(victim);

    DeathStruct death;
    death.who = victim;
    death.damage = reason;
    QVariant data = QVariant::fromValue(death);
    thread->trigger(BeforeGameOverJudge, this, victim, data);

    updateStateItem();

    LogMessage log;
    log.type = killer ? (killer == victim ? "#Suicide" : "#Murder") : "#Contingency";
    log.to << victim;
    log.arg = victim->getKingdom();
    log.from = killer;
    sendLog(log);

    broadcastProperty(victim, "alive");
    broadcastProperty(victim, "role");

    doBroadcastNotify(S_COMMAND_KILL_PLAYER, victim->objectName());

    thread->trigger(GameOverJudge, this, victim, data);

    foreach (ServerPlayer *p, players_with_victim) {
        if (p->isAlive() || p == victim)
            thread->trigger(Death, this, p, data);
    }

    doNotify(victim, S_COMMAND_SET_DASHBOARD_SHADOW, victim->objectName());

    victim->detachAllSkills();
    thread->trigger(BuryVictim, this, victim, data);

    if (!victim->isAlive()) {
        bool expose_roles = true;
        foreach (ServerPlayer *player, m_alivePlayers) {
            if (!player->isOffline()) {
                expose_roles = false;
                break;
            }
        }

        if (expose_roles) {
            foreach (ServerPlayer *player, m_alivePlayers) {
                QString role = player->getKingdom();
                if (role == "god") {
                    role = Sanguosha->getGeneral(getTag(player->objectName()).toStringList().at(0))->getKingdom();
                    role = HegemonyMode::GetMappedRole(role);
                    broadcastProperty(player, "role", role);
                }
            }

            if (Config.AlterAIDelayAD)
                Config.AIDelay = Config.AIDelayAD;
            if (victim->isOnline() && Config.SurrenderAtDeath && askForSkillInvoke(victim, "surrender", "yes"))
                makeSurrender(victim);
        }
    }
}

void Room::judge(JudgeStruct &judge_struct)
{
    Q_ASSERT(judge_struct.who != NULL);

    JudgeStruct *judge_star = &judge_struct;
    QVariant data = QVariant::fromValue(judge_star);

    setTag("judge", getTag("judge").toInt() + 1);

    thread->trigger(StartJudge, this, judge_star->who, data);

    QList<ServerPlayer *> players = getAllPlayers();
    foreach (ServerPlayer *player, players) {
        if (thread->trigger(AskForRetrial, this, player, data))
            break;
    }

    thread->trigger(FinishRetrial, this, judge_star->who, data);

    setTag("judge", getTag("judge").toInt() - 1);

    thread->trigger(FinishJudge, this, judge_star->who, data);
}

void Room::sendJudgeResult(const JudgeStruct *judge)
{
    JsonArray arg;
    arg << (int)QSanProtocol::S_GAME_EVENT_JUDGE_RESULT;
    arg << judge->card->getEffectiveId();
    arg << judge->isEffected();
    arg << judge->who->objectName();
    arg << judge->reason;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
}

QList<int> Room::getNCards(int n, bool update_pile_number)
{
    QList<int> card_ids;
    for (int i = 0; i < n; i++)
        card_ids << drawCard();

    if (update_pile_number)
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, m_drawPile->length());

    return card_ids;
}

QStringList Room::aliveRoles(ServerPlayer *except) const
{
    QStringList roles;
    foreach (ServerPlayer *player, m_alivePlayers) {
        if (player != except)
            roles << player->getRole();
    }

    return roles;
}

void Room::gameOver(const QString &winner)
{
    QStringList all_roles;
    foreach (ServerPlayer *player, m_players) {
        all_roles << player->getRole();
        if (player->getHandcardNum() > 0) {
            QStringList handcards;
            foreach (const Card *card, player->getHandcards())
                handcards << Sanguosha->getEngineCard(card->getId())->getLogName();
            QString handcard = handcards.join(", ");
            setPlayerProperty(player, "last_handcards", handcard);
        }
    }

    game_finished = true;

    emit game_over(winner);

    if (mode.contains("_mini_")) {
        ServerPlayer *playerWinner = NULL;
        QStringList winners = winner.split("+");
        foreach (ServerPlayer *sp, m_players) {
            if (sp->getState() != "robot"
                && (winners.contains(sp->getRole())
                || winners.contains(sp->objectName()))) {
                playerWinner = sp;
                break;
            }
        }

        if (playerWinner) {
            QString id = Config.GameMode;
            id.replace("_mini_", "");
            int stage = Config.value("MiniSceneStage", 1).toInt();
            int current = id.toInt();
            if (current < Sanguosha->getMiniSceneCounts()) {
                if (current + 1 > stage) Config.setValue("MiniSceneStage", current + 1);
                QString mode = QString(MiniScene::S_KEY_MINISCENE).arg(QString::number(current + 1));
                Config.setValue("GameMode", mode);
                Config.GameMode = mode;
            }
        }
    }
    Config.AIDelay = Config.OriginAIDelay;

    if (!getTag("NextGameMode").toString().isNull()) {
        QString name = getTag("NextGameMode").toString();
        Config.GameMode = name;
        Config.setValue("GameMode", name);
        removeTag("NextGameMode");
    }

    JsonArray arg;
    arg << winner;
    arg << JsonUtils::toJsonArray(all_roles);
    doBroadcastNotify(S_COMMAND_GAME_OVER, arg);
    throw GameFinished;
}

void Room::slashEffect(const SlashEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    if (thread->trigger(SlashEffected, this, effect.to, data)) {
        if (!effect.to->hasFlag("Global_NonSkillNullify"))
            ;//setEmotion(effect.to, "skill_nullify");
        else
            effect.to->setFlags("-Global_NonSkillNullify");
        if (effect.slash)
            effect.to->removeQinggangTag(effect.slash);
    }
}

void Room::slashResult(const SlashEffectStruct &effect, const Card *jink)
{
    SlashEffectStruct result_effect = effect;
    result_effect.jink = jink;
    QVariant data = QVariant::fromValue(result_effect);

    if (jink == NULL) {
        if (effect.to->isAlive())
            thread->trigger(SlashHit, this, effect.from, data);
    } else {
        if (effect.to->isAlive()) {
            if (jink->getSkillName() != "EightDiagram")
                setEmotion(effect.to, "jink");
        }
        if (effect.slash)
            effect.to->removeQinggangTag(effect.slash);
        thread->trigger(SlashMissed, this, effect.from, data);
    }
}

void Room::attachSkillToPlayer(ServerPlayer *player, const QString &skill_name)
{
    player->acquireSkill(skill_name);
    doNotify(player, S_COMMAND_ATTACH_SKILL, skill_name);
}

void Room::detachSkillFromPlayer(ServerPlayer *player, const QString &skill_name, bool is_equip, bool acquire_only, bool head)
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (head && !player->getHeadSkillList(false, true, is_equip).contains(skill)) return;
    if (!head && !player->getDeputySkillList(false, true, is_equip).contains(skill)) return;

    if (player->getAcquiredSkills(head ? "head" : "deputy").contains(skill_name)) {
        player->detachSkill(skill_name, head);
    } else if (!acquire_only)
        player->loseSkill(skill_name, head);
    else
        return;

    if (skill && skill->isVisible()) {
        JsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_DETACH_SKILL;
        args << player->objectName();
        args << skill_name;
        args << head;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        if (!is_equip) {
            LogMessage log;
            log.type = "#LoseSkill";
            log.from = player;
            log.arg = skill_name;
            sendLog(log);

            QVariant data = skill_name + ":" + (head ? "head" : "deputy");
            thread->trigger(EventLoseSkill, this, player, data);
        }

        foreach (const Skill *skill, Sanguosha->getRelatedSkills(skill_name)) {
            if (skill->isVisible())
                detachSkillFromPlayer(player, skill->objectName(), is_equip, acquire_only, head);
        }
    }
}

void Room::handleAcquireDetachSkills(ServerPlayer *player, const QStringList &skill_names, bool acquire_only)
{
    if (skill_names.isEmpty()) return;
    QList<bool> isLost;
    QList<bool> isHead;
    QStringList triggerList;
    foreach (const QString &_skill_name, skill_names) {
        if (_skill_name.startsWith("-")) {
            QString actual_skill = _skill_name.mid(1);
            bool head = true;
            if (actual_skill.endsWith("!")) {
                actual_skill.chop(1);
                head = false;
            }
            if (head && !player->getAcquiredSkills("head").contains(actual_skill) &&
                    !player->getHeadSkillList().contains(Sanguosha->getSkill(actual_skill))) continue;
            if (!head && !player->getAcquiredSkills("deputy").contains(actual_skill) &&
                    !player->getDeputySkillList().contains(Sanguosha->getSkill(actual_skill))) continue;
            if (player->getAcquiredSkills(head ? "head" : "deputy").contains(actual_skill))
                player->detachSkill(actual_skill, head);
            else if (!acquire_only)
                player->loseSkill(actual_skill, head);
            else
                continue;
            const Skill *skill = Sanguosha->getSkill(actual_skill);
            if (skill && skill->isVisible()) {
                JsonArray args;
                args << (int)QSanProtocol::S_GAME_EVENT_DETACH_SKILL;
                args << player->objectName();
                args << actual_skill;
                args << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                LogMessage log;
                log.type = "#LoseSkill";
                log.from = player;
                log.arg = actual_skill;
                sendLog(log);

                triggerList << actual_skill;
                isLost << true;
                isHead << head;
            }
        } else {
            bool head = true;
            QString skill_name = _skill_name;
            if (skill_name.endsWith("!")) {
                skill_name.chop(1);
                head = false;
            }
            const Skill *skill = Sanguosha->getSkill(skill_name);
            if (!skill) continue;
            if (player->getAcquiredSkills().contains(skill_name)) continue;
            player->acquireSkill(skill_name, head);

            if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                setPlayerMark(player, skill->getLimitMark(), 1);

            if (skill->isVisible()) {
                JsonArray args;
                args << (int)QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL;
                args << player->objectName();
                args << skill_name;
                args << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                triggerList << skill_name;
                isLost << false;
                isHead << head;
            }
        }
    }
    if (!triggerList.isEmpty()) {
        for (int i = 0; i < triggerList.length(); i++) {
            QVariant data = triggerList.at(i) + ":" + (isHead.at(i) ? "head" : "deputy");
            thread->trigger(isLost.at(i) ? EventLoseSkill : EventAcquireSkill, this, player, data);
        }
    }
}

void Room::handleAcquireDetachSkills(ServerPlayer *player, const QString &skill_names, bool acquire_only)
{
    handleAcquireDetachSkills(player, skill_names.split("|"), acquire_only);
}

bool Room::doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, bool wait)
{
    time_t timeOut = ServerInfo.getCommandTimeout(command, S_SERVER_INSTANCE);
    return doRequest(player, command, arg, timeOut, wait);
}

bool Room::doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, time_t timeOut, bool wait)
{
    Packet packet(S_SRC_ROOM | S_TYPE_REQUEST | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    player->m_isClientResponseReady = false;
    player->drainLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
    player->setClientReply(QVariant());
    player->m_isWaitingReply = true;
    player->m_expectedReplySerial = packet.createGlobalSerial();
    if (m_requestResponsePair.contains(command))
        player->m_expectedReplyCommand = m_requestResponsePair[command];
    else
        player->m_expectedReplyCommand = command;

    player->unicast(&packet);
    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    if (wait) return getResult(player, timeOut);
    else return true;
}

bool Room::doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command)
{
    time_t timeOut = ServerInfo.getCommandTimeout(command, S_SERVER_INSTANCE);
    return doBroadcastRequest(players, command, timeOut);
}

bool Room::doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut)
{
    foreach (ServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    QTime timer;
    time_t remainTime = timeOut;
    timer.start();
    foreach (ServerPlayer *player, players) {
        remainTime = timeOut - timer.elapsed();
        if (remainTime < 0) remainTime = 0;
        getResult(player, remainTime);
    }
    return true;
}

ServerPlayer *Room::doBroadcastRaceRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command,
    time_t timeOut, ResponseVerifyFunction validateFunc, void *funcArg)
{
    _m_semRoomMutex.acquire();
    _m_raceStarted = true;
    _m_raceWinner = NULL;
    while (_m_semRaceRequest.tryAcquire(1)) {
    } //drain lock
    _m_semRoomMutex.release();
    Countdown countdown;
    countdown.max = timeOut;
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    if (command != S_COMMAND_NULLIFICATION)
        notifyMoveFocus(players, countdown);

    foreach (ServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    QThread* thread = new QThread(this);
    thread->start();
    QTimer *timer = new QTimer(0);
    timer->setSingleShot(true);
    if (_m_AIraceWinner != NULL) {
        int time = timeOut - 2800;
        if (Config.OperationNoLimit)
            time = 5000 - 800;
        int AIraceRespondTime = 800 + qrand() % time;
        timer->start(AIraceRespondTime);
    }
    connect(timer, &QTimer::timeout, this, &Room::endaskfornull, Qt::DirectConnection);
    timer->moveToThread(thread);

    ServerPlayer *winner = getRaceResult(players, command, timeOut, validateFunc, funcArg);

    thread->quit();
    timer->deleteLater();
    thread->deleteLater();

    return winner;
}

ServerPlayer *Room::getRaceResult(QList<ServerPlayer *> &players, QSanProtocol::CommandType, time_t timeOut,
    ResponseVerifyFunction validateFunc, void *funcArg)
{

    QTime timer;
    timer.start();

    bool validResult = false;
    for (int i = 0; i < players.size(); i++) {
        time_t timeRemain = timeOut - timer.elapsed();

        if (timeRemain < 0) timeRemain = 0;
        bool tryAcquireResult = true;
        if (Config.OperationNoLimit)
            _m_semRaceRequest.acquire();
        else
            tryAcquireResult = _m_semRaceRequest.tryAcquire(1, timeRemain);

        if (!tryAcquireResult)
            _m_semRoomMutex.tryAcquire(1);
        // So that processResponse cannot update raceWinner when we are reading it.

        if (_m_raceWinner == NULL) {
            _m_semRoomMutex.release();
            continue;
        }

        if (validateFunc == NULL
            || (_m_raceWinner->m_isClientResponseReady
            && (this->*validateFunc)(_m_raceWinner, _m_raceWinner->getClientReply(), funcArg))) {
            validResult = true;
            break;

        } else if (_m_raceWinner == _m_AIraceWinner) {
            validResult = true;
            break;

        } else {
            // Don't give this player any more chance for this race
            _m_raceWinner->m_isWaitingReply = false;
            _m_raceWinner = NULL;
            _m_semRoomMutex.release();
        }
    }

    if (!validResult) _m_semRoomMutex.acquire();
    _m_raceStarted = false;

    foreach (ServerPlayer *player, players) {
        player->acquireLock(ServerPlayer::SEMA_MUTEX);
        player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
        player->m_isWaitingReply = false;
        player->m_expectedReplySerial = -1;
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }
    _m_semRoomMutex.release();

    return _m_raceWinner;
}

void Room::endaskfornull()
{
    if (_m_raceWinner == NULL && _m_AIraceWinner != NULL && _m_raceStarted) {
        _m_raceStarted = false;
        _m_semRaceRequest.release();
        _m_raceWinner = _m_AIraceWinner;
    }
}

bool Room::doNotify(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->unicast(&packet);
    return true;
}

bool Room::doBroadcastNotify(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, const QVariant &arg)
{
    foreach (ServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool Room::doBroadcastNotify(QSanProtocol::CommandType command, const QVariant &arg, ServerPlayer *except)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);

    foreach (ServerPlayer *player, m_players) {
        if (player != except) {
            player->unicast(&packet);
        }
    }
    return true;
}

// the following functions for Lua
bool Room::doNotify(ServerPlayer *player, int command, const char *arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, (QSanProtocol::CommandType)command);
    JsonDocument doc = JsonDocument::fromJson(arg);
    if (doc.isValid()) {
        packet.setMessageBody(doc.toVariant());
        player->unicast(&packet);
    } else {
        output(QString("Fail to parse the Json Value %1").arg(arg));
    }
    return true;
}

bool Room::doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const char *arg)
{
    foreach (ServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool Room::doBroadcastNotify(int command, const char *arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

bool Room::doNotify(ServerPlayer *player, int command, const QVariant &arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, (QSanProtocol::CommandType)command);
    packet.setMessageBody(arg);
    player->unicast(&packet);
    return true;
}

bool Room::doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const QVariant &arg)
{
    foreach (ServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool Room::doBroadcastNotify(int command, const QVariant &arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

// end for Lua

void Room::broadcast(const QSanProtocol::AbstractPacket *packet, ServerPlayer *except)
{
    broadcast(packet->toJson(), except);
}

bool Room::getResult(ServerPlayer *player, time_t timeOut)
{
    Q_ASSERT(player->m_isWaitingReply);
    bool validResult = false;
    player->acquireLock(ServerPlayer::SEMA_MUTEX);

    if (player->isOnline()) {
        player->releaseLock(ServerPlayer::SEMA_MUTEX);

        if (Config.OperationNoLimit)
            player->acquireLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        else
            player->tryAcquireLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE, timeOut);

        // Note that we rely on processResponse to filter out all unrelevant packet.
        // By the time the lock is released, m_clientResponse must be the right message
        // assuming the client side is not tampered.

        // Also note that lock can be released when a player switch to trust or offline status.
        // It is ensured by trustCommand and reportDisconnection that the player reports these status
        // is the player waiting the lock. In these cases, the serial number and command type doesn't matter.
        player->acquireLock(ServerPlayer::SEMA_MUTEX);
        validResult = player->m_isClientResponseReady;
    }
    player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
    player->m_isWaitingReply = false;
    player->m_expectedReplySerial = -1;
    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    return validResult;
}

bool Room::notifyMoveFocus(ServerPlayer *focus)
{
    QList<ServerPlayer *> players;
    players.append(focus);
    Countdown countdown;
    countdown.type = Countdown::S_COUNTDOWN_NO_LIMIT;
    return notifyMoveFocus(players, countdown, focus);
}

bool Room::notifyMoveFocus(ServerPlayer *focus, CommandType command)
{
    QList<ServerPlayer *> players;
    players.append(focus);
    Countdown countdown;
    countdown.max = ServerInfo.getCommandTimeout(command, S_CLIENT_INSTANCE);
    if (countdown.max == ServerInfo.getCommandTimeout(S_COMMAND_UNKNOWN, S_CLIENT_INSTANCE)) {
        countdown.type = Countdown::S_COUNTDOWN_USE_DEFAULT;
    } else {
        countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    }

    return notifyMoveFocus(players, countdown, focus);
}

bool Room::notifyMoveFocus(const QList<ServerPlayer *> &focuses, const Countdown &countdown, ServerPlayer *except)
{
    JsonArray arg;
    //============================================
    //for protecting anjiang
    //============================================
    bool verify = false;
    foreach (ServerPlayer *focus, focuses) {
        if (focus->hasFlag("Global_askForSkillCost")) {
            verify = true;
            break;
        }
    }

    if (!verify) {
        JsonArray players;
        int n = focuses.size();
        for (int i = 0; i < n; i++) {
            players << focuses.at(i)->objectName();
        }
        arg << QVariant(players);
    } else {
        arg << QSanProtocol::S_ALL_ALIVE_PLAYERS;
    }
    //============================================

    if (countdown.type != Countdown::S_COUNTDOWN_USE_DEFAULT) {
        arg << countdown.toVariant();
    }

    return doBroadcastNotify(S_COMMAND_MOVE_FOCUS, arg, except);
}

bool Room::askForSkillInvoke(ServerPlayer *player, const QString &skill_name, const QVariant &data)
{
    tryPause();
    if (data.type() == QVariant::String && data.toString() == "GameStart") {
        Countdown countdown;
        countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_INVOKE_SKILL, S_CLIENT_INSTANCE);
        if (countdown.max == ServerInfo.getCommandTimeout(S_COMMAND_UNKNOWN, S_CLIENT_INSTANCE))
            countdown.type = Countdown::S_COUNTDOWN_USE_DEFAULT;
        else
            countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;

        notifyMoveFocus(getAllPlayers(), countdown);
    } else
        notifyMoveFocus(player, S_COMMAND_INVOKE_SKILL);

    bool invoked = false;
    AI *ai = player->getAI();
    if (ai) {
        invoked = ai->askForSkillInvoke(skill_name, data);
        if (skill_name.endsWith("!"))
            invoked = false;
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (invoked && !(skill && skill->getFrequency() != Skill::NotFrequent))
            thread->delay();
    } else {
        JsonArray skillCommand;
        if (data.type() == QVariant::String)
            skillCommand << skill_name << data.toString();
        else {
            ServerPlayer *player = data.value<ServerPlayer *>();
            QString data_str;
            if (player != NULL)
                data_str = "playerdata:" + player->objectName();
            skillCommand << skill_name << data_str;
        }
        if (player != NULL) {
            const Skill *mainskill = Sanguosha->getMainSkill(skill_name);
            if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
                skillCommand << getTag(mainskill->objectName() + player->objectName()).toStringList().last();
        }
        if (!doRequest(player, S_COMMAND_INVOKE_SKILL, skillCommand, true) || skill_name.endsWith("!"))
            invoked = false;
        else {
            const QVariant &clientReply = player->getClientReply();
            if (JsonUtils::isBool(clientReply))
                invoked = clientReply.toBool();
        }
    }

    if (invoked) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill && skill->inherits("TriggerSkill")) {
            const TriggerSkill *tr_skill = qobject_cast<const TriggerSkill *>(skill);
            if (tr_skill && !tr_skill->isGlobal()) {
                JsonArray msg;
                msg << skill_name;
                msg << player->objectName();
                doBroadcastNotify(S_COMMAND_INVOKE_SKILL, msg);
                notifySkillInvoked(player, skill_name);
            }
        }
    }

    QVariant decisionData = QVariant::fromValue("skillInvoke:" + skill_name + ":" + (invoked ? "yes" : "no"));
    thread->trigger(ChoiceMade, this, player, decisionData);
    return invoked;
}

QString Room::askForChoice(ServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data)
{
    tryPause();


    QStringList validChoices;
    foreach (const QString &choice, choices.split("|"))
        validChoices.append(choice.split("+"));
    QStringList titles = skill_name.split("%");
    QString skillname = titles.at(0);

    Q_ASSERT(!validChoices.isEmpty());

    QString answer;
    if (validChoices.length() == 1) {
        answer = validChoices.first();
    } else {
        notifyMoveFocus(player, S_COMMAND_MULTIPLE_CHOICE);

        AI *ai = player->getAI();
        if (ai) {
            answer = ai->askForChoice(skillname, choices, data);
            thread->delay();
        } else {
            bool success = doRequest(player, S_COMMAND_MULTIPLE_CHOICE, JsonArray() << skill_name << choices, true);
            const QVariant &clientReply = player->getClientReply();
            if (!success || !JsonUtils::isString(clientReply)) {
                answer = ".";
            } else
                answer = clientReply.toString();
        }
        bool check = false;
        foreach (const QString &c, validChoices) {
            if ((c == answer) || (c.split("%").at(0) == answer.split("%").at(0))) {
                check = true;
                break;
            }
        }
        if (!check)
            answer = validChoices[0];
    }

    QVariant decisionData = QVariant::fromValue("skillChoice:" + skillname + ":" + answer);
    thread->trigger(ChoiceMade, this, player, decisionData);
    return answer;
}

void Room::obtainCard(ServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide)
{
    if (card == NULL) return;
    moveCardTo(card, NULL, target, Player::PlaceHand, reason, unhide);
}

void Room::obtainCard(ServerPlayer *target, const Card *card, bool unhide)
{
    if (card == NULL) return;
    CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, target->objectName());
    obtainCard(target, card, reason, unhide);
}

void Room::obtainCard(ServerPlayer *target, int card_id, bool unhide)
{
    obtainCard(target, Sanguosha->getCard(card_id), unhide);
}

bool Room::isCanceled(const CardEffectStruct &effect)
{
    if (!effect.card->isCancelable(effect))
        return false;

    QStringList targets = getTag(effect.card->toString() + "HegNullificationTargets").toStringList();
    if (!targets.isEmpty()) {
        if (targets.contains(effect.to->objectName())) {
            LogMessage log;
            log.type = "#HegNullificationEffect";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = effect.card->objectName();
            sendLog(log);
            return true;
        }
    }

    setTag("HegNullificationValid", false);

    QVariant decisionData = QVariant::fromValue(effect.to);
    setTag("NullifyingTarget", decisionData);
    decisionData = QVariant::fromValue(effect.from);
    setTag("NullifyingSource", decisionData);
    decisionData = QVariant::fromValue(effect.card);
    setTag("NullifyingCard", decisionData);
    setTag("NullifyingTimes", 0);
    bool result = askForNullification(effect.card, effect.from, effect.to, true);
    if (getTag("HegNullificationValid").toBool() && effect.card->isNDTrick()) {
        foreach (ServerPlayer *p, m_players) {
            if (p->isAlive() && p->isFriendWith(effect.to))
                targets << p->objectName();
        }
        setTag(effect.card->toString() + "HegNullificationTargets", targets);
    }
    return result;
}

bool Room::verifyNullificationResponse(ServerPlayer *player, const QVariant &response, void *)
{
    const Card *card = NULL;
    if (player != NULL && response.canConvert<JsonArray>()) {
        JsonArray responseArray = response.value<JsonArray>();
        if (JsonUtils::isString(responseArray[0]))
            card = Card::Parse(responseArray[0].toString());
    }
    return card != NULL;
}

bool Room::askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive)
{
    _NullificationAiHelper aiHelper;
    aiHelper.m_from = from;
    aiHelper.m_to = to;
    aiHelper.m_trick = trick;
    return _askForNullification(trick, from, to, positive, aiHelper);
}

bool Room::_askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive, _NullificationAiHelper aiHelper)
{
    tryPause();

    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    QString trick_name = trick->objectName();
    QList<ServerPlayer *> validHumanPlayers;
    QList<ServerPlayer *> validAiPlayers;

    JsonArray arg;
    arg << trick_name;
    arg << (from ? QVariant(from->objectName()) : QVariant());
    arg << (to ? QVariant(to->objectName()) : QVariant());

    CardEffectStruct trickEffect;
    trickEffect.card = trick;
    trickEffect.from = from;
    trickEffect.to = to;
    QVariant data = QVariant::fromValue(trickEffect);
    foreach (ServerPlayer *player, m_alivePlayers) {
        if (player->hasNullification()) {
            if (!thread->trigger(TrickCardCanceling, this, player, data)) {
                if (player->isOnline()) {
                    player->m_commandArgs = arg;
                    validHumanPlayers << player;
                } else
                    validAiPlayers << player;
            }
        }
    }

    QList<ServerPlayer *> AIs;
    foreach (ServerPlayer *player, validAiPlayers) {
        AI *ai = player->getAI();
        if (ai == NULL) continue;
        const Card *card = ai->askForNullification(aiHelper.m_trick, aiHelper.m_from, aiHelper.m_to, positive);
        if (card && player->isCardLimited(card, Card::MethodUse))
            card = NULL;
        if (card != NULL) {
            AIs << player;
        }
    }
    _m_AIraceWinner = NULL;
    if (AIs.length() > 0) {
        int index = qrand() % AIs.length();
        _m_AIraceWinner = AIs.at(index);

    }

    ServerPlayer *repliedPlayer = NULL;
    time_t timeOut = ServerInfo.getCommandTimeout(S_COMMAND_NULLIFICATION, S_SERVER_INSTANCE);
    Countdown countdown;
    countdown.max = timeOut;
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    notifyMoveFocus(getAllPlayers(), countdown);

    if (!validHumanPlayers.isEmpty()) {

        if (_m_AIraceWinner != NULL)
            validHumanPlayers << _m_AIraceWinner;

        foreach (ServerPlayer *p, validHumanPlayers)
            doNotify(p, S_COMMAND_NULLIFICATION_ASKED, trick->objectName());

        repliedPlayer = doBroadcastRaceRequest(validHumanPlayers, S_COMMAND_NULLIFICATION,
            timeOut, &Room::verifyNullificationResponse);
    }

    if (validHumanPlayers.isEmpty() && Config.AIDelay != 0 && _m_AIraceWinner != NULL)
        thread->delay(Config.AIDelay + 500);

    arg.clear();
    arg << true;

    Packet packet(S_SRC_ROOM | S_TYPE_REQUEST | S_DEST_CLIENT, S_COMMAND_NULLIFICATION);
    packet.setMessageBody(arg);

    foreach (ServerPlayer *player, getAllPlayers())
        player->unicast(&packet);

    const Card *card = NULL;
    if (repliedPlayer == NULL) repliedPlayer = _m_AIraceWinner;
    if (repliedPlayer != NULL) {
        if (repliedPlayer != _m_AIraceWinner) {
            JsonArray clientReply = repliedPlayer->getClientReply().value<JsonArray>();
            if (clientReply.size() > 0 && JsonUtils::isString(clientReply[0]))
                card = Card::Parse(clientReply[0].toString());
        } else {
            AI *ai = repliedPlayer->getAI();
            card = ai->askForNullification(aiHelper.m_trick, aiHelper.m_from, aiHelper.m_to, positive);
        }
    }

    if (card == NULL) return false;

    card = card->validateInResponse(repliedPlayer);

    if (card == NULL)
        return _askForNullification(trick, from, to, positive, aiHelper);
    if (to != NULL)
        doAnimate(S_ANIMATE_NULLIFICATION, repliedPlayer->objectName(), to->objectName());
    useCard(CardUseStruct(card, repliedPlayer, QList<ServerPlayer *>()));
    bool isHegNullification = false;
    QString heg_nullification_selection;
    if (card->isKindOf("HegNullification") && !trick->isKindOf("Nullification") && trick->isNDTrick() && to->getRole() != "careerist" && to->hasShownOneGeneral()) {

        QVariantList qtargets = tag["targets" + trick->toString()].toList();
        QList<ServerPlayer *> targets;
        for (int i = 0; i < qtargets .size(); i++) {
            QVariant n = qtargets.at(i);
            targets.append(n.value<ServerPlayer *>());
        }
        QList<ServerPlayer *> targets_copy = targets;
        targets.removeOne(to);
        foreach (ServerPlayer *p, targets_copy) {
            if (targets_copy.indexOf(p) < targets_copy.indexOf(to))
                targets.removeOne(p);
        }
        if (!targets.isEmpty()) {
            foreach (ServerPlayer *p, targets) {
                if (p->getRole() != "careerist" && p->hasShownOneGeneral() && p->getKingdom() == to->getKingdom()) {
                    isHegNullification = true;
                    break;
                }
            }
        }
        if (isHegNullification) {
            QString log = trick->getFullName(true);
            heg_nullification_selection = askForChoice(repliedPlayer, "heg_nullification%log:" + log, "single%to:" + to->objectName() +
                "+all%to:" + to->objectName() + "%log:" + Sanguosha->translate(to->getKingdom()), data);
        }
        if (heg_nullification_selection.contains("all"))
            heg_nullification_selection = "all";
        else
            heg_nullification_selection = "single";
    }

    if (!isHegNullification) {
        LogMessage log;
        log.type = "#NullificationDetails";
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
    } else {
        LogMessage log;
        log.type = "#HegNullificationDetails";
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
        LogMessage log2;
        log2.type = "#HegNullificationSelection";
        log2.from = repliedPlayer;
        log2.arg = "hegnul_" + heg_nullification_selection;
        sendLog(log2);
    }

    setTag("NullificatonType", isHegNullification);
    thread->delay(500);

    QVariant decisionData = QVariant::fromValue("Nullification:" + QString(trick->getClassName())
        + ":" + to->objectName() + ":" + (positive ? "true" : "false"));
    thread->trigger(ChoiceMade, this, repliedPlayer, decisionData);
    setTag("NullifyingTimes", getTag("NullifyingTimes").toInt() + 1);

    bool result = true;
    CardEffectStruct effect;
    effect.card = card;
    effect.to = repliedPlayer;
    if (card->isCancelable(effect))
        result = !_askForNullification(card, repliedPlayer, to, !positive, aiHelper);

    removeTag("NullificatonType");

    if (isHegNullification && heg_nullification_selection == "all" && result) {
        setTag("HegNullificationValid", true);
    }

    return result;
}

int Room::askForCardChosen(ServerPlayer *player, ServerPlayer *who, const QString &flags, const QString &reason,
    bool handcard_visible, Card::HandlingMethod method, const QList<int> &disabled_ids)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_CARD);
    //Q_ASSERT(!who->getCards(flags).isEmpty()); //a very six solution...

    QList<int> card_ids;
    QList<int> disabled_ids_copy = disabled_ids;
    QString flags_copy = flags;
    foreach (const Card *card, who->getCards(flags))
        card_ids << card->getEffectiveId();
    if (method == Card::MethodDiscard) {
        foreach (int card_id, card_ids)
            if (!player->canDiscard(who, card_id))
                disabled_ids_copy.append(card_id);
        if (flags_copy.contains("h") && !player->canDiscard(who, "h"))
            flags_copy.remove("h");
        if (flags_copy.contains("e") && !player->canDiscard(who, "e"))
            flags_copy.remove("e");
        if (flags_copy.contains("j") && !player->canDiscard(who, "j"))
            flags_copy.remove("j");
    }
    if (method == Card::MethodGet) {
        foreach (int card_id, card_ids)
            if (!player->canGetCard(who, card_id))
                disabled_ids_copy.append(card_id);
        if (flags_copy.contains("h") && !player->canGetCard(who, "h"))
            flags_copy.remove("h");
        if (flags_copy.contains("e") && !player->canGetCard(who, "e"))
            flags_copy.remove("e");
        if (flags_copy.contains("j") && !player->canGetCard(who, "j"))
            flags_copy.remove("j");
    }

    QList<const Card *> available = who->getCards(flags_copy);
    foreach (int id, disabled_ids_copy) {
        const Card *c = getCard(id);
        if (c && available.contains(c))
            available.removeOne(c);
    }
    if (available.isEmpty()) return Card::S_UNKNOWN_CARD_ID;
    if ((handcard_visible && !who->isKongcheng())) {
        QList<int> handcards = who->handCards();
        JsonArray arg;
        arg << who->objectName();
        arg << JsonUtils::toJsonArray(handcards);
        doNotify(player, S_COMMAND_SET_KNOWN_CARDS, arg);
    }
    int card_id = Card::S_UNKNOWN_CARD_ID;
    if (who != player && !handcard_visible && player->getAI()
        && (flags_copy == "h"
        || (flags_copy == "he" && !who->hasEquip())
        || (flags_copy == "hej" && !who->hasEquip() && who->getJudgingArea().isEmpty()))) {
        QList<int> handcards = who->handCards();
        foreach (int id, disabled_ids_copy)
            handcards.removeOne(id);
        card_id = handcards.at(qrand() % handcards.length());
    } else {
        AI *ai = player->getAI();
        if (ai) {
            thread->delay();
            card_id = ai->askForCardChosen(who, flags_copy, reason.split("%").at(0), method, disabled_ids_copy);
            if (card_id == -1) {
                QList<const Card *> cards = who->getCards(flags_copy);
                foreach (const Card *card, cards)
                    if (disabled_ids_copy.contains(card->getEffectiveId()))
                        cards.removeOne(card);

                Q_ASSERT(!cards.isEmpty());
                card_id = cards.at(qrand() % cards.length())->getId();
            }
        } else {
            QList<int> handcards;
            if (who->hasFlag("continuous_card_chosen")) {
                handcards = VariantList2IntList(tag["askforCardsChosen"].toList());
            } else {
                handcards = who->handCards();
                qShuffle(handcards);
            }

            JsonArray arg;
            arg << who->objectName();
            arg << flags_copy;
            arg << reason;
            arg << handcard_visible;
            arg << (int)method;
            arg << JsonUtils::toJsonArray(disabled_ids_copy);
            arg << JsonUtils::toJsonArray(handcards);
            bool success = doRequest(player, S_COMMAND_CHOOSE_CARD, arg, true);

            //@todo: check if the card returned is valid
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            if (!success || clientReply.size() != 2) {
                // randomly choose a card
                QList<const Card *> cards = who->getCards(flags_copy);
                foreach (int id, disabled_ids_copy)
                    cards.removeOne(Sanguosha->getCard(id));
                card_id = cards.at(qrand() % cards.length())->getId();
            } else {
                card_id = clientReply.at(0).toInt();

                int index = clientReply.at(1).toInt();

                if (card_id == Card::S_UNKNOWN_CARD_ID)
                    card_id = handcards.at(index);
            }
        }
    }

    Q_ASSERT(card_id != Card::S_UNKNOWN_CARD_ID);

    QVariant decisionData = QVariant::fromValue(QString("cardChosen:%1:%2:%3:%4").arg(reason).arg(card_id)
        .arg(player->objectName()).arg(who->objectName()));
    if (!who->hasFlag("continuous_card_chosen"))
        thread->trigger(ChoiceMade, this, player, decisionData);
    return card_id;
}

QList<int> Room::askForCardsChosen(ServerPlayer *chooser, ServerPlayer *choosee, const QStringList &handle_list, const QString &reason)
{
    QList<int> result;
    result.clear();
    setPlayerFlag(choosee, "continuous_card_chosen");

    QList<int> handcard_ids = choosee->handCards();
    qShuffle(handcard_ids);
    tag["askforCardsChosen"] = IntList2VariantList(handcard_ids);

    if (chooser && chooser->isAlive() && choosee && choosee->isAlive() && !choosee->isAllNude()) {
        foreach (const QString &src, handle_list) {
            if (src.isEmpty()) continue;
            QStringList handle = src.split("^");
            if (handle[0].isEmpty()) continue;
            if (choosee->getCards(handle[0]).isEmpty()) continue;
            if (handle.length() == 1) handle.append("false");
            if (handle.length() == 2) handle.append("none");
            QList<int> ids;
            ids.clear();
            if (handle.length() > 3) {
                foreach(const QString &id, handle[3].split("+"))
                    ids.append(id.toInt());
            }
            int id = askForCardChosen(chooser, choosee, handle[0], reason, handle[1] == "true",
                Sanguosha->getCardHandlingMethod(handle[2]), ids + result);
            if (id != Card::S_UNKNOWN_CARD_ID)
                result << id;
        }
    }
    setPlayerFlag(choosee, "-continuous_card_chosen");
    tag.remove("askforCardsChosen");

    QVariant decisionData = QVariant::fromValue(QString("cardChosen:%1:%2:%3:%4").arg(reason).arg(IntList2StringList(result).join("+"))
        .arg(chooser->objectName()).arg(choosee->objectName()));
    thread->trigger(ChoiceMade, this, chooser, decisionData);

    return result;
}

QList<const Card *> Room::askForCardsChosen(ServerPlayer *chooser, ServerPlayer *choosee, const QString &handle_string, const QString &reason)
{
    QList<int> value = askForCardsChosen(chooser, choosee, handle_string.split("|"), reason);
    QList<const Card *> result;
    foreach (int id, value)
        result.append(getCard(id));
    return result;
}

const Card *Room::askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt,
    const QVariant &data, const QString &skill_name)
{
    return askForCard(player, pattern, prompt, data, Card::MethodDiscard, NULL, false, skill_name, false);
}

const Card *Room::askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt,
    const QVariant &data, Card::HandlingMethod method, ServerPlayer *to,
    bool isRetrial, const QString &_skill_name, bool isProvision)
{

    Q_ASSERT(pattern != "slash" || method != Card::MethodUse); // use askForUseSlashTo instead
    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    _m_roomState.setCurrentCardUsePattern(pattern);

    const Card *card = NULL;

    CardUseStruct::CardUseReason reason = CardUseStruct::CARD_USE_REASON_UNKNOWN;
    if (method == Card::MethodResponse)
        reason = CardUseStruct::CARD_USE_REASON_RESPONSE;
    else if (method == Card::MethodUse)
        reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    _m_roomState.setCurrentCardUseReason(reason);

    QStringList asked;
    asked << pattern << prompt;
    QVariant asked_data = QVariant::fromValue(asked);
    if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial && !player->hasFlag("continuing"))
        thread->trigger(CardAsked, this, player, asked_data);

    if (player->hasFlag("continuing"))
        setPlayerFlag(player, "-continuing");
    if (has_provided || !player->isAlive()) {
        card = provided;
        if (player->isCardLimited(card, method)) card = NULL;
        provided = NULL;
        has_provided = false;
    } else {
        AI *ai = player->getAI();
        if (ai) {
            card = ai->askForCard(pattern, prompt, data);
            if (card && card->isKindOf("DummyCard") && card->subcardsLength() == 1)
                card = Sanguosha->getCard(card->getEffectiveId());
            if (card && player->isCardLimited(card, method)) card = NULL;
            if (card) thread->delay();
        } else {
            JsonArray arg;
            arg << pattern;
            arg << prompt;
            arg << int(method);
            bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, arg, true);
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            if (success && !clientReply.isEmpty())
                card = Card::Parse(clientReply[0].toString());
        }
    }

    if (card == NULL) {
        QVariant decisionData = QVariant::fromValue(QString("cardResponded:%1:%2:_nil_").arg(pattern).arg(prompt));
        thread->trigger(ChoiceMade, this, player, decisionData);
        return NULL;
    }

    card = card->validateInResponse(player);
    const Card *result = NULL;

    if (card) {
        if (!card->isVirtualCard()) {
            WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getEffectiveId());
            if (wrapped->isModified())
                broadcastUpdateCard(getPlayers(), card->getEffectiveId(), wrapped);
            else
                broadcastResetCard(getPlayers(), card->getEffectiveId());
        }

        if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial) {
            LogMessage log;
            log.card_str = card->toString();
            log.from = player;
            log.type = QString("#%1").arg(card->getClassName());
            if (method == Card::MethodResponse)
                log.type += "_Resp";
            sendLog(log);
            player->broadcastSkillInvoke(card);
        } else if (method == Card::MethodDiscard) {
            LogMessage log;
            log.type = _skill_name.isEmpty() ? "$DiscardCard" : "$DiscardCardWithSkill";
            log.from = player;
            QList<int> to_discard;
            if (card->isVirtualCard())
                to_discard.append(card->getSubcards());
            else
                to_discard << card->getEffectiveId();
            log.card_str = IntList2StringList(to_discard).join("+");
            if (!_skill_name.isEmpty())
                log.arg = _skill_name;
            sendLog(log);
            if (!_skill_name.isEmpty())
                notifySkillInvoked(player, _skill_name);
        }
    }

    bool isHandcard = true;
    if (card) {
        QList<int> ids;
        if (!card->isVirtualCard()) ids << card->getEffectiveId();
        else ids = card->getSubcards();
        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                if (getCardOwner(id) != player || getCardPlace(id) != Player::PlaceHand) {
                    isHandcard = false;
                    break;
                }
            }
        } else {
            isHandcard = false;
        }

        QVariant decisionData = QVariant::fromValue(QString("cardResponded:%1:%2:_%3_").arg(pattern).arg(prompt).arg(card->toString()));
        thread->trigger(ChoiceMade, this, player, decisionData);

        if (method == Card::MethodUse) {
            QList<int> card_ids;
            if (card->isVirtualCard())
                card_ids = card->getSubcards();
            else
                card_ids << card->getEffectiveId();
            if (!card_ids.isEmpty()) {
                CardMoveReason reason(CardMoveReason::S_REASON_LETUSE, player->objectName(), QString(), card->getSkillName(), QString());
                if (!card->getSkillPosition().isEmpty())
                    reason.m_eventName = card->getSkillPosition() == "left" ? player->getActualGeneral1Name() : player->getActualGeneral2Name();
                QList<CardsMoveStruct> moves;
                foreach (int id, card_ids) {
                    CardsMoveStruct move(id, NULL, Player::PlaceTable, reason);
                    moves.append(move);
                }
                moveCardsAtomic(moves, true);
            }
            player->showSkill(card->showSkill(), card->getSkillPosition());
        } else if (method == Card::MethodDiscard) {
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName());
            moveCardTo(card, player, NULL, Player::PlaceTable, reason, true);
            QList<int> table_cardids = getCardIdsOnTable(card);
            if (!table_cardids.isEmpty()) {
                DummyCard dummy(table_cardids);
                moveCardTo(&dummy, player, NULL, Player::DiscardPile, reason, true);
            }
        } else if (method != Card::MethodNone && !isRetrial) {
            QList<int> card_ids;
            if (card->isVirtualCard())
                card_ids = card->getSubcards();
            else
                card_ids << card->getEffectiveId();
            if (!card_ids.isEmpty()) {
                CardMoveReason reason(CardMoveReason::S_REASON_RESPONSE, player->objectName());
                reason.m_skillName = card->getSkillName();
                if (!card->getSkillPosition().isEmpty())
                    reason.m_eventName = card->getSkillPosition() == "left" ? player->getActualGeneral1Name() : player->getActualGeneral2Name();
                QList<CardsMoveStruct> moves;
                foreach (int id, card_ids) {
                    CardsMoveStruct move(id, NULL, Player::PlaceTable, reason);
                    moves.append(move);
                }
                moveCardsAtomic(moves, true);
            }
            player->showSkill(card->showSkill(), card->getSkillPosition());
        }

        if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial) {
            if (!card->getSkillName().isNull() && card->getSkillName(true) == card->getSkillName(false)
                && player->hasSkill(card->getSkillName()))
                notifySkillInvoked(player, card->getSkillName());
            CardResponseStruct resp(card, to, method == Card::MethodUse);
            resp.m_isHandcard = isHandcard;
            QVariant data = QVariant::fromValue(resp);
            thread->trigger(CardResponded, this, player, data);
            if (method == Card::MethodUse) {
                QList<int> table_cardids = getCardIdsOnTable(card);
                if (!table_cardids.isEmpty()) {
                    DummyCard dummy(table_cardids);
                    CardMoveReason reason(CardMoveReason::S_REASON_LETUSE, player->objectName(),
                        QString(), card->getSkillName(), QString());
                    if (!card->getSkillPosition().isEmpty())
                        reason.m_eventName = card->getSkillPosition() == "left" ? player->getActualGeneral1Name() : player->getActualGeneral2Name();
                    moveCardTo(&dummy, NULL, Player::DiscardPile, reason, true);
                }
                CardUseStruct card_use;
                card_use.card = card;
                card_use.from = player;
                if (to) card_use.to << to;
                QVariant data2 = QVariant::fromValue(card_use);
                thread->trigger(CardFinished, this, player, data2);
            } else if (!isProvision) {
                QList<int> table_cardids = getCardIdsOnTable(card);
                if (!table_cardids.isEmpty()) {
                    DummyCard dummy(table_cardids);
                    CardMoveReason reason(CardMoveReason::S_REASON_RESPONSE, player->objectName());
                    reason.m_skillName = card->getSkillName();
                    if (!card->getSkillPosition().isEmpty())
                        reason.m_eventName = card->getSkillPosition() == "left" ? player->getActualGeneral1Name() : player->getActualGeneral2Name();
                    moveCardTo(&dummy, NULL, Player::DiscardPile, reason, true);
                }
            }
        }
        result = card;
    } else {
        setPlayerFlag(player, "continuing");
        result = askForCard(player, pattern, prompt, data, method, to, isRetrial);
    }
    return result;
}

const Card *Room::askForUseCard(ServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index,
    Card::HandlingMethod method, bool addHistory)
{
    Q_ASSERT(method != Card::MethodResponse);
    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    _m_roomState.setCurrentCardUsePattern(pattern);
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    CardUseStruct card_use;

    bool isCardUsed = false;
    AI *ai = player->getAI();
    if (ai) {
        QString answer = ai->askForUseCard(pattern, prompt, method);
        if (answer != ".") {
            isCardUsed = true;
            card_use.from = player;
            card_use.parse(answer, this);
            thread->delay();
        }
    } else {
        JsonArray ask_str;
        ask_str << pattern;
        ask_str << prompt;
        ask_str << int(method);
        ask_str << notice_index;
        QRegExp rx("@@([_A-Za-z]+)(\\d+)?!?");
        if (rx.exactMatch(pattern)) {
            QStringList texts = rx.capturedTexts();
            if (!getTag(texts.at(1) + player->objectName()).toStringList().isEmpty())
                ask_str << getTag(texts.at(1) + player->objectName()).toStringList().last();
        }

        bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, ask_str, true);
        if (success) {
            const QVariant &clientReply = player->getClientReply();
            isCardUsed = !clientReply.isNull();
            if (isCardUsed && card_use.tryParse(clientReply, this))
                card_use.from = player;
        }
    }
    card_use.m_reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    if (isCardUsed && card_use.isValid(pattern)) {
        QVariant decisionData = QVariant::fromValue(card_use);
        thread->trigger(ChoiceMade, this, player, decisionData);
        if (!useCard(card_use, addHistory))
            return askForUseCard(player, pattern, prompt, notice_index, method, addHistory);
        return card_use.card;
    } else {
        QVariant decisionData = QVariant::fromValue("cardUsed:" + pattern + ":" + prompt + ":nil");
        thread->trigger(ChoiceMade, this, player, decisionData);
    }

    return NULL;
}

const Card *Room::askForUseSlashTo(ServerPlayer *slasher, QList<ServerPlayer *> victims, const QString &prompt,
    bool distance_limit, bool disable_extra, bool addHistory)
{
    Q_ASSERT(!victims.isEmpty());

    // The realization of this function in the Slash::onUse and Slash::targetFilter.
    setPlayerFlag(slasher, "slashTargetFix");
    if (!distance_limit)
        setPlayerFlag(slasher, "slashNoDistanceLimit");
    if (disable_extra)
        setPlayerFlag(slasher, "slashDisableExtraTarget");
    if (victims.length() == 1)
        setPlayerFlag(slasher, "slashTargetFixToOne");
    foreach (ServerPlayer *victim, victims)
        setPlayerFlag(victim, "SlashAssignee");

    const Card *slash = askForUseCard(slasher, "slash", prompt, -1, Card::MethodUse, addHistory);
    if (slash == NULL) {
        setPlayerFlag(slasher, "-slashTargetFix");
        setPlayerFlag(slasher, "-slashTargetFixToOne");
        foreach (ServerPlayer *victim, victims)
            setPlayerFlag(victim, "-SlashAssignee");
        if (slasher->hasFlag("slashNoDistanceLimit"))
            setPlayerFlag(slasher, "-slashNoDistanceLimit");
        if (slasher->hasFlag("slashDisableExtraTarget"))
            setPlayerFlag(slasher, "-slashDisableExtraTarget");
    }

    return slash;
}

const Card *Room::askForUseSlashTo(ServerPlayer *slasher, ServerPlayer *victim, const QString &prompt,
    bool distance_limit, bool disable_extra, bool addHistory)
{
    Q_ASSERT(victim != NULL);
    QList<ServerPlayer *> victims;
    victims << victim;
    return askForUseSlashTo(slasher, victims, prompt, distance_limit, disable_extra, addHistory);
}

QList<int> Room::GlobalCardChosen(ServerPlayer *player, QList<ServerPlayer *> targets, const QString &flags, const QString &skillName, const QString &prompt,
    int min, int max, ChoosingType type, bool handcard_visible, Card::HandlingMethod method, const QList<int> &disabled_ids, bool notify_skill)
{
    if (targets.isEmpty() || (type == OnebyOne && min > 0 && targets.length() < min) || (min > max && max > 0))
        return QList<int>();

    tryPause();
    notifyMoveFocus(player, S_COMMAND_GLOBAL_CHOOSECARD);

    JsonArray req;
    JsonArray req_targets;
    foreach (ServerPlayer *target, targets)
        req_targets << target->objectName();
    req << QVariant(req_targets);
    req << flags;
    req << skillName;
    req << prompt;
    req << min;
    req << max;
    req << type;
    req << handcard_visible;

    QList<int> disabled_ids_copy = disabled_ids;
    QList<int> available;
    QList<int> type0;
    foreach (ServerPlayer *p, targets) {
        QList<int> card_ids;

        if ((handcard_visible && !p->isKongcheng())) {
            QList<int> handcards = p->handCards();
            JsonArray arg;
            arg << p->objectName();
            arg << JsonUtils::toJsonArray(handcards);
            doNotify(player, S_COMMAND_SET_KNOWN_CARDS, arg);
        }

        foreach (const Card *card, p->getCards(flags)) {
            int card_id = card->getEffectiveId();
            if (method == Card::MethodDiscard) {
                if (!player->canDiscard(p, card_id))
                    disabled_ids_copy.append(card_id);
            }
            if (method == Card::MethodGet) {
                if (!player->canGetCard(p, card_id))
                    disabled_ids_copy.append(card_id);
            }
            if (!disabled_ids_copy.contains(card_id)) {
                available << card_id;
                card_ids << card_id;
            }
        }
        qShuffle(card_ids);
        if (type0.length() < min && min > 0) type0 << card_ids.first();
        card_ids.clear();
        if (flags.contains("h")) {
            card_ids = p->handCards();
            qShuffle(card_ids);
        }
        req << JsonUtils::toJsonArray(card_ids);
    }
    req << JsonUtils::toJsonArray(disabled_ids_copy);

    if (available.isEmpty()) return QList<int>();
    if (available.length() <= min && type != OnebyOne) return available;
    if (type == OnebyOne && type0.length() < min) return  QList<int>();

    QList<int> result;

    AI *ai = player->getAI();
    if (ai) {
        result = ai->askForCardsChosen(targets, flags, skillName, min, max, disabled_ids_copy);
        if (!result.isEmpty() && notify_skill)
            thread->delay();
    } else {
        const Skill *mainskill = Sanguosha->getMainSkill(skillName);
        if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
            req << getTag(mainskill->objectName() + player->objectName()).toStringList().last();

        bool success = doRequest(player, S_COMMAND_GLOBAL_CHOOSECARD, req, true);
        if (success) {
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            JsonUtils::tryParse(clientReply, result);
        }
    }
    bool check = true;
    foreach (int id, result)
        if (disabled_ids_copy.contains(id))
            check = false;
    if (result.length() < min || (result.length() > max && max > 0)) check = false;
    if (!check) {
        if (type == OnebyOne)
            result = type0;
        else {
            result.clear();
            for (int i = 1; i <= min; i ++) {
                while (result.length() < i) {
                    int id = available.at(qrand() % available.length());
                    if (!result.contains(id))
                        result << id;
                }
            }
        }
        check = true;
    }
    if (result.isEmpty()) return QList<int>();

    QList<ServerPlayer *> players;
    foreach (int id, result) {
        ServerPlayer *p = getCardOwner(id);
        if (!players.contains(p))
            players << p;
    }
    if (check && notify_skill) {
        notifySkillInvoked(player, skillName);
        QVariant decisionData = QVariant::fromValue("skillInvoke:" + skillName + ":yes");
        thread->trigger(ChoiceMade, this, player, decisionData);
        foreach (ServerPlayer *p, players)
            doAnimate(S_ANIMATE_INDICATE, player->objectName(), p->objectName());
        LogMessage log;
        log.type = "#ChoosePlayerWithSkill";
        log.from = player;
        log.to = players;
        log.arg = skillName;
        sendLog(log);
    }

    foreach (ServerPlayer *p, players) {
        QVariant data = QString("%1:%2:%3").arg("playerChosen").arg(skillName).arg(p->objectName());
        thread->trigger(ChoiceMade, this, player, data);

        QList<int> ids;
        foreach (int id, result)
            if (getCardOwner(id) == p) ids << id;
        QVariant decisionData = QVariant::fromValue(QString("cardChosen:%1:%2:%3:%4").arg(skillName).arg(IntList2StringList(ids).join("+"))
            .arg(player->objectName()).arg(p->objectName()));
        thread->trigger(ChoiceMade, this, player, decisionData);
    }
    return result;
}

int Room::askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_AMAZING_GRACE);
    Q_ASSERT(card_ids.length() > 0);

    int card_id = -1;
    if (card_ids.length() == 1 && !refusable)
        card_id = card_ids.first();
    else {
        AI *ai = player->getAI();
        if (ai) {
            thread->delay();
            card_id = ai->askForAG(card_ids, refusable, reason);
        } else {
            bool success = doRequest(player, S_COMMAND_AMAZING_GRACE, refusable, true);
            const QVariant &clientReply = player->getClientReply();
            if (success && JsonUtils::isNumber(clientReply))
                card_id = clientReply.toInt();
        }

        if (!card_ids.contains(card_id))
            card_id = refusable ? -1 : card_ids.first();
    }

    QVariant decisionData = QVariant::fromValue("AGChosen:" + reason + ":" + QString::number(card_id));
    thread->trigger(ChoiceMade, this, player, decisionData);

    return card_id;
}

const Card *Room::askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const QString &reason)
{
    Q_ASSERT(!player->isKongcheng());
    tryPause();
    notifyMoveFocus(player, S_COMMAND_SHOW_CARD);
    const Card *card = NULL;

    AI *ai = player->getAI();
    if (ai)
        card = ai->askForCardShow(requestor, reason);
    else {
        if (player->getHandcardNum() == 1)
            card = player->getHandcards().first();
        else {
            bool success = doRequest(player, S_COMMAND_SHOW_CARD, requestor->getGeneralName(), true);
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            if (success && clientReply.size() > 0 && JsonUtils::isString(clientReply[0]))
                card = Card::Parse(clientReply[0].toString());
            if (card == NULL)
                card = player->getRandomHandCard();
        }
    }

    QVariant decisionData = QVariant::fromValue("cardShow:" + reason + ":_" + card->toString() + "_");
    thread->trigger(ChoiceMade, this, player, decisionData);
    return card;
}

const Card *Room::askForSinglePeach(ServerPlayer *player, ServerPlayer *dying)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_ASK_PEACH);
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);

    const Card *card = NULL;

    AI *ai = player->getAI();
    if (ai)
        card = ai->askForSinglePeach(dying);
    else {
        int peaches = 1 - dying->getHp();
        JsonArray arg;
        arg << dying->objectName();
        arg << peaches;
        bool success = doRequest(player, S_COMMAND_ASK_PEACH, arg, true);
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || clientReply.isEmpty() || !JsonUtils::isString(clientReply[0]))
            return NULL;

        card = Card::Parse(clientReply[0].toString());
    }

    if (card && player->isCardLimited(card, Card::MethodUse))
        card = NULL;
    if (card != NULL)
        card = card->validateInResponse(player);
    else
        return NULL;

    const Card *result = NULL;
    if (card) {
        QVariant decisionData = QVariant::fromValue(QString("peach:%1:%2:%3")
            .arg(dying->objectName())
            .arg(1 - dying->getHp())
            .arg(card->toString()));
        thread->trigger(ChoiceMade, this, player, decisionData);
        result = card;
    } else
        result = askForSinglePeach(player, dying);
    return result;
}

QString Room::askForTriggerOrder(ServerPlayer *player, const QString &reason, SPlayerDataMap &skills, bool optional, const QVariant &data)
{
    tryPause();

    Q_ASSERT(!skills.isEmpty());

    QString answer;
    QStringList all_pairs;
    foreach (const ServerPlayer *p, skills.keys()) {                            //if string not contains the skill_postion info, then it will check for each general
        foreach (const QString &str, skills.value(p)) {                         //sample as: player_name?skill_position:skill_name(*skill_time or ->skill_target)
            if (str.contains("!") && !str.contains("&")) {                      //it means skill will invoke serveral times
                QString skill_name = str.split("!").first();
                const Skill *skill = Sanguosha->getSkill(skill_name);
                if (skill && (p->getHeadActivedSkills().contains(skill) || p->getDeputyActivedSkills().contains(skill))) {
                    if (p->getHeadActivedSkills().contains(skill))
                        all_pairs << QString("%1?%2:%3*%4").arg(p->objectName()).arg("left").arg(str.split("!").first()).arg(str.split("*").last());
                    if (p->getDeputyActivedSkills().contains(skill))
                        all_pairs << QString("%1?%2:%3*%4").arg(p->objectName()).arg("right").arg(str.split("!").first()).arg(str.split("*").last());
                } else
                    all_pairs << QString("%1:%2*%3").arg(p->objectName()).arg(str.split("!").first()).arg(str.split("*").last());   //it means player doesnt has this skill
            } else {
                const Skill *skill = Sanguosha->getSkill(str);
                if (skill && (p->getHeadActivedSkills().contains(skill) || p->getDeputyActivedSkills().contains(skill))) {
                    if (p->getHeadActivedSkills().contains(skill))
                        all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg("left").arg(str);
                    if (p->getDeputyActivedSkills().contains(skill))
                        all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg("right").arg(str);
                } else {                                                        //it means str contains much infomation
                    if (str.contains("->")) {
                        QString skill_name = str.split("->").first();
                        QString targets = str.split("->").last();
                        QString skill_position;
                        if (skill_name.contains("?")) {
                            skill_position = skill_name.split("?").first();
                            skill_name = skill_name.split("?").last();
                        }
                        if (!skill_position.isEmpty())
                            all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg(skill_position).arg(skill_name + "->" + targets);
                        else {
                            const Skill *skill = Sanguosha->getSkill(skill_name);
                            if (skill && (p->getHeadActivedSkills().contains(skill) || p->getDeputyActivedSkills().contains(skill))) {
                                if (p->getHeadActivedSkills().contains(skill))
                                    all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg("left").arg(str);
                                if (p->getDeputyActivedSkills().contains(skill))
                                    all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg("right").arg(str);
                            } else
                                all_pairs << QString("%1:%2").arg(p->objectName()).arg(str);
                        }
                    } else if (str.contains("?"))
                        all_pairs << QString("%1?%2:%3").arg(p->objectName()).arg(str.split("?").first()).arg(str.split("?").last());
                    else                                                                                    //it means player doesnt has this skill. sgs1'songwei also here,
                        all_pairs << QString("%1:%2").arg(p->objectName()).arg(str);                        //cause its very complicated when invoke others skill
                }
            }
        }
    }

    if (all_pairs.length() == 1 && reason != "GameRule:TurnStart") {                                        //do not use this function when only 1 choice except turnstart
        answer = all_pairs.first();
    } else {
        notifyMoveFocus(player, S_COMMAND_TRIGGER_ORDER);

        AI *ai = player->getAI();

        if (ai) {
            //Temporary method to keep compatible with existing AI system
            QStringList all_skills;
            foreach (const QStringList &list, skills)
                foreach (QString str, list)
                    if (!all_skills.contains(str.split("?").last()))
                        all_skills << str.split("?").last();

            if (optional)
                all_skills << "cancel";

            const QString reply = ai->askForChoice(reason, all_skills.join("+"), data);
            if (reply == "cancel") {
                answer = reply;
            } else {
                foreach (const QString &str, all_pairs) {
                    if (str.contains(reply)) {
                        answer = str;
                        break;
                    }
                }
            }
            thread->delay();
        } else {
            JsonArray args;
            //example: "["turn_start", ["sgs1:tiandu", "sgs1:tuntian", "sgs2:slobsb"], true]"

            args << reason;
            args << JsonUtils::toJsonArray(all_pairs);
            args << optional;

            bool success = doRequest(player, S_COMMAND_TRIGGER_ORDER, args, true);
            const QVariant &clientReply = player->getClientReply();
            if (!success || !JsonUtils::isString(clientReply))
                answer = "cancel";
            else
                answer = clientReply.toString();
        }

        if (answer != "cancel" && !all_pairs.contains(answer))
            answer = all_pairs[0];
    }

    return answer;
}

void Room::addPlayerHistory(ServerPlayer *player, const QString &key, int times)
{
    if (player) {
        if (key == ".")
            player->clearHistory();
        else if (times == 0)
            player->clearHistory(key);
        else
            player->addHistory(key, times);
    }

    JsonArray arg;
    arg << key;
    arg << times;

    if (player)
        doNotify(player, S_COMMAND_ADD_HISTORY, arg);
    else
        doBroadcastNotify(S_COMMAND_ADD_HISTORY, arg);
}

void Room::notifyMoveToPile(ServerPlayer *player, const QList<int> &cards, const QString &reason, Player::Place place, bool in, bool is_visible)
{
    CardsMoveStruct move;
    if (in) {
        move = CardsMoveStruct(cards, NULL, player, place, Player::PlaceSpecial,
            CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, player->objectName()));
        move.to_pile_name = "#" + reason;
    } else {
        move = CardsMoveStruct(cards, player, NULL, Player::PlaceSpecial, place,
            CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, player->objectName()));
        move.from_pile_name = "#" + reason;
    }
    QList<CardsMoveStruct> moves;
    moves.append(move);
    QList<ServerPlayer *> _player;
    _player.append(player);
    notifyMoveCards(true, moves, is_visible, _player);
    notifyMoveCards(false, moves, is_visible, _player);
}

QList<int> Room::notifyChooseCards(ServerPlayer *player, const QList<int> &cards, const QString &reason, Player::Place notify_from_place, Player::Place notify_to_place, int max_num, int min_num, const QString &prompt, const QString &pattern)
{
    setPlayerFlag(player, "Global_InTempMoving");
    notifyMoveToPile(player, cards, reason, notify_from_place, true, true);
    QString new_pattern;
    if (pattern.isEmpty())
        new_pattern = ".|.|.|$" + reason;
    else
        new_pattern = pattern;

    QList<int> result = askForExchange(player, reason, max_num, min_num, prompt, "#" + reason, new_pattern);

    notifyMoveToPile(player, cards, reason, notify_to_place, false, false);

    setPlayerFlag(player, "-Global_InTempMoving");
    return result;
}

void Room::setPlayerFlag(ServerPlayer *player, const QString &flag)
{
    if (flag.startsWith("-")) {
        QString set_flag = flag.mid(1);
        if (!player->hasFlag(set_flag)) return;
    }
    player->setFlags(flag);
    broadcastProperty(player, "flags", flag);
}

void Room::setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value)
{
#ifndef QT_NO_DEBUG
    player->event_received = false;
    if (player->thread() == currentThread()) {
#endif
        player->setProperty(property_name, value);
        broadcastProperty(player, property_name);
#ifndef QT_NO_DEBUG
        player->event_received = true;
    } else {
        char *pname = const_cast<char *>(property_name);
        QVariant &v = const_cast<QVariant &>(value);
        qApp->postEvent(player, new ServerPlayerEvent(pname, v), INT_MAX);
    }
#endif

    if (strcmp(property_name, "hp") == 0)
        thread->trigger(HpChanged, this, player);

    if (strcmp(property_name, "maxhp") == 0)
        thread->trigger(MaxHpChanged, this, player);

    if (strcmp(property_name, "chained") == 0)
        thread->trigger(ChainStateChanged, this, player);

    if (strcmp(property_name, "removed") == 0)
        thread->trigger(RemoveStateChanged, this, player);
#ifndef QT_NO_DEBUG
    //wait for main thread
    while (!player->event_received) {
    };
#endif
}

void Room::setPlayerMark(ServerPlayer *player, const QString &mark, int value)
{
    player->setMark(mark, value);

    JsonArray arg;
    arg << player->objectName();
    arg << mark;
    arg << value;
    doBroadcastNotify(S_COMMAND_SET_MARK, arg);
}

void Room::addPlayerMark(ServerPlayer *player, const QString &mark, int add_num)
{
    int value = player->getMark(mark);
    value += add_num;
    setPlayerMark(player, mark, value);
}

void Room::removePlayerMark(ServerPlayer *player, const QString &mark, int remove_num)
{
    int value = player->getMark(mark);
    if (value == 0) return;
    value -= remove_num;
    value = qMax(0, value);
    setPlayerMark(player, mark, value);
}

void Room::setPlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern, bool single_turn)
{
    player->setCardLimitation(limit_list, pattern, single_turn);

    JsonArray arg;
    arg << true;
    arg << limit_list;
    arg << pattern;
    arg << single_turn;
    doNotify(player, S_COMMAND_CARD_LIMITATION, arg);
}

void Room::removePlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern)
{
    player->removeCardLimitation(limit_list, pattern);

    JsonArray arg;
    arg << false;
    arg << limit_list;
    arg << pattern;
    arg << false;
    doNotify(player, S_COMMAND_CARD_LIMITATION, arg);
}

void Room::clearPlayerCardLimitation(ServerPlayer *player, bool single_turn)
{
    player->clearCardLimitation(single_turn);

    JsonArray arg;
    arg << true;
    arg << QVariant();
    arg << QVariant();
    arg << single_turn;
    doNotify(player, S_COMMAND_CARD_LIMITATION, arg);
}

void Room::setPlayerDisableShow(ServerPlayer *player, const QString &flags, const QString &reason)
{
    player->setDisableShow(flags, reason);

    JsonArray arg;
    arg << player->objectName();
    arg << true;
    arg << flags;
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void Room::removePlayerDisableShow(ServerPlayer *player, const QString &reason)
{
    player->removeDisableShow(reason);

    JsonArray arg;
    arg << player->objectName();
    arg << false;
    arg << QVariant();
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void Room::setCardFlag(const Card *card, const QString &flag, ServerPlayer *who)
{
    if (flag.isEmpty()) return;

    card->setFlags(flag);

    if (!card->isVirtualCard())
        setCardFlag(card->getEffectiveId(), flag, who);
}

void Room::setCardFlag(int card_id, const QString &flag, ServerPlayer *who)
{
    if (flag.isEmpty()) return;

    Q_ASSERT(Sanguosha->getCard(card_id) != NULL);
    Sanguosha->getCard(card_id)->setFlags(flag);

    JsonArray arg;
    arg << card_id;
    arg << flag;
    if (who)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

void Room::clearCardFlag(const Card *card, ServerPlayer *who)
{
    card->clearFlags();

    if (!card->isVirtualCard())
        clearCardFlag(card->getEffectiveId(), who);
}

void Room::clearCardFlag(int card_id, ServerPlayer *who)
{
    Q_ASSERT(Sanguosha->getCard(card_id) != NULL);
    Sanguosha->getCard(card_id)->clearFlags();

    JsonArray arg;
    arg << card_id;
    arg << ".";
    if (who)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

ServerPlayer *Room::addSocket(ClientSocket *socket)
{
    ServerPlayer *player = new ServerPlayer(this);
    player->setSocket(socket);
    m_players << player;

    connect(player, &ServerPlayer::disconnected, this, &Room::reportDisconnection);
    connect(player, &ServerPlayer::roomPacketReceived, this, &Room::processClientPacket);
    connect(player, &ServerPlayer::invalidPacketReceived, this, &Room::reportInvalidPacket);

    return player;
}

bool Room::isFull() const
{
    return m_players.length() == player_count;
}

bool Room::isFinished() const
{
    return game_finished;
}

bool Room::canPause(ServerPlayer *player) const
{
    if (!isFull()) return false;
    if (!player || !player->isOwner()) return false;
    foreach (ServerPlayer *p, m_players) {
        if (!p->isAlive() || p->isOwner()) continue;
        if (p->getState() != "robot")
            return false;
    }
    return true;
}

void Room::tryPause()
{
    if (!canPause(getOwner())) return;
    QMutexLocker locker(&m_mutex);
    while (game_paused)
        m_waitCond.wait(locker.mutex());
}

int Room::getLack() const
{
    return player_count - m_players.length();
}

QString Room::getMode() const
{
    return mode;
}

const Scenario *Room::getScenario() const
{
    return scenario;
}

void Room::broadcast(const QByteArray &message, ServerPlayer *except)
{
    foreach (ServerPlayer *player, m_players) {
        if (player != except)
            player->unicast(message);
    }
}

void Room::swapPile()
{
    if (m_discardPile->isEmpty()) {
        // the standoff
        gameOver(".");
    }

    int times = tag.value("SwapPile", 0).toInt();
    setTag("SwapPile", ++times);

    int limit = Config.value("PileSwappingLimitation", 5).toInt() + 1;
    if (limit > 0 && times == limit)
        gameOver(".");

    qShuffle(*m_discardPile);
    foreach (int card_id, *m_discardPile)
        setCardMapping(card_id, NULL, Player::DrawPile);
    *m_drawPile += *m_discardPile;
    *m_discardPile = QList<int>();

    doBroadcastNotify(S_COMMAND_RESET_PILE, QVariant());
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, m_drawPile->length());
}

QList<int> Room::getDiscardPile()
{
    return *m_discardPile;
}

ServerPlayer *Room::findPlayer(const QString &general_name, bool include_dead) const
{
    const QList<ServerPlayer *> &list = include_dead ? m_players : m_alivePlayers;

    if (general_name.contains("+")) {
        QStringList names = general_name.split("+");
        foreach (ServerPlayer *player, list) {
            if (names.contains(player->getGeneralName()) || names.contains(player->objectName()))
                return player;
        }
        return NULL;
    }

    foreach (ServerPlayer *player, list) {
        if (player->getGeneralName() == general_name || player->objectName() == general_name)
            return player;
    }

    return NULL;
}

ServerPlayer *Room::findPlayerbyobjectName(const QString &name, bool include_dead) const
{
    const QList<ServerPlayer *> &list = include_dead ? m_players : m_alivePlayers;

    foreach (ServerPlayer *player, list)
        if (player->objectName() == name)
            return player;

    return NULL;
}

QList<ServerPlayer *>Room::findPlayersBySkillName(const QString &skill_name) const
{
    QList<ServerPlayer *> list;
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player->hasSkill(skill_name))
            list << player;
    }
    return list;
}

ServerPlayer *Room::findPlayerBySkillName(const QString &skill_name) const
{
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player->hasSkill(skill_name))
            return player;
    }
    return NULL;
}

void Room::installEquip(ServerPlayer *player, const QString &equip_name)
{
    if (player == NULL) return;

    int card_id = getCardFromPile(equip_name);
    if (card_id == -1) return;

    moveCardTo(Sanguosha->getCard(card_id), player, Player::PlaceEquip, true);
}

void Room::resetAI(ServerPlayer *player)
{
    AI *smart_ai = player->getSmartAI();
    int index = -1;
    if (smart_ai) {
        index = ais.indexOf(smart_ai);
        ais.removeOne(smart_ai);
        smart_ai->deleteLater();
    }
    AI *new_ai = cloneAI(player);
    player->setAI(new_ai);
    if (index == -1)
        ais.append(new_ai);
    else
        ais.insert(index, new_ai);
}

void Room::doDragonPhoenix(ServerPlayer *player, const QString &general1_name, const QString &general2_name, bool full_state, const QString &kingdom, bool sendLog, const QString &show_flags, bool resetHp)
{
    QStringList names;
    names << player->getActualGeneral1Name() << player->getActualGeneral2Name();
    QStringList names_orig = names;
    names_orig.removeAll("sujiang");
    names_orig.removeAll("sujiangf");
    if (player->isAlive())
        return;
    player->throwAllHandCardsAndEquips();
    if (player->getGeneral())
        player->removeGeneral(true);
    if (player->getGeneral2())
        player->removeGeneral(false);
    setPlayerMark(player, "drank", 0);
    player->throwAllMarks(); // necessary.

    QVariant void_data;
    QList<const TriggerSkill *> game_start;

    QStringList duanchang = player->property("Duanchang").toString().split(",");
    int max_hp = 0;
    if (!general1_name.isEmpty()) {
        handleUsedGeneral(general1_name);
        if (duanchang.contains("head"))
            duanchang.removeAll("head");

        JsonArray arg;
        arg << (int)S_GAME_EVENT_CHANGE_HERO;
        arg << player->objectName();
        arg << general1_name;
        arg << false;
        arg << false;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        foreach (const Skill *skill, Sanguosha->getGeneral(general1_name)->getVisibleSkillList(true, true)) {
            if (skill->inherits("TriggerSkill")) {
                const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(skill);
                if (tr != NULL) {
                    if (tr->getTriggerEvents().contains(GameStart) && !tr->triggerable(GameStart, this, player, void_data).isEmpty())
                        game_start << tr;
                }
            }
            player->addSkill(skill->objectName());
        }

        changePlayerGeneral(player, "anjiang");
        player->setActualGeneral1Name(general1_name);
        notifyProperty(player, player, "actual_general1");
        notifyProperty(player, player, "general", general1_name);

        max_hp += Sanguosha->getGeneral(general1_name)->getMaxHpHead();
        names[0] = general1_name;
        setPlayerProperty(player, "general1_showed", false);
    }
    if (!general2_name.isEmpty()) {
        handleUsedGeneral(general2_name);
        if (duanchang.contains("deputy"))
            duanchang.removeAll("deputy");

        JsonArray arg;
        arg << (int)S_GAME_EVENT_CHANGE_HERO;
        arg << player->objectName();
        arg << general2_name;
        arg << true;
        arg << false;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        foreach (const Skill *skill, Sanguosha->getGeneral(general2_name)->getVisibleSkillList(true, false)) {
            if (skill->inherits("TriggerSkill")) {
                const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(skill);
                if (tr != NULL) {
                    if (tr->getTriggerEvents().contains(GameStart) && !tr->triggerable(GameStart, this, player, void_data).isEmpty())
                        game_start << tr;
                }
            }
            player->addSkill(skill->objectName(), false);
        }

        changePlayerGeneral2(player, "anjiang");
        player->setActualGeneral2Name(general2_name);
        notifyProperty(player, player, "actual_general2");
        notifyProperty(player, player, "general2", general2_name);

        max_hp += Sanguosha->getGeneral(general2_name)->getMaxHpDeputy();
        names[1] = general2_name;
        setPlayerProperty(player, "general2_showed", false);
    }

    setPlayerProperty(player, "Duanchang", duanchang.join(","));

    revivePlayer(player);

    player->setHp(1);
    if (resetHp) {
        if (general1_name.isEmpty() || general2_name.isEmpty())
            max_hp *= 2;
        setPlayerMark(player, "HalfMaxHpLeft", max_hp % 2);
        player->setMaxHp(max_hp / 2);
        broadcastProperty(player, "maxhp");
        player->setHp(player->getMaxHp());
    }
    broadcastProperty(player, "hp");
    setPlayerFlag(player, "Global_DFDebut");

    setTag(player->objectName(), names);

    player->setKingdom(Sanguosha->getGeneral(general1_name)->getKingdom());
    if (show_flags.isEmpty())
        notifyProperty(player, player, "kingdom");
    else
        broadcastProperty(player, "kingdom");
    QString role = HegemonyMode::GetMappedRole(kingdom.isEmpty() ? Sanguosha->getGeneral(general1_name)->getKingdom() : kingdom);
    if (role.isEmpty())
        role = player->getGeneral()->getKingdom();
    player->setRole(role);
    if (show_flags.isEmpty())
        notifyProperty(player, player, "role");
    else
        broadcastProperty(player, "role");

    foreach (const Skill *skill, player->getSkillList(false, true)) {
        if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty()) {
            player->setMark(skill->getLimitMark(), 1);
            JsonArray arg;
            arg << player->objectName();
            arg << skill->getLimitMark();
            arg << 1;
            doNotify(player, S_COMMAND_SET_MARK, arg);
        }
    }

    foreach (const TriggerSkill *skill, game_start) {
        if (skill->cost(GameStart, this, player, void_data, player))
            skill->effect(GameStart, this, player, void_data, player);
    }

    if (full_state) {
        player->setChained(false);
        broadcastProperty(player, "chained");

        player->setFaceUp(true);
        broadcastProperty(player, "faceup");
        if (Sanguosha->getGeneral(general1_name)->isCompanionWith(general2_name))
            setPlayerMark(player, "CompanionEffect", 1);
    }

    if (sendLog) {
        LogMessage l;
        l.type = "#DragonPhoenixRevive" + QString::number(names_orig.length());
        l.from = player;
        l.to << player;
        if (names_orig.length() > 0)
            l.arg = names_orig.first();
        if (names_orig.length() > 1)
            l.arg2 = names_orig.last();

        this->sendLog(l);
    }

    resetAI(player);

    player->setSkillsPreshowed();

    if (show_flags.contains("h"))
        player->showGeneral(true, false, false);
    if (show_flags.contains("d"))
        player->showGeneral(false, false, false);
}

void Room::transformDeputyGeneral(ServerPlayer *player)
{
    if (!player->canTransform()) return;

    QStringList names;
    names << player->getActualGeneral1Name() << player->getActualGeneral2Name();
    QStringList available;
    foreach (QString name, Sanguosha->getLimitedGeneralNames())
        if (!name.startsWith("lord_") && !used_general.contains(name) && Sanguosha->getGeneral(name)->getKingdom() == player->getKingdom())
            available << name;
    if (available.isEmpty()) return;

    qShuffle(available);
    QString general_name = available.first();

    handleUsedGeneral("-" + player->getActualGeneral2Name());
    handleUsedGeneral(general_name);

    player->removeGeneral(false);
    QVariant void_data;
    QList<const TriggerSkill *> game_start;

    foreach (const Skill *skill, Sanguosha->getGeneral(general_name)->getVisibleSkillList(true, false)) {
        if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(skill);
            if (tr != NULL) {
                if (tr->getTriggerEvents().contains(GameStart) && !tr->triggerable(GameStart, this, player, void_data).isEmpty())
                    game_start << tr;
            }
        }
        player->addSkill(skill->objectName(), false);
    }

    changePlayerGeneral2(player, "anjiang");
    player->setActualGeneral2Name(general_name);
    notifyProperty(player, player, "actual_general2");
    notifyProperty(player, player, "general2", general_name);

    names[1] = general_name;
    setPlayerProperty(player, "general2_showed", false);

    setTag(player->objectName(), names);

    foreach (const Skill *skill, Sanguosha->getGeneral(general_name)->getSkillList(true, false)) {
        if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty()) {
            player->setMark(skill->getLimitMark(), 1);
            JsonArray arg;
            arg << player->objectName();
            arg << skill->getLimitMark();
            arg << 1;
            doNotify(player, S_COMMAND_SET_MARK, arg);
        }
    }

    foreach (const TriggerSkill *skill, game_start) {
        if (skill->cost(GameStart, this, player, void_data, player))
            skill->effect(GameStart, this, player, void_data, player);
    }

    if (Sanguosha->getGeneral(names[0])->isCompanionWith(general_name))
        setPlayerMark(player, "CompanionEffect", 1);
    player->showGeneral(false, true, true);
}

void Room::handleUsedGeneral(const QString &general)
{
    bool remove = general.startsWith("-") ? true : false;
    QString general_name = general;
    if (remove) general_name = general_name.remove("-");
    QString main = Sanguosha->getMainGenerals(general_name);
    if (remove)
        used_general.removeAll(main);
    else
        used_general.append(main);
    foreach (QString sub, Sanguosha->getConvertGenerals(main)) {
        if (remove)
            used_general.removeAll(sub);
        else
            used_general.append(sub);
    }
}

lua_State *Room::getLuaState() const
{
    return L;
}

void Room::setFixedDistance(Player *from, const Player *to, int distance)
{
    from->setFixedDistance(to, distance);

    JsonArray arg;
    arg << from->objectName();
    arg << to->objectName();
    arg << distance;
    doBroadcastNotify(S_COMMAND_FIXED_DISTANCE, arg);
}

const ProhibitSkill *Room::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    return Sanguosha->isProhibited(from, to, card, others);
}

int Room::drawCard()
{
    thread->trigger(FetchDrawPileCard, this, NULL);
    if (m_drawPile->isEmpty())
        swapPile();
    return m_drawPile->takeFirst();
}

void Room::prepareForStart()
{
    if (scenario) {
        if (scenario->isRandomSeat() && Config.RandomSeat && mode != "custom_scenario")
            qShuffle(m_players);
        //The process of the followings is moved to Room::run.
        //QStringList generals, generals2, kingdoms;
        //scenario->assign(generals, generals2, kingdoms, this);

        //for (int i = 0; i < m_players.length(); i++) {
        //    ServerPlayer *player = m_players[i];
        //    if (generals.size() > i && !generals[i].isNull() && !generals2[i].isNull()) {
        //        QStringList names;
        //        names.append(generals[i]);
        //        names.append(generals2[i]);
        //        this->setTag(player->objectName(), QVariant::fromValue(names));
        //        player->setGeneralName("anjiang");
        //        player->setActualGeneral1Name(generals[i]);
        //        notifyProperty(player, player, "actual_general1");
        //        foreach(ServerPlayer *p, getOtherPlayers(player))
        //            notifyProperty(p, player, "general");
        //        notifyProperty(player, player, "general", generals[i]);
        //        if (generals[i] != generals2[i] || mode == "custom_scenario") {
        //            player->setGeneral2Name("anjiang");
        //            player->setActualGeneral2Name(generals2[i]);
        //            notifyProperty(player, player, "actual_general2");
        //            foreach(ServerPlayer *p, getOtherPlayers(player))
        //                notifyProperty(p, player, "general2");
        //            notifyProperty(player, player, "general2", generals2[i]);
        //        }
        //        setPlayerProperty(player, "kingdom", kingdoms[i]);
        //        setPlayerProperty(player, "role", HegemonyMode::GetMappedRole(kingdoms[i]));
        //        setPlayerProperty(player, "scenario_role_shown", true);
        //    }
        //}
    } else {
        if (Config.RandomSeat)
            qShuffle(m_players);
        assignRoles();
    }

    adjustSeats();
}

void Room::reportDisconnection()
{
    ServerPlayer *player = qobject_cast<ServerPlayer *>(sender());
    if (player == NULL) return;

    // send disconnection message to server log
    emit room_message(player->reportHeader() + tr("disconnected"));

    // all the circumstances below should set its socket to NULL
    player->setSocket(NULL);

    // the 3 kinds of circumstances
    if (player->objectName().isEmpty()) {
        //Just connected, with no object name : just remove it from player list
        player->setParent(NULL);
        m_players.removeOne(player);
    } else if (player->getRole().isEmpty()) {
        // Connected, with an object name : remove it, tell other clients and decrease signup_count
        if (m_players.length() < player_count) {
            player->setParent(NULL);
            m_players.removeOne(player);

            if (player->getState() != "robot") {
                QString screen_name = player->screenName();
                QString leaveStr = tr("<font color=#000000>Player <b>%1</b> left the game</font>").arg(screen_name);
                speakCommand(player, leaveStr);
            }

            doBroadcastNotify(S_COMMAND_REMOVE_PLAYER, player->objectName());
        }
    } else {
        // Game is started, do not remove it just set its state as offline
        if (player->m_isWaitingReply)
            player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        setPlayerProperty(player, "state", "offline");

        bool someone_is_online = false;
        foreach (ServerPlayer *player, m_players) {
            if (player->getState() == "online" || player->getState() == "trust") {
                someone_is_online = true;
                break;
            }
        }

        if (!someone_is_online) {
            game_finished = true;
            emit game_over(QString());
            return;
        }
    }

    if (player->isOwner()) {
        player->setOwner(false);
        broadcastProperty(player, "owner");
        foreach (ServerPlayer *p, m_players) {
            if (p->getState() == "online") {
                p->setOwner(true);
                broadcastProperty(p, "owner");
                break;
            }
        }
    }

    if (player->parent() == NULL)
        player->deleteLater();
}

void Room::trustCommand(ServerPlayer *player, const QVariant &)
{
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    if (player->isOnline()) {
        player->setState("trust");
        if (player->m_isWaitingReply) {
            player->releaseLock(ServerPlayer::SEMA_MUTEX);
            player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }
    } else
        player->setState("online");

    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    broadcastProperty(player, "state");
}

void Room::pauseCommand(ServerPlayer *player, const QVariant &arg)
{
    if (!canPause(player)) return;
    bool pause = arg.toBool();
    QMutexLocker locker(&m_mutex);

    if (game_paused != pause) {
        JsonArray arg;
        arg << (int)S_GAME_EVENT_PAUSE;
        arg << pause;
        doNotify(player, S_COMMAND_LOG_EVENT, arg);

        game_paused = pause;
        if (!game_paused)
            m_waitCond.wakeAll();
    }
}

void Room::processRequestCheat(ServerPlayer *player, const QVariant &arg)
{
    if (!Config.EnableCheat || !arg.canConvert<JsonArray>()) return;

    JsonArray args = arg.value<JsonArray>();
    if (!JsonUtils::isNumber(args[0])) return;

    //@todo: synchronize this
    player->m_cheatArgs = arg;
    player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
}

bool Room::makeSurrender(ServerPlayer *initiator)
{
    // broadcast polling request
    QList<ServerPlayer *> playersAlive;
    foreach (ServerPlayer *player, m_players) {
        if (player != initiator && player->isAlive() && player->isOnline()) {
            player->m_commandArgs = initiator->getGeneral()->objectName();
            playersAlive << player;
        }
    }
    doBroadcastRequest(playersAlive, S_COMMAND_SURRENDER);

    int give_up = 1;

    // collect polls
    foreach (ServerPlayer *player, playersAlive) {
        bool result = false;
        if (!player->m_isClientResponseReady || !JsonUtils::isBool(player->getClientReply()))
            result = !player->isOnline();
        else
            result = player->getClientReply().toBool();

        if (result) give_up++;
    }

    if (give_up > (playersAlive.length() + 1) / 2)
        gameOver(".");

    m_surrenderRequestReceived = false;

    initiator->setFlags("Global_ForbidSurrender");
    doNotify(initiator, S_COMMAND_ENABLE_SURRENDER, QVariant(false));
    return true;
}

void Room::processRequestSurrender(ServerPlayer *player, const QVariant &)
{
    //@todo: Strictly speaking, the client must be in the PLAY phase
    //@todo: return false for 3v3 and 1v1!!!
    if (player == NULL || !player->m_isWaitingReply)
        return;
    if (!_m_isFirstSurrenderRequest
        && _m_timeSinceLastSurrenderRequest.elapsed() <= Config.S_SURRENDER_REQUEST_MIN_INTERVAL)
        return; //@todo: warn client here after new protocol has been enacted on the warn request

    _m_isFirstSurrenderRequest = false;
    _m_timeSinceLastSurrenderRequest.restart();
    m_surrenderRequestReceived = true;
    player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
}

void Room::processRequestPreshow(ServerPlayer *player, const QVariant &arg)
{
    if (player == NULL)
        return;

    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 3 || !JsonUtils::isString(args[0]) || !JsonUtils::isBool(args[1]) || !JsonUtils::isBool(args[2])) return;
    player->acquireLock(ServerPlayer::SEMA_MUTEX);

    const QString skill_name = args[0].toString();
    const bool isPreshowed = args[1].toBool();
    const bool head = args[2].toBool();
    player->setSkillPreshowed(skill_name, isPreshowed, head);

    player->releaseLock(ServerPlayer::SEMA_MUTEX);
}

void Room::processClientPacket(const QSanProtocol::Packet &packet)
{
    ServerPlayer *player = qobject_cast<ServerPlayer *>(sender());
    if (packet.getPacketType() == S_TYPE_REPLY) {
        if (player == NULL) return;
        processClientReply(player, packet);
    } else if (packet.getPacketType() == S_TYPE_REQUEST) {
        Callback callback = interactions[packet.getCommandType()];
        if (!callback) return;
        (this->*callback)(player, packet.getMessageBody());
    } else if (packet.getPacketType() == S_TYPE_NOTIFICATION) {
        Callback callback = callbacks[packet.getCommandType()];
        if (!callback) return;
        (this->*callback)(player, packet.getMessageBody());
    }
}

void Room::reportInvalidPacket(const QByteArray &message)
{
    ServerPlayer *player = qobject_cast<ServerPlayer *>(sender());
    if (player == NULL) return;

    if (game_finished) {
        if (player->isOnline())
            player->notify(S_COMMAND_WARN, "GAME_OVER");
        return;
    }

    emit room_message(tr("%1: %2 is not invokable").arg(player->reportHeader()).arg(QString::fromUtf8(message)));
}

void Room::addRobotCommand(ServerPlayer *player, const QVariant &)
{
    if (Config.ForbidAddingRobot || isFull()) return;
    if (player && !player->isOwner()) return;

    static QStringList names;
    if (names.isEmpty()) {
        names = GetConfigFromLuaState(Sanguosha->getLuaState(), "robot_names").toStringList();
        qShuffle(names);
    }

    int n = 0;
    foreach (ServerPlayer *player, m_players) {
        if (player->getState() == "robot") {
            QString screenname = player->screenName();
            if (names.contains(screenname))
                names.removeOne(screenname);
            else
                n++;
        }
    }

    ServerPlayer *robot = new ServerPlayer(this);
    robot->setState("robot");

    m_players << robot;

    const QString robot_name = names.isEmpty() ? tr("Computer %1").arg(QChar('A' + n)) : names.first();
    const QString robot_avatar = Sanguosha->getRandomGeneralName();
    signup(robot, robot_name, robot_avatar, true);

    QString greeting;
    QDate date = QDate::currentDate();
    if (date.month() == 11 && date.day() == 30)
        greeting = tr("Happy Birthday to Rara!");
    else
        greeting = tr("Hello, I'm a robot");
    speakCommand(robot, greeting);

    broadcastProperty(robot, "state");
}

void Room::fillRobotsCommand(ServerPlayer *player, const QVariant &)
{
    int left = player_count - m_players.length();
    for (int i = 0; i < left; i++) {
        addRobotCommand(player);
    }
}

void Room::mirrorGuanxingStepCommand(ServerPlayer *player, const QVariant &arg)
{
    doBroadcastNotify(S_COMMAND_MIRROR_GUANXING_STEP, arg, player);
}

void Room::mirrorMoveCardsStepCommand(ServerPlayer *player, const QVariant &arg)
{
    doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, arg, player);
}

void Room::onPindianReply(ServerPlayer *, const QVariant &arg)
{
    doBroadcastNotify(S_COMMAND_PINDIAN, arg);
}

void Room::changeSkinCommand(ServerPlayer *player, const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 2) return;
    int skin_id = args.at(0).toInt();
    bool is_head = args.at(1).toBool();

    QString propertyName;
    if (is_head) {
        player->setHeadSkinId(skin_id);

        if (!player->hasShownGeneral1())
            return;

        propertyName = "head_skin_id";
    } else {
        player->setDeputySkinId(skin_id);

        if (!player->hasShownGeneral2())
            return;

        propertyName = "deputy_skin_id";
    }

    foreach (ServerPlayer *target, m_players) {
        if (player == target) continue;
        notifyProperty(target, player, propertyName.toLatin1().constData());
    }
}

ServerPlayer *Room::getOwner() const
{
    foreach (ServerPlayer *player, m_players) {
        if (player->isOwner())
            return player;
    }

    return NULL;
}

void Room::toggleReadyCommand(ServerPlayer *, const QVariant &)
{
    if (!game_started && isFull())
        start();
}

void Room::signup(ServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot)
{
    player->setObjectName(generatePlayerName());
    player->setProperty("avatar", avatar);
    player->setScreenName(screen_name);

    if (!is_robot) {
        notifyProperty(player, player, "objectName");

        ServerPlayer *owner = getOwner();
        if (owner == NULL) {
            player->setOwner(true);
            notifyProperty(player, player, "owner");
        }
    }

    // introduce the new joined player to existing players except himself
    player->introduceTo(NULL);

    if (!is_robot) {
        QString greetingStr = tr("<font color=#EEB422>Player <b>%1</b> joined the game</font>").arg(screen_name);
        speakCommand(player, greetingStr);
        player->startNetworkDelayTest();

        // introduce all existing player to the new joined
        foreach (ServerPlayer *p, m_players) {
            if (p != player)
                p->introduceTo(player);
        }
    } else
        toggleReadyCommand(player, QVariant());
}

void Room::assignGeneralsForPlayers(const QList<ServerPlayer *> &to_assign)
{
    QSet<QString> existed;
    foreach (ServerPlayer *player, m_players) {
        if (player->getGeneral())
            existed << player->getGeneralName();
        if (player->getGeneral2())
            existed << player->getGeneral2Name();
    }

    const int max_choice = Config.value("HegemonyMaxChoice", 7).toInt();
    const int total = Sanguosha->getGeneralCount();
    const int max_available = (total - existed.size()) / to_assign.length();
    const int choice_count = qMin(max_choice, max_available);

    QStringList choices = Sanguosha->getRandomGenerals(total - existed.size(), existed);

    foreach (ServerPlayer *player, to_assign) {
        player->clearSelected();

        for (int i = 0; i < choice_count; i++) {
            QString choice = player->findReasonable(choices, true);
            if (choice.isEmpty()) break;
            player->addToSelected(choice);
            choices.removeOne(choice);
        }
    }
}

void Room::chooseGenerals(QList<ServerPlayer *> &to_assign, bool has_assign, bool is_scenario)
{
    if (!has_assign)
        assignGeneralsForPlayers(to_assign);

    foreach (ServerPlayer *player, to_assign) {
        JsonArray args;
        args << JsonUtils::toJsonArray(player->getSelected());
        args << false;
        args << !is_scenario;
        player->m_commandArgs = args;
    }

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
    foreach (ServerPlayer *player, to_assign) {
        if (player->getGeneral() != NULL) continue;
        const QVariant &generalName = player->getClientReply();
        if (!player->m_isClientResponseReady || !JsonUtils::isString(generalName)) {
            QStringList default_generals = _chooseDefaultGenerals(player);
            _setPlayerGeneral(player, default_generals.first(), true);
            _setPlayerGeneral(player, default_generals.last(), false);
        } else {
            QStringList generals = generalName.toString().split("+");
            if (generals.length() != 2 || !_setPlayerGeneral(player, generals.first(), true)
                || !_setPlayerGeneral(player, generals.last(), false)) {
                QStringList default_generals = _chooseDefaultGenerals(player);
                _setPlayerGeneral(player, default_generals.first(), true);
                _setPlayerGeneral(player, default_generals.last(), false);
            }
        }
    }

    m_generalSelector->resetValues();

    if (is_scenario)
        return; // the following code need writing in Scenario::assien.
    //I consider writing a function to wrap it.

    foreach (ServerPlayer *player, m_players) {
        QStringList names;
        if (player->getGeneral()) {
            QString name = player->getGeneralName();
            QString role = HegemonyMode::GetMappedRole(player->getGeneral()->getKingdom());
            if (role.isEmpty())
                role = player->getGeneral()->getKingdom();
            names.append(name);
            player->setActualGeneral1Name(name);
            player->setRole(role);
            player->setGeneralName("anjiang");
            notifyProperty(player, player, "actual_general1");
            foreach (ServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general");
            notifyProperty(player, player, "general", name);
            notifyProperty(player, player, "role", role);
        }
        if (player->getGeneral2()) {
            QString name = player->getGeneral2Name();
            names.append(name);
            player->setActualGeneral2Name(name);
            player->setGeneral2Name("anjiang");
            notifyProperty(player, player, "actual_general2");
            foreach (ServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general2");
            notifyProperty(player, player, "general2", name);
        }
        this->setTag(player->objectName(), QVariant::fromValue(names));
    }
}

void Room::run()
{
    // initialize random seed for later use
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    Config.AIDelay = Config.OriginAIDelay;

    foreach (ServerPlayer *player, m_players) {
        //Ensure that the game starts with all player's mutex locked
        player->drainAllLocks();
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }

    prepareForStart();

    bool using_countdown = true;
    if (_virtual || !property("to_test").toString().isEmpty())
        using_countdown = false;

#ifndef QT_NO_DEBUG
    using_countdown = false;
#endif

    if (using_countdown) {
        for (int i = Config.CountDownSeconds; i >= 0; i--) {
            doBroadcastNotify(S_COMMAND_START_IN_X_SECONDS, QVariant(i));
            sleep(1);
        }
    } else
        doBroadcastNotify(S_COMMAND_START_IN_X_SECONDS, QVariant(0));


    if (scenario && !scenario->generalSelection()) {
        QStringList generals, generals2, kingdoms;
        scenario->assign(generals, generals2, kingdoms, this);
        for (int i = 0; i < m_players.length(); i++) {
            ServerPlayer *player = m_players[i];
            if (generals.size() > i && !generals[i].isNull() && !generals2[i].isNull()) {
                QStringList names;
                names.append(generals[i]);
                names.append(generals2[i]);
                this->setTag(player->objectName(), QVariant::fromValue(names));
                player->setGeneralName("anjiang");
                player->setActualGeneral1Name(generals[i]);
                notifyProperty(player, player, "actual_general1");
                foreach (ServerPlayer *p, getOtherPlayers(player))
                    notifyProperty(p, player, "general");
                notifyProperty(player, player, "general", generals[i]);
                if (generals[i] != generals2[i] || mode == "custom_scenario") {
                    player->setGeneral2Name("anjiang");
                    player->setActualGeneral2Name(generals2[i]);
                    notifyProperty(player, player, "actual_general2");
                    foreach (ServerPlayer *p, getOtherPlayers(player))
                        notifyProperty(p, player, "general2");
                    notifyProperty(player, player, "general2", generals2[i]);
                }
                QString role = HegemonyMode::GetMappedRole(kingdoms[i]);
                if (role.isEmpty())
                    role = player->getGeneral()->getKingdom();
                if (scenario->exposeRoles()) {
                    setPlayerProperty(player, "kingdom", kingdoms[i]);
                    setPlayerProperty(player, "role", role);
                } else {
                    //                    foreach (ServerPlayer *p,getOtherPlayers(player))
                    //                        notifyProperty(p,player,"kingdom","god");
                    notifyProperty(player, player, "role", role);
                    player->setRole(role);
                }
                setPlayerProperty(player, "scenario_role_shown", scenario->exposeRoles());
            }
        }
        startGame();
    } else {
        chooseGenerals(m_players);
        startGame();
    }
}

void Room::assignRoles()
{
    int n = m_players.count();

    QStringList roles = Sanguosha->getRoleList(mode);
    qShuffle(roles);

    for (int i = 0; i < n; i++) {
        ServerPlayer *player = m_players[i];
        QString role = roles.at(i);

        player->setRole(role);

        notifyProperty(player, player, "role");
    }
}

void Room::swapSeat(ServerPlayer *a, ServerPlayer *b)
{
    int seat1 = m_players.indexOf(a);
    int seat2 = m_players.indexOf(b);

    m_players.swap(seat1, seat2);

    JsonArray player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();
    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, player_circle);

    m_alivePlayers.clear();
    for (int i = 0; i < m_players.length(); i++) {
        ServerPlayer *player = m_players.at(i);
        if (player->isAlive()) {
            m_alivePlayers << player;
            player->setSeat(m_alivePlayers.length());
        } else {
            player->setSeat(0);
        }

        broadcastProperty(player, "seat");
        player->setNext(m_players.at((i + 1) % m_players.length()));
        broadcastProperty(player, "next");
    }
}

void Room::adjustSeats()
{
    QList<ServerPlayer *> players;
    int i = 0;
    for (i = 0; i < m_players.length(); i++) {
        if (m_players.at(i)->getRoleEnum() == Player::Lord)
            break;
    }
    for (int j = i; j < m_players.length(); j++)
        players << m_players.at(j);
    for (int j = 0; j < i; j++)
        players << m_players.at(j);

    m_players = players;

    for (int i = 0; i < m_players.length(); i++)
        m_players.at(i)->setSeat(i + 1);

    // tell the players about the seat, and the first is always the lord
    JsonArray player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();

    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, player_circle);
}

int Room::getCardFromPile(const QString &card_pattern)
{
    if (m_drawPile->isEmpty())
        swapPile();

    if (card_pattern.startsWith("@")) {
        if (card_pattern == "@duanliang") {
            foreach (int card_id, *m_drawPile) {
                const Card *card = Sanguosha->getCard(card_id);
                if (card->isBlack() && (card->isKindOf("BasicCard") || card->isKindOf("EquipCard")))
                    return card_id;
            }
        }
    } else {
        QString card_name = card_pattern;
        foreach (int card_id, *m_drawPile) {
            const Card *card = Sanguosha->getCard(card_id);
            if (card->objectName() == card_name)
                return card_id;
        }
    }

    return -1;
}

QStringList Room::_chooseDefaultGenerals(ServerPlayer *player) const
{
    Q_ASSERT(!player->getSelected().isEmpty());
    QStringList generals = m_generalSelector->selectGenerals(player, player->getSelected());

    Q_ASSERT(!generals.isEmpty());
    return generals;
}

bool Room::_setPlayerGeneral(ServerPlayer *player, const QString &generalName, bool isFirst)
{
    const General *general = Sanguosha->getGeneral(generalName);
    if (general == NULL)
        return false;
    else if (!Config.FreeChoose && !player->getSelected().contains(Sanguosha->getMainGenerals(generalName)))
        return false;

    if (isFirst) {
        player->setGeneralName(general->objectName());
        notifyProperty(player, player, "general");
    } else {
        player->setGeneral2Name(general->objectName());
        notifyProperty(player, player, "general2");
    }
    return true;
}

void Room::speakCommand(ServerPlayer *player, const QVariant &message)
{
    if (player && Config.EnableCheat) {
        QString sentence = message.toString();
        if (sentence.at(0) == '.') {
            int split = sentence.indexOf('=');
            QString cmd = split == -1 ? sentence : sentence.left(split);
            Callback cheatFunc = cheatCommands[cmd];
            if (cheatFunc) {
                JsonArray body;
                body << player->objectName();
                body << message;
                player->notify(S_COMMAND_SPEAK, body);

                (this->*cheatFunc)(player, split != -1 ? sentence.mid(split + 1) : QVariant());
                return;
            }
        }
    }

    JsonArray body;
    body << player->objectName();
    body << message;

    Packet packet(S_SRC_CLIENT | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SPEAK);
    packet.setMessageBody(body);
    broadcast(&packet);
}

void Room::broadcastRoles(ServerPlayer *, const QVariant &target)
{
    if (target.isNull()) {
        foreach (ServerPlayer *p, m_alivePlayers)
            broadcastProperty(p, "role", p->getRole());
    } else {
        QString name = target.toString();
        foreach (ServerPlayer *p, m_alivePlayers) {
            if (p->objectName() == name || p->getGeneralName() == name) {
                broadcastProperty(p, "role", p->getRole());
                break;
            }
        }
    }
}

void Room::showHandCards(ServerPlayer *player, const QVariant &target)
{
    if (target.isNull()) {
        JsonArray splitLine;
        splitLine << player->objectName();
        splitLine << "----------";

        player->notify(S_COMMAND_SPEAK, splitLine);
        foreach (ServerPlayer *p, m_alivePlayers) {
            if (!p->isKongcheng()) {
                QStringList handcards;
                foreach (const Card *card, p->getHandcards())
                    handcards << QString("<b>%1</b>")
                    .arg(Sanguosha->getEngineCard(card->getId())->getLogName());
                QString hand = handcards.join(", ");

                JsonArray arg;
                arg << p->objectName();
                arg << hand;

                player->notify(S_COMMAND_SPEAK, arg);
            }
        }
        player->notify(S_COMMAND_SPEAK, splitLine);
    } else {
        QString name = target.toString();
        foreach (ServerPlayer *p, m_alivePlayers) {
            if (p->objectName() == name || p->getGeneralName() == name) {
                if (!p->isKongcheng()) {
                    QStringList handcards;
                    foreach (const Card *card, p->getHandcards())
                        handcards << QString("<b>%1</b>").arg(Sanguosha->getEngineCard(card->getId())->getLogName());

                    QString hand = handcards.join(", ");

                    JsonArray arg;
                    arg << p->objectName();
                    arg << hand;
                    player->notify(S_COMMAND_SPEAK, arg);
                }
                break;
            }
        }
    }
}

void Room::showPrivatePile(ServerPlayer *player, const QVariant &args)
{
    QStringList arg = args.toString().split(":");
    if (arg.length() != 2)
        return;

    QString name = arg.first();
    QString pile_name = arg.last();
    foreach (ServerPlayer *p, m_alivePlayers) {
        if (p->objectName() == name || p->getGeneralName() == name) {
            if (!p->getPile(pile_name).isEmpty()) {
                QStringList pile_cards;
                foreach (int id, p->getPile(pile_name))
                    pile_cards << QString("<b>%1</b>").arg(Sanguosha->getEngineCard(id)->getLogName());
                QString pile = pile_cards.join(", ");

                JsonArray arg;
                arg << p->objectName();
                arg << pile;
                player->notify(S_COMMAND_SPEAK, arg);
            }
            break;
        }
    }
}

void Room::setAIDelay(ServerPlayer *, const QVariant &delay)
{
    bool ok = false;
    int miliseconds = delay.toInt(&ok);
    if (ok) {
        Config.AIDelay = Config.OriginAIDelay = miliseconds;
        Config.setValue("OriginAIDelay", miliseconds);
    }
}

void Room::setGameMode(ServerPlayer *, const QVariant &mode)
{
    QString name = mode.toString();
    setTag("NextGameMode", name);
}

void Room::pause(ServerPlayer *player, const QVariant &)
{
    pauseCommand(player, true);
}

void Room::resume(ServerPlayer *player, const QVariant &)
{
    pauseCommand(player, false);
}

void Room::processClientReply(ServerPlayer *player, const Packet &packet)
{
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    bool success = false;
    if (player == NULL)
        emit room_message(tr("Unable to parse player"));
    else if (!player->m_isWaitingReply || player->m_isClientResponseReady)
        emit room_message(tr("Server is not waiting for reply from %1").arg(player->objectName()));
    else if (packet.getCommandType() != player->m_expectedReplyCommand)
        emit room_message(tr("Reply command should be %1 instead of %2")
        .arg(player->m_expectedReplyCommand).arg(packet.getCommandType()));
    else if (packet.localSerial != player->m_expectedReplySerial)
        emit room_message(tr("Reply serial should be %1 instead of %2")
        .arg(player->m_expectedReplySerial).arg(packet.localSerial));
    else
        success = true;

    if (!success) {
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
        return;
    } else {
        _m_semRoomMutex.acquire();
        if (_m_raceStarted) {
            player->setClientReply(packet.getMessageBody());
            player->m_isClientResponseReady = true;
            // Warning: the statement below must be the last one before releasing the lock!!!
            // Any statement after this statement will totally compromise the synchronization
            // because getRaceResult will then be able to acquire the lock, reading a non-null
            // raceWinner and proceed with partial data. The current implementation is based on
            // the assumption that the following line is ATOMIC!!!
            // @todo: Find a Qt atomic semantic or use _asm to ensure the following line is atomic
            // on a multi-core machine. This is the core to the whole synchornization mechanism for
            // broadcastRaceRequest.
            _m_raceWinner = player;
            // the _m_semRoomMutex.release() signal is in getRaceResult();
            _m_semRaceRequest.release();
        } else {
            _m_semRoomMutex.release();
            player->setClientReply(packet.getMessageBody());
            player->m_isClientResponseReady = true;
            player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }

        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }
}

bool Room::useCard(const CardUseStruct &use, bool add_history)
{
    CardUseStruct card_use = use;
    card_use.m_addHistory = false;
    card_use.m_isHandcard = true;
    const Card *card = card_use.card;
    QList<int> ids;
    if (!card->isVirtualCard()) ids << card->getEffectiveId();
    else ids = card->getSubcards();
    if (!ids.isEmpty()) {
        foreach (int id, ids) {
            if (getCardOwner(id) != use.from || getCardPlace(id) != Player::PlaceHand) {
                card_use.m_isHandcard = false;
                break;
            }
        }
    } else {
        card_use.m_isHandcard = false;
    }

    if (card_use.from->isCardLimited(card, card->getHandlingMethod())
        && (!card->canRecast() || card_use.from->isCardLimited(card, Card::MethodRecast)))
        return false;

    QString key;
    if (card->inherits("LuaSkillCard"))
        key = "#" + card->objectName();
    else
        key = card->getClassName();

    int slash_count = card_use.from->getSlashCount();
    bool showTMskill = false;
    foreach (const Skill *skill, card_use.from->getSkillList(false, true)) {
        if (skill->inherits("TargetModSkill")) {
            const TargetModSkill *tm = qobject_cast<const TargetModSkill *>(skill);
            if (tm->getResidueNum(card_use.from, card) > 500 && card_use.from->hasShownSkill(skill)) showTMskill = true;
        }
    }
    bool slash_not_record = key.contains("Slash") && slash_count > 0 && (card_use.from->hasWeapon("Crossbow") || showTMskill);
    card = card_use.card->validate(card_use);
    if (card == NULL)
        return false;

    if (card_use.from->getPhase() == Player::Play && add_history) {
        if (!slash_not_record) {
            card_use.m_addHistory = true;
            addPlayerHistory(card_use.from, key);
            if (!card->getSkillName().isEmpty()) {
                QString name = card->getSkillName();
                addPlayerHistory(card_use.from, "ViewAsSkill_" + name + "Card");
            }
        }
        addPlayerHistory(NULL, "pushPile");
    }

    try {
        if (card_use.card->getRealCard() == card) {
            QStringList tarmod_detect;
            while (!((tarmod_detect = card_use.card->checkTargetModSkillShow(card_use)).isEmpty())) {
                QString to_show = askForChoice(card_use.from, "tarmod_show", tarmod_detect.join("+"), QVariant::fromValue(card_use));
                card_use.from->showSkill(to_show);
            }

            if (card->isKindOf("DelayedTrick") && card->isVirtualCard() && card->subcardsLength() == 1) {
                Card *trick = Sanguosha->cloneCard(card);
                Q_ASSERT(trick != NULL);
                WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getSubcards().first());
                wrapped->takeOver(trick);
                broadcastUpdateCard(getPlayers(), wrapped->getId(), wrapped);
                card_use.card = wrapped;
                wrapped->setShowSkill(card->showSkill());
                wrapped->onUse(this, card_use);
                return true;
            }
            if (card_use.card->isKindOf("Slash") && add_history && slash_count > 0)
                card_use.from->setFlags("Global_MoreSlashInOneTurn");
            if (!card_use.card->isVirtualCard()) {
                WrappedCard *wrapped = Sanguosha->getWrappedCard(card_use.card->getEffectiveId());
                if (wrapped->isModified())
                    broadcastUpdateCard(getPlayers(), card_use.card->getEffectiveId(), wrapped);
                else
                    broadcastResetCard(getPlayers(), card_use.card->getEffectiveId());
            }

            card_use.card->onUse(this, card_use);
        } else if (card) {
            CardUseStruct new_use = card_use;
            new_use.card = card;
            useCard(new_use);
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == StageChange || triggerEvent == TurnBroken) {
            QList<int> table_cardids = getCardIdsOnTable(card_use.card);
            if (!table_cardids.isEmpty()) {
                DummyCard dummy(table_cardids);
                CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
                if (card_use.to.size() == 1) reason.m_targetId = card_use.to.first()->objectName();
                moveCardTo(&dummy, card_use.from, NULL, Player::DiscardPile, reason, true);
            }
            QVariant data = QVariant::fromValue(card_use);
            card_use.from->setFlags("Global_ProcessBroken");
            thread->trigger(CardFinished, this, card_use.from, data);
            card_use.from->setFlags("-Global_ProcessBroken");

            foreach (ServerPlayer *p, m_alivePlayers) {
                p->tag.remove("Qinggang");

                foreach (const QString &flag, p->getFlagList()) {
                    if (flag == "Global_GongxinOperator")
                        p->setFlags("-" + flag);
                    else if (flag.endsWith("_InTempMoving"))
                        setPlayerFlag(p, "-" + flag);
                }
            }

            foreach (int id, Sanguosha->getRandomCards()) {
                if (getCardPlace(id) == Player::PlaceTable || getCardPlace(id) == Player::PlaceJudge)
                    moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, true);
                if (Sanguosha->getCard(id)->hasFlag("using"))
                    setCardFlag(id, "-using");
            }
        }
        throw triggerEvent;
    }
    /*
    if (card->isVirtualCard()){
    delete card;
    }
    */ //temporily revert this because it cause sudden exits
    return true;
}

void Room::loseHp(ServerPlayer *victim, int lose)
{
    Q_ASSERT(lose > 0);
    if (lose <= 0)
        return;

    if (victim->isDead())
        return;
    QVariant data = lose;
    if (thread->trigger(PreHpLost, this, victim, data))
        return;

    lose = data.toInt();

    Q_ASSERT(lose >= 0);
    if (lose <= 0)
        return;

    LogMessage log;
    log.type = "#LoseHp";
    log.from = victim;
    log.arg = QString::number(lose);
    sendLog(log);

    setPlayerProperty(victim, "hp", victim->getHp() - lose);

    JsonArray arg;
    arg << victim->objectName();
    arg << -lose;
    arg << -1;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    thread->trigger(PostHpReduced, this, victim, data);
    thread->trigger(HpLost, this, victim, data);
}

void Room::loseMaxHp(ServerPlayer *victim, int lose)
{
    Q_ASSERT(lose > 0);
    if (lose <= 0)
        return;

    int hp_1 = victim->getHp();
    victim->setMaxHp(qMax(victim->getMaxHp() - lose, 0));
    int hp_2 = victim->getHp();

    broadcastProperty(victim, "maxhp");
    broadcastProperty(victim, "hp");

    LogMessage log;
    log.type = "#LoseMaxHp";
    log.from = victim;
    log.arg = QString::number(lose);
    sendLog(log);

    JsonArray arg;
    arg << victim->objectName();
    arg << -lose;
    doBroadcastNotify(S_COMMAND_CHANGE_MAXHP, arg);

    LogMessage log2;
    log2.type = "#GetHp";
    log2.from = victim;
    log2.arg = QString::number(victim->getHp());
    log2.arg2 = QString::number(victim->getMaxHp());
    sendLog(log2);

    if (victim->getMaxHp() == 0)
        killPlayer(victim);
    else {
        thread->trigger(MaxHpChanged, this, victim);
        if (hp_1 > hp_2) {
            QVariant data = QVariant::fromValue(hp_1 - hp_2);
            thread->trigger(PostHpReduced, this, victim, data);
        }
    }
}

void Room::applyDamage(ServerPlayer *victim, const DamageStruct &damage)
{
    int new_hp = victim->getHp() - damage.damage;

    setPlayerProperty(victim, "hp", new_hp);
    QString change_str = QString("%1:%2").arg(victim->objectName()).arg(-damage.damage);
    switch (damage.nature) {
        case DamageStruct::Fire: change_str.append("F"); break;
        case DamageStruct::Thunder: change_str.append("T"); break;
        default: break;
    }

    JsonArray arg;
    arg << victim->objectName();
    arg << -damage.damage;
    arg << int(damage.nature);
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);
}

void Room::recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion)
{
    if (player->getLostHp() == 0 || player->isDead())
        return;
    RecoverStruct recover_struct = recover;

    QVariant data = QVariant::fromValue(recover_struct);
    if (thread->trigger(PreHpRecover, this, player, data))
        return;

    recover_struct = data.value<RecoverStruct>();
    int recover_num = recover_struct.recover;

    int new_hp = qMin(player->getHp() + recover_num, player->getMaxHp());
    setPlayerProperty(player, "hp", new_hp);

    JsonArray arg;
    arg << player->objectName();
    arg << recover_num;
    arg << 0;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    if (set_emotion) setEmotion(player, "recover");

    thread->trigger(HpRecover, this, player, data);
}

bool Room::cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to, bool multiple)
{
    CardEffectStruct effect;
    effect.card = card;
    effect.from = from;
    effect.to = to;
    effect.multiple = multiple;

    return cardEffect(effect);
}

bool Room::cardEffect(const CardEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    bool cancel = false;
    if (effect.to->isAlive() || effect.card->isKindOf("Slash")) { // Be care!!!
        // No skills should be triggered here!
        thread->trigger(CardEffect, this, effect.to, data);
        // Make sure that effectiveness of Slash isn't judged here!
        if (!thread->trigger(CardEffected, this, effect.to, data)) {
            cancel = true;
        } else {
            if (!effect.to->hasFlag("Global_NonSkillNullify"))
                ;//setEmotion(effect.to, "skill_nullify");
            else
                effect.to->setFlags("-Global_NonSkillNullify");
        }
    }
    thread->trigger(PostCardEffected, this, effect.to, data);
    return cancel;
}

bool Room::isJinkEffected(ServerPlayer *user, const Card *jink)
{
    if (jink == NULL || user == NULL)
        return false;
    Q_ASSERT(jink->isKindOf("Jink"));
    QVariant jink_data = QVariant::fromValue((const Card *)jink);
    return !thread->trigger(JinkEffect, this, user, jink_data);
}

void Room::damage(const DamageStruct &data)
{
    DamageStruct damage_data = data;
    if (damage_data.to == NULL || damage_data.to->isDead())
        return;

    QVariant qdata = QVariant::fromValue(damage_data);

    if (!damage_data.chain && !damage_data.transfer) {
        thread->trigger(ConfirmDamage, this, damage_data.from, qdata);
        damage_data = qdata.value<DamageStruct>();
    }

#define REMOVE_QINGGANG_TAG if (damage_data.card && damage_data.card->isKindOf("Slash")) damage_data.to->removeQinggangTag(damage_data.card);

    // Predamage
    if (thread->trigger(Predamage, this, damage_data.from, qdata)) {
        REMOVE_QINGGANG_TAG
            return;
    }

    try {
        bool enter_stack = false;
        do {
            if (thread->trigger(DamageForseen, this, damage_data.to, qdata)) {
                REMOVE_QINGGANG_TAG
                    break;
            }

            if (damage_data.from) {
                if (thread->trigger(DamageCaused, this, damage_data.from, qdata)) {
                    REMOVE_QINGGANG_TAG
                        break;
                }
            }

            damage_data = qdata.value<DamageStruct>();
            damage_data.to->tag.remove("TransferDamage");
            if (thread->trigger(DamageInflicted, this, damage_data.to, qdata)) {
                REMOVE_QINGGANG_TAG
                    // Make sure that the trigger in which 'TransferDamage' tag is set returns TRUE
                    DamageStruct transfer_damage_data = damage_data.to->tag["TransferDamage"].value<DamageStruct>();
                if (transfer_damage_data.to)
                    damage(transfer_damage_data);
                break;
            }

            enter_stack = true;
            m_damageStack.push_back(damage_data);
            setTag("CurrentDamageStruct", qdata);

            thread->trigger(PreDamageDone, this, damage_data.to, qdata);

            REMOVE_QINGGANG_TAG
                thread->trigger(DamageDone, this, damage_data.to, qdata);

            if (damage_data.from && !damage_data.from->hasFlag("Global_DFDebut"))
                thread->trigger(Damage, this, damage_data.from, qdata);

            if (!damage_data.to->hasFlag("Global_DFDebut"))
                thread->trigger(Damaged, this, damage_data.to, qdata);
        } while (false);

#undef REMOVE_QINGGANG_TAG

        if (!enter_stack) {
            damage_data = qdata.value<DamageStruct>();
            damage_data.prevented = true;
            qdata = QVariant::fromValue(damage_data);
            setTag("SkipGameRule", true);
        }
        damage_data = qdata.value<DamageStruct>();
        thread->trigger(DamageComplete, this, damage_data.to, qdata);

        if (enter_stack) {
            m_damageStack.pop();
            if (m_damageStack.isEmpty())
                removeTag("CurrentDamageStruct");
            else
                setTag("CurrentDamageStruct", QVariant::fromValue(m_damageStack.first()));
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == StageChange || triggerEvent == TurnBroken) {
            removeTag("is_chained");
            removeTag("CurrentDamageStruct");
            m_damageStack.clear();
        }
        throw triggerEvent;
    }
}

void Room::sendDamageLog(const DamageStruct &data)
{
    LogMessage log;

    if (data.from) {
        log.type = "#Damage";
        log.from = data.from;
    } else {
        log.type = "#DamageNoSource";
    }

    log.to << data.to;
    log.arg = QString::number(data.damage);

    switch (data.nature) {
        case DamageStruct::Normal: log.arg2 = "normal_nature"; break;
        case DamageStruct::Fire: log.arg2 = "fire_nature"; break;
        case DamageStruct::Thunder: log.arg2 = "thunder_nature"; break;
    }

    sendLog(log);
}

ServerPlayer *Room::getFront(ServerPlayer *a, ServerPlayer *b) const
{
    QList<ServerPlayer *> players = getAllPlayers(true);
    int index_a = players.indexOf(a), index_b = players.indexOf(b);
    if (index_a < index_b)
        return a;
    else
        return b;
}

void Room::reconnect(ServerPlayer *player, ClientSocket *socket)
{
    player->setSocket(socket);
    player->setState("online");

    marshal(player);

    broadcastProperty(player, "state");
}

void Room::marshal(ServerPlayer *player)
{
    notifyProperty(player, player, "objectName");
    notifyProperty(player, player, "role");
    notifyProperty(player, player, "flags", "marshalling");

    foreach (ServerPlayer *p, m_players) {
        if (p != player)
            p->introduceTo(player);
    }

    JsonArray player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();

    player->notify(S_COMMAND_ARRANGE_SEATS, player_circle);
    player->notify(S_COMMAND_START_IN_X_SECONDS, 0);

    foreach (ServerPlayer *p, m_players) {
        /* don't notify generals of the player himself here, for kingdom can be set inside Player::setGeneral
           only when it's empty, that is, kingdom can be set in this way only once. */
        if (p == player)
            continue;

        notifyProperty(player, p, "general");

        if (p->getGeneral2())
            notifyProperty(player, p, "general2");
    }

    notifyProperty(player, player, "actual_general1");
    notifyProperty(player, player, "actual_general2");

    notifyProperty(player, player, "general", player->getActualGeneral1Name());
    notifyProperty(player, player, "general2", player->getActualGeneral2Name());

    foreach (const Skill *skill, player->getVisibleSkillList()) {
        JsonArray args1;
        args1 << (int)S_GAME_EVENT_ADD_SKILL;
        args1 << player->objectName();
        args1 << skill->objectName();
        args1 << player->inHeadSkills(skill->objectName());
        doNotify(player, S_COMMAND_LOG_EVENT, args1);
    }

    player->notifyPreshow();

    doNotify(player, S_COMMAND_GAME_START, QVariant());

    QList<int> drawPile = Sanguosha->getRandomCards();
    doNotify(player, S_COMMAND_AVAILABLE_CARDS, JsonUtils::toJsonArray(drawPile));

    foreach (ServerPlayer *p, m_players)
        p->marshal(player);

    notifyProperty(player, player, "flags", "-marshalling");
    doNotify(player, S_COMMAND_UPDATE_PILE, m_drawPile->length());
}

void Room::startGame()
{
    m_alivePlayers = m_players;
    for (int i = 0; i < player_count - 1; i++)
        m_players.at(i)->setNext(m_players.at(i + 1));
    m_players.last()->setNext(m_players.first());

    foreach (ServerPlayer *player, m_players) {
        QStringList generals = getTag(player->objectName()).toStringList();
        const General *general1 = Sanguosha->getGeneral(generals.first());
        const General *general2 = Sanguosha->getGeneral(generals.last());
        handleUsedGeneral(generals.first());
        handleUsedGeneral(generals.last());
        Q_ASSERT(general1 && general2);
        if (general1->isCompanionWith(generals.last()))
            addPlayerMark(player, "CompanionEffect");

        int max_hp = general1->getMaxHpHead() + general2->getMaxHpDeputy();
        setPlayerMark(player, "HalfMaxHpLeft", max_hp % 2);

        player->setMaxHp(max_hp / 2);
        player->setHp(player->getMaxHp());
        // setup AI
        AI *ai = cloneAI(player);
        ais << ai;
        player->setAI(ai);
    }

    foreach (ServerPlayer *player, m_players) {
        broadcastProperty(player, "hp");
        broadcastProperty(player, "maxhp");
    }

    preparePlayers();

    QList<int> drawPile = *m_drawPile;
    qShuffle(drawPile);
    doBroadcastNotify(S_COMMAND_AVAILABLE_CARDS, JsonUtils::toJsonArray(drawPile));

    doBroadcastNotify(S_COMMAND_GAME_START, QVariant());
    game_started = true;

    Server *server = qobject_cast<Server *>(parent());
    foreach (ServerPlayer *player, m_players) {
        if (player->getState() == "online")
            server->signupPlayer(player);
    }

    current = m_players.first();

    // initialize the place_map and owner_map;
    foreach (int card_id, *m_drawPile)
        setCardMapping(card_id, NULL, Player::DrawPile);
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, m_drawPile->length());

    _m_roomState.reset();

    thread = new RoomThread(this);
    connect(thread, &RoomThread::started, this, &Room::game_start);

    if (!_virtual) thread->start();
}

bool Room::notifyProperty(ServerPlayer *playerToNotify, const ServerPlayer *propertyOwner, const char *propertyName, QString value)
{
    if (propertyOwner == NULL) return false;
    if (value.isNull()) value = propertyOwner->property(propertyName).toString();
    JsonArray arg;
    if (propertyOwner == playerToNotify)
        arg << QSanProtocol::S_PLAYER_SELF_REFERENCE_ID;
    else
        arg << propertyOwner->objectName();
    arg << propertyName;
    arg << value;
    return doNotify(playerToNotify, S_COMMAND_SET_PROPERTY, arg);
}

bool Room::broadcastProperty(ServerPlayer *player, const char *property_name, const QString &value)
{
    if (player == NULL) return false;
    QString real_value = value;
    if (real_value.isNull()) real_value = player->property(property_name).toString();

    if (strcmp(property_name, "role"))
        player->setShownRole(true);

    JsonArray arg;
    arg << player->objectName();
    arg << property_name;
    arg << real_value;
    return doBroadcastNotify(S_COMMAND_SET_PROPERTY, arg);
}

void Room::drawCards(ServerPlayer *player, int n, const QString &reason)
{
    QList<ServerPlayer *> players;
    players.append(player);
    drawCards(players, n, reason);
}

void Room::drawCards(QList<ServerPlayer *> players, int n, const QString &reason)
{
    QList<int> n_list;
    n_list.append(n);
    drawCards(players, n_list, reason);
}

void Room::drawCards(QList<ServerPlayer *> players, QList<int> n_list, const QString &reason)
{
    QList<CardsMoveStruct> moves;
    int index = -1, len = n_list.length();
    Q_ASSERT(len >= 1);
    foreach (ServerPlayer *player, players) {
        index++;
        if (!player->isAlive() && reason != "reform") continue;
        int n = n_list.at(qMin(index, len - 1));
        if (n <= 0) continue;
        QList<int> card_ids = getNCards(n, false);

        CardsMoveStruct move;
        move.card_ids = card_ids;
        move.from = NULL;
        move.to = player;
        move.to_place = Player::PlaceHand;
        moves.append(move);
    }
    moveCardsAtomic(moves, false);
}

void Room::throwCard(const Card *card, ServerPlayer *who, ServerPlayer *thrower, const QString &skill_name)
{
    CardMoveReason reason;
    if (thrower == NULL) {
        reason.m_reason = CardMoveReason::S_REASON_THROW;
        reason.m_playerId = who ? who->objectName() : QString();
    } else {
        reason.m_reason = CardMoveReason::S_REASON_DISMANTLE;
        reason.m_targetId = who ? who->objectName() : QString();
        reason.m_playerId = thrower->objectName();
    }
    reason.m_skillName = card->getSkillName();
    throwCard(card, reason, who, thrower, skill_name);
}

void Room::throwCard(const Card *card, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower, const QString &skill_name)
{
    if (card == NULL)
        return;

    QList<int> to_discard;
    if (card->isVirtualCard())
        to_discard.append(card->getSubcards());
    else
        to_discard << card->getEffectiveId();

    LogMessage log;
    if (who) {
        if (thrower == NULL) {
            if (skill_name.isEmpty())
                log.type = "$DiscardCard";
            else {
                log.type = "$DiscardCardWithSkill";
                log.arg = skill_name;
            }
            log.from = who;
        } else {
            log.type = "$DiscardCardByOther";
            log.from = thrower;
            log.to << who;
        }
    } else {
        log.type = "$EnterDiscardPile";
    }
    log.card_str = IntList2StringList(to_discard).join("+");
    sendLog(log);

    if (who) { // player's card cannot enter discard_pile directly
        CardsMoveStruct move(to_discard, who, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moveCardsAtomic(move, true);
        QList<int> new_list = getCardIdsOnTable(to_discard);
        if (!new_list.isEmpty()) {
            CardsMoveStruct move2(new_list, who, NULL, Player::PlaceTable, Player::DiscardPile, reason);
            moveCardsAtomic(move2, true);
        }
    } else if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
        // discard must through place_table
        CardsMoveStruct move(to_discard, NULL, Player::PlaceTable, reason);
        moveCardsAtomic(move, true);
        QList<int> new_list = getCardIdsOnTable(to_discard);
        if (!new_list.isEmpty()) {
            CardsMoveStruct move2(new_list, NULL, Player::DiscardPile, reason);
            moveCardsAtomic(move2, true);
        }
    } else { // other conditions
        CardsMoveStruct move(to_discard, NULL, Player::DiscardPile, reason);
        moveCardsAtomic(move, true);
    }
}

void Room::throwCard(int card_id, ServerPlayer *who, ServerPlayer *thrower, const QString &skill_name)
{
    throwCard(Sanguosha->getCard(card_id), who, thrower, skill_name);
}

RoomThread *Room::getThread() const
{
    return thread;
}

void Room::moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, bool forceMoveVisible)
{
    moveCardTo(card, dstPlayer, dstPlace,
        CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()), forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace,
    const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, NULL, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
    const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, srcPlayer, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
    const QString &pileName, const CardMoveReason &reason, bool forceMoveVisible)
{
    CardsMoveStruct move;
    if (card->isVirtualCard()) {
        move.card_ids = card->getSubcards();
        if (move.card_ids.size() == 0) return;
    } else
        move.card_ids.append(card->getId());
    move.to = dstPlayer;
    move.to_place = dstPlace;
    move.to_pile_name = pileName;

    if (!pileName.isEmpty())
        dstPlayer->pileAdd(pileName, move.card_ids);

    move.from = srcPlayer;
    move.reason = reason;
    QList<CardsMoveStruct> moves;
    moves.append(move);
    moveCardsAtomic(moves, forceMoveVisible);
}

void Room::moveCards(CardsMoveStruct cards_move, bool forceMoveVisible, bool ignoreChanges)
{
    QList<CardsMoveStruct> cards_moves;
    cards_moves.append(cards_move);
    moveCards(cards_moves, forceMoveVisible, ignoreChanges);
}

void Room::_fillMoveInfo(CardsMoveStruct &moves, int card_index) const
{
    int card_id = moves.card_ids[card_index];
    if (!moves.from)
        moves.from = getCardOwner(card_id);
    moves.from_place = getCardPlace(card_id);
    if (moves.from) { // Hand/Equip/Judge
        if (moves.from_place == Player::PlaceSpecial || moves.from_place == Player::PlaceTable)
            moves.from_pile_name = moves.from->getPileName(card_id);
        if (moves.from_player_name.isEmpty())
            moves.from_player_name = moves.from->objectName();
    }
    if (moves.to) {
        if (moves.to_player_name.isEmpty())
            moves.to_player_name = moves.to->objectName();
        int card_id = moves.card_ids[card_index];
        if (moves.to_place == Player::PlaceSpecial || moves.to_place == Player::PlaceTable)
            moves.to_pile_name = moves.to->getPileName(card_id);
    }
}

static bool CompareByActionOrder_OneTime(CardsMoveOneTimeStruct move1, CardsMoveOneTimeStruct move2)
{
    ServerPlayer *a = qobject_cast<ServerPlayer *>(move1.from);
    if (a == NULL) a = qobject_cast<ServerPlayer *>(move1.to);
    ServerPlayer *b = qobject_cast<ServerPlayer *>(move2.from);
    if (b == NULL) b = qobject_cast<ServerPlayer *>(move2.to);

    if (a == NULL || b == NULL)
        return a != NULL;

    Room *room = a->getRoom();
    return room->getFront(a, b) == a;
}

static bool CompareByActionOrder(CardsMoveStruct move1, CardsMoveStruct move2)
{
    ServerPlayer *a = qobject_cast<ServerPlayer *>(move1.from);
    if (a == NULL) a = qobject_cast<ServerPlayer *>(move1.to);
    ServerPlayer *b = qobject_cast<ServerPlayer *>(move2.from);
    if (b == NULL) b = qobject_cast<ServerPlayer *>(move2.to);

    if (a == NULL || b == NULL)
        return a != NULL;

    Room *room = a->getRoom();
    return room->getFront(a, b) == a;
}

QList<CardsMoveOneTimeStruct> Room::_mergeMoves(QList<CardsMoveStruct> cards_moves)
{
    QMap<_MoveMergeClassifier, QList<CardsMoveStruct> > moveMap;

    foreach (const CardsMoveStruct &cards_move, cards_moves) {
        _MoveMergeClassifier classifier(cards_move);
        moveMap[classifier].append(cards_move);
    }

    QList<CardsMoveOneTimeStruct> result;
    foreach (const _MoveMergeClassifier &cls, moveMap.keys()) {
        CardsMoveOneTimeStruct moveOneTime;
        moveOneTime.from = cls.m_from;
        moveOneTime.reason = moveMap[cls].first().reason;
        moveOneTime.to = cls.m_to;
        moveOneTime.to_place = cls.m_to_place;
        moveOneTime.to_pile_name = cls.m_to_pile_name;
        moveOneTime.is_last_handcard = false;
        moveOneTime.origin_from = cls.m_origin_from;
        moveOneTime.origin_to = cls.m_origin_to;
        moveOneTime.origin_to_place = cls.m_origin_to_place;
        moveOneTime.origin_to_pile_name = cls.m_origin_to_pile_name;
        foreach (const CardsMoveStruct &move, moveMap[cls]) {
            moveOneTime.card_ids.append(move.card_ids);
            for (int i = 0; i < move.card_ids.size(); i++) {
                moveOneTime.from_places.append(move.from_place);
                moveOneTime.origin_from_places.append(move.from_place);
                moveOneTime.from_pile_names.append(move.from_pile_name);
                moveOneTime.origin_from_pile_names.append(move.from_pile_name);
                moveOneTime.open.append(move.open);
            }
            if (move.is_last_handcard)
                moveOneTime.is_last_handcard = true;
        }
        result.append(moveOneTime);
    }

    if (result.size() > 1)
        qSort(result.begin(), result.end(), CompareByActionOrder_OneTime);

    return result;
}

QList<CardsMoveStruct> Room::_separateMoves(QList<CardsMoveOneTimeStruct> moveOneTimes)
{
    QList<_MoveSeparateClassifier> classifiers;
    QList<QList<int> > ids;
    foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
        for (int i = 0; i < moveOneTime.card_ids.size(); i++) {
            _MoveSeparateClassifier classifier(moveOneTime, i);
            if (classifiers.contains(classifier)) {
                int pos = classifiers.indexOf(classifier);
                ids[pos].append(moveOneTime.card_ids[i]);
            } else {
                classifiers << classifier;
                QList<int> new_ids;
                new_ids << moveOneTime.card_ids[i];
                ids << new_ids;
            }
        }
    }

    QList<CardsMoveStruct> card_moves;
    int i = 0;
    QMap<ServerPlayer *, QList<int> > from_handcards;
    foreach (const _MoveSeparateClassifier &cls, classifiers) {
        CardsMoveStruct card_move;
        ServerPlayer *from = qobject_cast<ServerPlayer *>(cls.m_from);
        card_move.from = cls.m_from;
        if (from && !from_handcards.contains(from))
            from_handcards[from] = from->handCards();
        card_move.to = cls.m_to;
        if (card_move.from)
            card_move.from_player_name = card_move.from->objectName();
        if (card_move.to)
            card_move.to_player_name = card_move.to->objectName();
        card_move.from_place = cls.m_from_place;
        card_move.to_place = cls.m_to_place;
        card_move.from_pile_name = cls.m_from_pile_name;
        card_move.to_pile_name = cls.m_to_pile_name;
        card_move.open = cls.m_open;
        card_move.card_ids = ids.at(i);
        card_move.reason = cls.m_reason;

        card_move.origin_from = cls.m_from;
        card_move.origin_to = cls.m_to;
        card_move.origin_from_place = cls.m_from_place;
        card_move.origin_to_place = cls.m_to_place;
        card_move.origin_from_pile_name = cls.m_from_pile_name;
        card_move.origin_to_pile_name = cls.m_to_pile_name;

        if (from && from_handcards.contains(from)) {
            QList<int> &move_ids = from_handcards[from];
            if (!move_ids.isEmpty()) {
                foreach (int id, card_move.card_ids)
                    move_ids.removeOne(id);
                card_move.is_last_handcard = move_ids.isEmpty();
            }
        }

        card_moves.append(card_move);
        i++;
    }
    if (card_moves.size() > 1)
        qSort(card_moves.begin(), card_moves.end(), CompareByActionOrder);
    return card_moves;
}

void Room::moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible)
{
    QList<CardsMoveStruct> cards_moves;
    cards_moves.append(cards_move);
    moveCardsAtomic(cards_moves, forceMoveVisible);
}

void Room::moveCardsAtomic(QList<CardsMoveStruct> cards_moves, bool forceMoveVisible)
{
    cards_moves = _breakDownCardMoves(cards_moves);

    QList<CardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        int i = 0;
        foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
            if (moveOneTime.card_ids.size() == 0) {
                i++;
                continue;
            }
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(BeforeCardsMove, this, player, data);
            CardsMoveOneTimeStruct moveOneTime2 = data.value<CardsMoveOneTimeStruct>();
            moveOneTimes[i] = moveOneTime2;
            i++;
        }
    }
    cards_moves = _separateMoves(moveOneTimes);

    notifyMoveCards(true, cards_moves, forceMoveVisible);
    // First, process remove card
    bool drawpile_changed = false;
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);

            if (cards_move.from) // Hand/Equip/Judge
                cards_move.from->removeCard(card, cards_move.from_place);

            switch (cards_move.from_place) {
                case Player::DiscardPile:
                    m_discardPile->removeOne(card_id);
                    break;
                case Player::DrawPile:
                case Player::DrawPileBottom:
                    m_drawPile->removeOne(card_id);
                    break;
                case Player::PlaceSpecial:
                    table_cards.removeOne(card_id);
                    break;
                default:
                    break;
            }
        }
        if (cards_move.from_place == Player::DrawPile || cards_move.from_place == Player::DrawPileBottom)
            drawpile_changed = true;
    }

    if (drawpile_changed)
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnLose(move);

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++)
            setCardMapping(cards_move.card_ids[j], qobject_cast<ServerPlayer *>(cards_move.to), cards_move.to_place);
    }
    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnGet(move);
    notifyMoveCards(false, cards_moves, forceMoveVisible);

    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);
            if (forceMoveVisible && cards_move.to_place == Player::PlaceHand)
                card->setFlags("visible");
            else
                card->setFlags("-visible");
            if (cards_move.to) // Hand/Equip/Judge
                cards_move.to->addCard(card, cards_move.to_place);

            switch (cards_move.to_place) {
                case Player::DiscardPile:
                    m_discardPile->prepend(card_id);
                    break;
                case Player::DrawPile:
                    m_drawPile->prepend(card_id);
                    break;
                case Player::DrawPileBottom:
                    m_drawPile->append(card_id);
                    break;
                case Player::PlaceSpecial:
                    table_cards.append(card_id);
                    break;
                default:
                    break;
            }
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
            if (moveOneTime.card_ids.size() == 0) continue;
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(CardsMoveOneTime, this, player, data);
        }
    }
}

QList<CardsMoveStruct> Room::_breakDownCardMoves(QList<CardsMoveStruct> &cards_moves)
{
    QList<CardsMoveStruct> all_sub_moves;
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &move = cards_moves[i];
        if (move.card_ids.size() == 0) continue;

        QMap<_MoveSourceClassifier, QList<int> > moveMap;
        // reassemble move sources
        for (int j = 0; j < move.card_ids.size(); j++) {
            _fillMoveInfo(move, j);
            _MoveSourceClassifier classifier(move);
            moveMap[classifier].append(move.card_ids[j]);
        }
        foreach (const _MoveSourceClassifier &cls, moveMap.keys()) {
            CardsMoveStruct sub_move = move;
            cls.copyTo(sub_move);
            if ((sub_move.from == sub_move.to && sub_move.from_place == sub_move.to_place)
                || sub_move.card_ids.size() == 0)
                continue;
            sub_move.card_ids = moveMap[cls];
            all_sub_moves.append(sub_move);
        }
    }
    return all_sub_moves;
}

void Room::moveCards(QList<CardsMoveStruct> cards_moves, bool forceMoveVisible, bool enforceOrigin)
{
    QList<CardsMoveStruct> all_sub_moves = _breakDownCardMoves(cards_moves);
    _moveCards(all_sub_moves, forceMoveVisible, enforceOrigin);
}

void Room::_moveCards(QList<CardsMoveStruct> cards_moves, bool forceMoveVisible, bool enforceOrigin)
{
    // First, process remove card

    QList<CardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        int i = 0;
        foreach (const CardsMoveOneTimeStruct &_moveOneTime, moveOneTimes) {
            if (_moveOneTime.card_ids.size() == 0) {
                i++;
                continue;
            }
            Player *origin_to = _moveOneTime.to;
            Player::Place origin_place = _moveOneTime.to_place;
            Player *origin_from = _moveOneTime.from;
            QList<Player::Place> origin_from_places = _moveOneTime.from_places;
            CardsMoveOneTimeStruct moveOneTime = _moveOneTime;
            moveOneTime.origin_to_place = origin_place;
            moveOneTime.origin_to = origin_to;
            moveOneTime.to = NULL;
            moveOneTime.to_place = Player::PlaceTable;
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(BeforeCardsMove, this, player, data);
            moveOneTime = data.value<CardsMoveOneTimeStruct>();
            moveOneTime.origin_from_places = origin_from_places;
            moveOneTime.origin_from = origin_from;
            moveOneTime.to = origin_to;
            moveOneTime.to_place = origin_place;
            moveOneTimes[i] = moveOneTime;
            i++;
        }
    }

    cards_moves = _separateMoves(moveOneTimes);
    notifyMoveCards(true, cards_moves, forceMoveVisible);
    QList<CardsMoveStruct> origin = cards_moves;

    QList<Player::Place> final_places;
    QList<Player *> move_tos;
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        final_places.append(cards_move.to_place);
        move_tos.append(cards_move.to);

        cards_move.to_place = Player::PlaceTable;
        cards_move.to = NULL;
    }

    bool drawpile_changed = false;
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);

            if (cards_move.from) // Hand/Equip/Judge
                cards_move.from->removeCard(card, cards_move.from_place);

            switch (cards_move.from_place) {
                case Player::DiscardPile:
                    m_discardPile->removeOne(card_id);
                    break;
                case Player::DrawPile:
                case Player::DrawPileBottom:
                    m_drawPile->removeOne(card_id);
                    break;
                case Player::PlaceSpecial:
                    table_cards.removeOne(card_id);
                    break;
                default:
                    break;
            }

            setCardMapping(card_id, NULL, Player::PlaceTable);
        }
        if (cards_move.from_place == Player::DrawPile || cards_move.from_place == Player::DrawPileBottom)
            drawpile_changed = true;
    }

    if (drawpile_changed) {
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    }

    foreach(const CardsMoveStruct &move, cards_moves)
        updateCardsOnLose(move);

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
            if (moveOneTime.card_ids.size() == 0) continue;
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(CardsMoveOneTime, this, player, data);
        }
    }

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        cards_move.to = move_tos[i];
        cards_move.to_place = final_places[i];
    }

    if (enforceOrigin) {
        for (int i = 0; i < cards_moves.size(); i++) {
            CardsMoveStruct &cards_move = cards_moves[i];
            if (cards_move.to && !cards_move.to->isAlive()) {
                cards_move.to = NULL;
                cards_move.to_place = Player::DiscardPile;
            }
        }
    }

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        cards_move.from_place = Player::PlaceTable;
    }

    moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        int i = 0;
        foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
            if (moveOneTime.card_ids.size() == 0) {
                i++;
                continue;
            }
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(BeforeCardsMove, this, player, data);
            CardsMoveOneTimeStruct moveOneTime2 = data.value<CardsMoveOneTimeStruct>();
            moveOneTimes[i] = moveOneTime2;
            i++;
        }
    }
    cards_moves = _separateMoves(moveOneTimes);

    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++)
            setCardMapping(cards_move.card_ids[j], qobject_cast<ServerPlayer *>(cards_move.to), cards_move.to_place);
    }
    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnGet(move);

    QList<CardsMoveStruct> origin_x;
    foreach (const CardsMoveStruct &m, origin) {
        CardsMoveStruct m_x = m;
        m_x.card_ids.clear();
        m_x.to = NULL;
        m_x.to_place = Player::DiscardPile;
        foreach (int id, m.card_ids) {
            bool sure = false;
            foreach (const CardsMoveStruct &cards_move, cards_moves) {
                if (cards_move.card_ids.contains(id)) {
                    m_x.card_ids << id;
                    if (!sure) {
                        m_x.to = cards_move.to;
                        m_x.to_place = cards_move.to_place;
                        sure = true;
                    }
                }
            }
        }
        origin_x.append(m_x);
    }
    notifyMoveCards(false, origin_x, forceMoveVisible);

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);
            if (forceMoveVisible && cards_move.to_place == Player::PlaceHand)
                card->setFlags("visible");
            else
                card->setFlags("-visible");
            if (cards_move.to) // Hand/Equip/Judge
                cards_move.to->addCard(card, cards_move.to_place);

            switch (cards_move.to_place) {
                case Player::DiscardPile:
                    m_discardPile->prepend(card_id);
                    break;
                case Player::DrawPile:
                    m_drawPile->prepend(card_id);
                    break;
                case Player::DrawPileBottom:
                    m_drawPile->append(card_id);
                    break;
                case Player::PlaceSpecial:
                    table_cards.append(card_id);
                    break;
                default:
                    break;
            }
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (ServerPlayer *player, getAllPlayers()) {
        foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
            if (moveOneTime.card_ids.size() == 0) continue;
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(CardsMoveOneTime, this, player, data);
        }
    }
}

void Room::updateCardsOnLose(const CardsMoveStruct &move)
{
    for (int i = 0; i < move.card_ids.size(); i++) {
        WrappedCard *card = qobject_cast<WrappedCard *>(getCard(move.card_ids[i]));
        if (card->isModified()) {
            if (move.to_place == Player::DiscardPile) {
                resetCard(move.card_ids[i]);
                broadcastResetCard(getPlayers(), move.card_ids[i]);
            }
        }
    }
}

void Room::updateCardsOnGet(const CardsMoveStruct &move)
{
    if (move.card_ids.isEmpty()) return;
    ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
    if (player != NULL && move.to_place == Player::PlaceDelayedTrick) {
        for (int i = 0; i < move.card_ids.size(); i++) {
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(move.card_ids[i]));
            const Card *engine_card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->getSuit() != engine_card->getSuit() || card->getNumber() != engine_card->getNumber()) {
                Card *trick = Sanguosha->cloneCard(card->getRealCard());
                trick->setSuit(engine_card->getSuit());
                trick->setNumber(engine_card->getNumber());
                card->takeOver(trick);
                broadcastUpdateCard(getPlayers(), move.card_ids[i], card);
            }
        }
        return;
    }

    player = qobject_cast<ServerPlayer *>(move.to);
    if (player != NULL && (move.to_place == Player::PlaceHand
        || move.to_place == Player::PlaceEquip
        || move.to_place == Player::PlaceJudge
        || move.to_place == Player::PlaceSpecial)) {
        QList<const Card *> cards;
        foreach (int cardId, move.card_ids)
            cards.append(getCard(cardId));
        filterCards(player, cards, true);
    }
}

bool Room::notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> cards_moves, bool forceVisible, QList<ServerPlayer *> players)
{
    if (players.isEmpty()) players = m_players;

    // Notify clients
    int moveId;
    if (isLostPhase)
        moveId = _m_lastMovementId++;
    else
        moveId = --_m_lastMovementId;
    Q_ASSERT(_m_lastMovementId >= 0);
    foreach (ServerPlayer *player, players) {
        if (player->isOffline()) continue;
        JsonArray arg;
        arg << moveId;
        int move_num = cards_moves.size();
        for (int i = 0; i < move_num; i++) {
            CardsMoveStruct &cards_move = cards_moves[i];
            cards_move.open = forceVisible || cards_move.isRelevant(player)
                // forceVisible will override cards to be visible
                || cards_move.to_place == Player::PlaceEquip
                || cards_move.from_place == Player::PlaceEquip
                || cards_move.to_place == Player::PlaceDelayedTrick
                || cards_move.from_place == Player::PlaceDelayedTrick
                // only cards moved to hand/special can be invisible
                || cards_move.from_place == Player::DiscardPile
                || cards_move.to_place == Player::DiscardPile
                // any card from/to discard pile should be visible
                || cards_move.from_place == Player::PlaceTable
                || cards_move.to_place == Player::PlaceTable
                // any card from/to place table should be visible
                || player->hasFlag("Global_GongxinOperator")
                || (!cards_move.to_pile_name.isEmpty() && cards_move.to && cards_move.to->pileOpen(cards_move.to_pile_name, player->objectName()));
            // the player put someone's cards to the drawpile
            arg << cards_move.toVariant();
        }
        doNotify(player, isLostPhase ? S_COMMAND_LOSE_CARD : S_COMMAND_GET_CARD, arg);
    }
    return true;
}

void Room::notifySkillInvoked(ServerPlayer *player, const QString &skill_name)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_SKILL_INVOKED;
    args << player->objectName();
    args << skill_name;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::broadcastSkillInvoke(const QString &skill_name, const QString &category)
{
    broadcastSkillInvoke(skill_name, category, -1, NULL);
}

void Room::broadcastSkillInvoke(const QString &skill_name, const ServerPlayer *who)
{
    broadcastSkillInvoke(skill_name, "male", -1, who);
}

void Room::broadcastSkillInvoke(const QString &skill_name, int type, const ServerPlayer *who)
{
    broadcastSkillInvoke(skill_name, "male", type, who);
}

void Room::broadcastSkillInvoke(const QString &skill_name, bool isMale, int type)
{
    broadcastSkillInvoke(skill_name, isMale ? "male" : "female", type, NULL);
}

void Room::broadcastSkillInvoke(const QString &skill_name, const QString &category, int type, const ServerPlayer *who, const QString &position)
{
    QString position_copy = position;
    if (who != NULL && position.isEmpty()) {
        const Skill *mainskill = Sanguosha->getMainSkill(skill_name);
        if (mainskill && !getTag(mainskill->objectName() + who->objectName()).toStringList().isEmpty()) {
            QStringList names = getTag(mainskill->objectName() + who->objectName()).toStringList();
            if (!names.isEmpty())
                position_copy = names.last();
        }
    }
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << category;
    args << type;
    if (who != NULL) {
        args << who->objectName();
        if (!position_copy.isEmpty())
            args << position_copy;
    }
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::doLightbox(const QString &lightboxName, int duration)
{
    if (Config.AIDelay == 0)
        return;

    doAnimate(S_ANIMATE_LIGHTBOX, lightboxName, QString::number(duration));
    thread->delay(duration / 1.2);
}

void Room::doSuperLightbox(const QString &heroName, const QString &skillName)
{
    if (Config.AIDelay == 0)
        return;

    doAnimate(S_ANIMATE_LIGHTBOX, "skill=" + heroName, skillName);
    thread->delay(4500);
}

void Room::doAnimate(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2,
    QList<ServerPlayer *> players)
{
    if (players.isEmpty())
        players = m_players;
    JsonArray arg;
    arg << (int)type;
    arg << arg1;
    arg << arg2;
    doBroadcastNotify(players, S_COMMAND_ANIMATE, arg);
}

void Room::doBattleArrayAnimate(ServerPlayer *player, ServerPlayer *target)
{
    if  (getAlivePlayers().length() < 4) return;
    if (!target) {
        QStringList names;
        foreach (const Player *p, player->getFormation())
            names << p->objectName();
        if (names.length() > 1)
            doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), names.join("+"));
    } else {
        foreach (ServerPlayer *p, getOtherPlayers(player))
            if (p->inSiegeRelation(player, target))
                doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), QString("%1+%2").arg(p->objectName()).arg(player->objectName()));
    }
}

void Room::preparePlayers()
{
    foreach (ServerPlayer *player, m_players) {
        QString general1_name = tag[player->objectName()].toStringList().at(0);
        if (!player->property("Duanchang").toString().split(",").contains("head")) {
            foreach (const Skill *skill, Sanguosha->getGeneral(general1_name)->getVisibleSkillList(true, true))
                player->addSkill(skill->objectName());
        }
        QString general2_name = tag[player->objectName()].toStringList().at(1);
        if (!player->property("Duanchang").toString().split(",").contains("deputy")) {
            foreach (const Skill *skill, Sanguosha->getGeneral(general2_name)->getVisibleSkillList(true, false))
                player->addSkill(skill->objectName(), false);
        }

        JsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        doNotify(player, QSanProtocol::S_COMMAND_LOG_EVENT, args);

        notifyProperty(player, player, "flags", "AutoPreshowAvailable");
        player->notifyPreshow();
        notifyProperty(player, player, "flags", "-AutoPreshowAvailable");

        player->setGender(General::Sexless);
    }
}

void Room::changePlayerGeneral(ServerPlayer *player, const QString &new_general)
{
    player->setProperty("general", new_general);
    QList<ServerPlayer *> players = m_players;
    if (new_general == "anjiang") players.removeOne(player);
    foreach (ServerPlayer *p, players)
        notifyProperty(p, player, "general");

    Q_ASSERT(player->getGeneral() != NULL);
    if (new_general != "anjiang")
        player->setGender(player->getGeneral()->getGender());
    else {
        if (player->hasShownGeneral2())
            player->setGender(player->getGeneral2()->getGender());
        else
            player->setGender(General::Sexless);
    }
    filterCards(player, player->getCards("he"), true);
}

void Room::changePlayerGeneral2(ServerPlayer *player, const QString &new_general)
{
    player->setProperty("general2", new_general);
    QList<ServerPlayer *> players = m_players;
    if (new_general == "anjiang") players.removeOne(player);
    foreach (ServerPlayer *p, players)
        notifyProperty(p, player, "general2");
    Q_ASSERT(player->getGeneral2() != NULL);
    if (!player->hasShownGeneral1()) {
        if (new_general != "anjiang")
            player->setGender(player->getGeneral2()->getGender());
        else
            player->setGender(General::Sexless);
    }
    filterCards(player, player->getCards("he"), true);
}

void Room::filterCards(ServerPlayer *player, QList<const Card *> cards, bool refilter)
{
    if (refilter) {
        for (int i = 0; i < cards.size(); i++) {
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(cards[i]->getId()));
            if (card->isModified()) {
                int cardId = card->getId();
                resetCard(cardId);
                if (getCardPlace(cardId) != Player::PlaceHand)
                    broadcastResetCard(m_players, cardId);
                else
                    notifyResetCard(player, cardId);
            }
        }
    }

    QList<bool> cardChanged;
    for (int i = 0; i < cards.size(); i++)
        cardChanged << false;

    QSet<const Skill *> skills = player->getSkills(false, false);
    QList<const FilterSkill *> filterSkills;

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("ViewHasSkill")) {
            const ViewHasSkill *skill = qobject_cast<const ViewHasSkill *>(Sanguosha->getSkill(name));
            foreach (QString name1, Sanguosha->getSkillNames()) {
                if (!Sanguosha->getSkill(name1)->inherits("ViewHasSkill") && skill->ViewHas(player, name1, "skill"))
                    skills.insert(Sanguosha->getSkill(name1));
            }
        }
    }

    foreach (const Skill *skill, skills) {
        if (player->hasSkill(skill->objectName()) && skill->inherits("FilterSkill")) {
            const FilterSkill *filter = qobject_cast<const FilterSkill *>(skill);
            Q_ASSERT(filter);
            filterSkills.append(filter);
        }
        if (player->hasSkill(skill->objectName()) && skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger = qobject_cast<const TriggerSkill *>(skill);
            const ViewAsSkill *vsskill = trigger->getViewAsSkill();
            if (vsskill && vsskill->inherits("FilterSkill")) {
                const FilterSkill *filter = qobject_cast<const FilterSkill *>(vsskill);
                Q_ASSERT(filter);
                filterSkills.append(filter);
            }
        }
    }
    if (filterSkills.size() == 0) return;

    for (int i = 0; i < cards.size(); i++) {
        const Card *card = cards[i];
        for (int fTime = 0; fTime < filterSkills.size(); fTime++) {
            bool converged = true;
            foreach (const FilterSkill *skill, filterSkills) {
                Q_ASSERT(skill);
                if (skill->viewFilter(cards[i], player)) {
                    cards[i] = skill->viewAs(card);
                    Q_ASSERT(cards[i] != NULL);
                    converged = false;
                    cardChanged[i] = true;
                }
            }
            if (converged) break;
        }
    }

    for (int i = 0; i < cards.size(); i++) {
        int cardId = cards[i]->getId();
        Player::Place place = getCardPlace(cardId);
        if (!cardChanged[i]) continue;
        if (place == Player::PlaceHand) {
            notifyUpdateCard(player, cardId, cards[i]);
        } else {
            broadcastUpdateCard(getPlayers(), cardId, cards[i]);
            if (place == Player::PlaceJudge) {
                LogMessage log;
                log.type = "#FilterJudge";
                log.arg = cards[i]->getSkillName();
                log.from = player;

                sendLog(log);
                broadcastSkillInvoke(cards[i]->getSkillName());
            }
        }
    }
}

void Room::acquireSkill(ServerPlayer *player, const Skill *skill, bool open, bool head)
{
    QString skill_name = skill->objectName();
    if (player->getAcquiredSkills().contains(skill_name))
        return;
    player->acquireSkill(skill_name, head);

    if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
        setPlayerMark(player, skill->getLimitMark(), 1);

    if (skill->isVisible()) {
        if (open) {
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL;
            args << player->objectName();
            args << skill_name;
            args << head;
            doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }

        QVariant data = skill_name + ":" + (head ? "head" : "deputy");
        thread->trigger(EventAcquireSkill, this, player, data);
    }
}

void Room::acquireSkill(ServerPlayer *player, const QString &skill_name, bool open, bool head)
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill) acquireSkill(player, skill, open, head);
}

void Room::setTag(const QString &key, const QVariant &value)
{
    tag.insert(key, value);
    if (scenario) scenario->onTagSet(this, key);
}

QVariant Room::getTag(const QString &key) const
{
    return tag.value(key);
}

void Room::removeTag(const QString &key)
{
    tag.remove(key);
}

void Room::setEmotion(ServerPlayer *target, const QString &emotion, bool playback, int duration)
{
    JsonArray arg;
    arg << target->objectName();
    arg << (emotion.isEmpty() ? QString(".") : emotion);
    arg << playback;
    arg << duration;
    doBroadcastNotify(S_COMMAND_SET_EMOTION, arg);
}

void Room::activate(ServerPlayer *player, CardUseStruct &card_use)
{
    tryPause();

    if (player->hasFlag("Global_PlayPhaseTerminated")) {
        setPlayerFlag(player, "-Global_PlayPhaseTerminated");
        card_use.card = NULL;
        return;
    }

    if (player->getPhase() != Player::Play)
        return;

    notifyMoveFocus(player, S_COMMAND_PLAY_CARD);

    _m_roomState.setCurrentCardUsePattern(QString());
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY);

    AI *ai = player->getAI();
    if (ai) {
        QElapsedTimer timer;
        timer.start();

        card_use.from = player;
        ai->activate(card_use);

        qint64 diff = Config.AIDelay - timer.elapsed();
        if (diff > 0) thread->delay(diff);
    } else {
        bool success = doRequest(player, S_COMMAND_PLAY_CARD, player->objectName(), true);
        const QVariant &clientReply = player->getClientReply();

        if (m_surrenderRequestReceived) {
            makeSurrender(player);
            if (!game_finished)
                return activate(player, card_use);
        } else {
            if (Config.EnableCheat && makeCheat(player)) {
                if (player->isAlive()) return activate(player, card_use);
                return;
            }
        }

        if (!success || clientReply.isNull()) return;

        card_use.from = player;
        if (!card_use.tryParse(clientReply, this)) {
            JsonArray use = clientReply.value<JsonArray>();
            emit room_message(tr("Card cannot be parsed:\n %1").arg(use[0].toString()));
            return;
        }
    }
    card_use.m_reason = CardUseStruct::CARD_USE_REASON_PLAY;
    if (!card_use.isValid(QString()))
        return;
    QVariant data = QVariant::fromValue(card_use);
    thread->trigger(ChoiceMade, this, player, data);
}

void Room::askForLuckCard()
{
    tryPause();

    QList<ServerPlayer *> players;
    foreach (ServerPlayer *player, m_players) {
        if (!player->getAI()) {
            player->m_commandArgs = QVariant();
            players << player;
        }
    }

    int n = 0;
    while (n < Config.LuckCardLimitation) {
        if (players.isEmpty())
            return;

        n++;

        Countdown countdown;
        countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_LUCK_CARD, S_CLIENT_INSTANCE);
        countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
        notifyMoveFocus(players, countdown);

        doBroadcastRequest(players, S_COMMAND_LUCK_CARD);

        QList<ServerPlayer *> used;
        foreach (ServerPlayer *player, players) {
            const QVariant &clientReply = player->getClientReply();
            if (!player->m_isClientResponseReady || !JsonUtils::isBool(clientReply) || !clientReply.toBool()) {
                players.removeOne(player);
                continue;
            }
            used << player;
        }
        if (used.isEmpty())
            return;

        LogMessage log;
        log.type = "#UseLuckCard";
        foreach (ServerPlayer *player, used) {
            log.from = player;
            sendLog(log);
        }

        QList<int> draw_list;
        foreach (ServerPlayer *player, used) {
            draw_list << player->getHandcardNum();

            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), "luck_card", QString());
            QList<CardsMoveStruct> moves;
            CardsMoveStruct move;
            move.from = player;
            move.from_place = Player::PlaceHand;
            move.to = NULL;
            move.to_place = Player::DrawPile;
            move.card_ids = player->handCards();
            move.reason = reason;
            moves.append(move);
            moves = _breakDownCardMoves(moves);

            QList<ServerPlayer *> tmp_list;
            tmp_list.append(player);

            notifyMoveCards(true, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card = Sanguosha->getCard(card_id);
                player->removeCard(card, Player::PlaceHand);
            }

            updateCardsOnLose(move);
            for (int j = 0; j < move.card_ids.size(); j++)
                setCardMapping(move.card_ids[j], NULL, Player::DrawPile);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                m_drawPile->prepend(card_id);
            }
        }
        qShuffle(*m_drawPile);
        int index = -1;
        foreach (ServerPlayer *player, used) {
            index++;
            QList<CardsMoveStruct> moves;
            CardsMoveStruct move;
            move.from = NULL;
            move.from_place = Player::DrawPile;
            move.to = player;
            move.to_place = Player::PlaceHand;
            move.card_ids = getNCards(draw_list.at(index), false);
            moves.append(move);
            moves = _breakDownCardMoves(moves);

            notifyMoveCards(true, moves, false);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                m_drawPile->removeOne(card_id);
            }

            updateCardsOnLose(move);
            for (int j = 0; j < move.card_ids.size(); j++)
                setCardMapping(move.card_ids[j], player, Player::PlaceHand);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card = Sanguosha->getCard(card_id);
                player->addCard(card, Player::PlaceHand);
            }
        }
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    }
}

Card::Suit Room::askForSuit(ServerPlayer *player, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_SUIT);

    AI *ai = player->getAI();
    if (ai)
        return ai->askForSuit(reason);

    bool success = doRequest(player, S_COMMAND_CHOOSE_SUIT, QVariant(), true);

    Card::Suit suit = Card::AllSuits[qrand() % 4];
    if (success) {
        const QVariant &clientReply = player->getClientReply();
        QString suitStr = clientReply.toString();
        if (suitStr == "spade")
            suit = Card::Spade;
        else if (suitStr == "club")
            suit = Card::Club;
        else if (suitStr == "heart")
            suit = Card::Heart;
        else if (suitStr == "diamond")
            suit = Card::Diamond;
    }

    return suit;
}

QString Room::askForKingdom(ServerPlayer *player)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_KINGDOM);

    AI *ai = player->getAI();
    if (ai)
        return ai->askForKingdom();

    bool success = doRequest(player, S_COMMAND_CHOOSE_KINGDOM, QVariant(), true);
    const QVariant &clientReply = player->getClientReply();
    if (success && JsonUtils::isString(clientReply)) {
        QString kingdom = clientReply.toString();
        if (Sanguosha->getKingdoms().contains(kingdom))
            return kingdom;
    }
    return "wei";
}

bool Room::askForDiscard(ServerPlayer *player, const QString &reason, int discard_num, int min_num, bool optional, bool include_equip, const QString &prompt, bool notify_skill)
{
    if (!player->isAlive())
        return false;
    tryPause();
    notifyMoveFocus(player, S_COMMAND_DISCARD_CARD);

    if (!optional) {
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        QList<int> jilei_list;
        QList<const Card *> handcards = player->getHandcards();
        foreach (const Card *card, handcards) {
            if (!player->isJilei(card))
                dummy->addSubcard(card);
            else
                jilei_list << card->getId();
        }
        if (include_equip) {
            QList<const Card *> equips = player->getEquips();
            foreach (const Card *card, equips) {
                if (!player->isJilei(card))
                    dummy->addSubcard(card);
            }
        }

        int card_num = dummy->subcardsLength();
        if (card_num <= min_num) {
            if (card_num > 0) {
                CardMoveReason movereason;
                movereason.m_playerId = player->objectName();
                movereason.m_skillName = notify_skill ? reason : QString();
                if (reason == "gamerule")
                    movereason.m_reason = CardMoveReason::S_REASON_RULEDISCARD;
                else
                    movereason.m_reason = CardMoveReason::S_REASON_THROW;

                throwCard(dummy, movereason, player);

                QVariant data;
                data = QString("%1:%2:%3").arg("cardDiscard").arg(reason).arg(dummy->toString());
                thread->trigger(ChoiceMade, this, player, data);
            }

            if (card_num < min_num && !jilei_list.isEmpty()) {
                JsonArray gongxinArgs;
                gongxinArgs << player->objectName();
                gongxinArgs << false;
                gongxinArgs << JsonUtils::toJsonArray(jilei_list);

                foreach (int cardId, jilei_list) {
                    WrappedCard *card = Sanguosha->getWrappedCard(cardId);
                    if (card->isModified())
                        broadcastUpdateCard(getOtherPlayers(player), cardId, card);
                    else
                        broadcastResetCard(getOtherPlayers(player), cardId);
                }

                LogMessage log;
                log.type = "$JileiShowAllCards";
                log.from = player;

                foreach (int card_id, jilei_list)
                    Sanguosha->getCard(card_id)->setFlags("visible");
                log.card_str = IntList2StringList(jilei_list).join("+");
                sendLog(log);

                doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
                return false;
            }
            return true;
        }
    }

    AI *ai = player->getAI();
    QList<int> to_discard;
    if (ai) {
        to_discard = ai->askForDiscard(reason, discard_num, min_num, optional, include_equip);
        if (optional && !to_discard.isEmpty())
            thread->delay();
    } else {
        JsonArray ask_str;
        ask_str << discard_num;
        ask_str << min_num;
        ask_str << optional;
        ask_str << include_equip;
        ask_str << prompt;
        ask_str << reason;
        const Skill *mainskill = Sanguosha->getMainSkill(reason);
        if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
            ask_str << getTag(mainskill->objectName() + player->objectName()).toStringList().last();

        bool success = doRequest(player, S_COMMAND_DISCARD_CARD, ask_str, true);

        //@todo: also check if the player does have that card!!!
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || ((int)clientReply.size() > discard_num || (int)clientReply.size() < min_num)
            || !JsonUtils::tryParse(clientReply, to_discard)) {
            if (optional) return false;
            // time is up, and the server choose the cards to discard
            to_discard = player->forceToDiscard(discard_num, include_equip);
        }
    }

    if (to_discard.isEmpty()) return false;

    DummyCard dummy_card(to_discard);
    if (reason == "gamerule") {
        CardMoveReason move_reason(CardMoveReason::S_REASON_RULEDISCARD, player->objectName(), QString(), reason, QString());
        throwCard(&dummy_card, move_reason, player);
    } else {
        CardMoveReason move_reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), notify_skill ? reason : QString(), QString());
        if (notify_skill)
            notifySkillInvoked(player, reason);
        throwCard(&dummy_card, move_reason, player, NULL, notify_skill ? reason : QString());
    }

    QVariant data;
    data = QString("%1:%2:%3").arg("cardDiscard").arg(reason).arg(dummy_card.toString());
    thread->trigger(ChoiceMade, this, player, data);

    return true;
}

QList<int> Room::askForExchange(ServerPlayer *player, const QString &reason, int exchange_num, int min_num, const QString &prompt, const QString &_expand_pile, const QString &pattern)
{
    if (!player->isAlive())
        return QList<int>();
    tryPause();
    notifyMoveFocus(player, S_COMMAND_EXCHANGE_CARD);

    QString  new_pattern;
    QString expand_pile = _expand_pile;
    if (expand_pile.startsWith("#"))
        expand_pile = "$" + _expand_pile.mid(1);
    if (pattern.isEmpty())
        new_pattern = ".|.|.|" + (expand_pile.isEmpty() ? "." : expand_pile);


    AI *ai = player->getAI();
    QList<int> to_exchange;
    if (ai) {
        player->setFlags("Global_AIDiscardExchanging");
        try {
            to_exchange = ai->askForExchange(reason, pattern, exchange_num, min_num, _expand_pile);
            if (min_num == 0 && !to_exchange.isEmpty())
                thread->delay();
            player->setFlags("-Global_AIDiscardExchanging");
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                player->setFlags("-Global_AIDiscardExchanging");
            throw triggerEvent;
        }
    } else {
        JsonArray exchange_str;
        exchange_str << exchange_num;
        exchange_str << min_num;
        exchange_str << prompt;
        exchange_str << _expand_pile;
        exchange_str << (pattern.isEmpty() ? new_pattern : pattern);
        exchange_str << reason;
        const Skill *mainskill = Sanguosha->getMainSkill(reason);
        if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
            exchange_str << getTag(mainskill->objectName() + player->objectName()).toStringList().last();

        bool success = doRequest(player, S_COMMAND_EXCHANGE_CARD, exchange_str, true);
        //@todo: also check if the player does have that card!!!
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || (int)clientReply.size() < min_num || (int)clientReply.size() > exchange_num || !JsonUtils::tryParse(clientReply, to_exchange)) {
            if (min_num == 0) return QList<int>();
            to_exchange = player->forceToDiscard(min_num, pattern, _expand_pile, false);
        }
    }

    if (to_exchange.isEmpty()) return QList<int>();

    QVariant data;
    data = QString("%1:%2:%3").arg("cardExchange").arg(reason).arg(IntList2StringList(to_exchange).join("+"));
    thread->trigger(ChoiceMade, this, player, data);

    return to_exchange;
}

void Room::setCardMapping(int card_id, ServerPlayer *owner, Player::Place place)
{
    owner_map.insert(card_id, owner);
    place_map.insert(card_id, place == Player::DrawPileBottom ? Player::DrawPile : place);
}

ServerPlayer *Room::getCardOwner(int card_id) const
{
    return owner_map.value(card_id);
}

Player::Place Room::getCardPlace(int card_id) const
{
    if (card_id < 0) return Player::PlaceUnknown;
    return place_map.value(card_id);
}

QList<int> Room::getCardIdsOnTable(const Card *virtual_card) const
{
    if (virtual_card == NULL)
        return QList<int>();
    if (!virtual_card->isVirtualCard()) {
        QList<int> ids;
        ids << virtual_card->getEffectiveId();
        return getCardIdsOnTable(ids);
    } else {
        return getCardIdsOnTable(virtual_card->getSubcards());
    }
    return QList<int>();
}

QList<int> Room::getCardIdsOnTable(const QList<int> &card_ids) const
{
    QList<int> r;
    foreach (int id, card_ids) {
        if (getCardPlace(id) == Player::PlaceTable)
            r << id;
    }
    return r;
}

ServerPlayer *Room::getLord(const QString &kingdom, bool include_death) const
{
    foreach (ServerPlayer *player, m_players) {
        if (player->getGeneral()->isLord() && (include_death || player->isAlive()) && player->getKingdom() == kingdom)
            return player;
    }

    return NULL;
}

void Room::askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type)
{
    QList<int> top_cards, bottom_cards;
    tryPause();
    notifyMoveFocus(zhuge, S_COMMAND_SKILL_GUANXING);


    if (guanxing_type == GuanxingUpOnly && cards.length() == 1) {
        top_cards = cards;
    } else if (guanxing_type == GuanxingDownOnly && cards.length() == 1) {
        bottom_cards = cards;
    } else {
        JsonArray stepArgs;
        stepArgs << S_GUANXING_START << zhuge->objectName() << (guanxing_type != GuanxingBothSides) << cards.length();
        doBroadcastNotify(S_COMMAND_MIRROR_GUANXING_STEP, stepArgs, zhuge);

        AI *ai = zhuge->getAI();
        if (ai) {
            ai->askForGuanxing(cards, top_cards, bottom_cards, static_cast<int>(guanxing_type));

            bool isTrustAI = zhuge->getState() == "trust";
            if (isTrustAI) {
                stepArgs[1] = QVariant();
                stepArgs[3] = JsonUtils::toJsonArray(cards);
                zhuge->notify(S_COMMAND_MIRROR_GUANXING_STEP, stepArgs);
            }

            thread->delay();
            thread->delay();

            QList<int> realtopcards = top_cards;
            QList<int> realbottomcards = bottom_cards;
            if (guanxing_type == GuanxingDownOnly) {
                realtopcards = realbottomcards;
                realbottomcards.clear();
            }

            QList<int> to_move = cards;

            if (to_move != realtopcards) {
                JsonArray movearg_base;
                movearg_base << S_GUANXING_MOVE;

                if (guanxing_type == GuanxingBothSides && !realbottomcards.isEmpty()) {
                    for (int i = 0; i < realbottomcards.length(); ++i) {
                        int id = realbottomcards.at(i);
                        int pos = to_move.indexOf(id);
                        to_move.removeOne(id);
                        JsonArray movearg = movearg_base;
                        movearg << pos + 1 << -i - 1;
                        doBroadcastNotify(S_COMMAND_MIRROR_GUANXING_STEP, movearg, isTrustAI ? NULL : zhuge);
                        thread->delay();
                    }
                }

                for (int i = 0; i < realtopcards.length() - 1; ++i) {
                    int id = realtopcards.at(i);
                    int pos = to_move.indexOf(id);

                    if (pos == i)
                        continue;

                    to_move.removeOne(id);
                    to_move.insert(i, id);
                    JsonArray movearg = movearg_base;
                    movearg << pos + 1 << i + 1;
                    doBroadcastNotify(S_COMMAND_MIRROR_GUANXING_STEP, movearg, isTrustAI ? NULL : zhuge);
                    thread->delay();
                }

                thread->delay();
                thread->delay();
            }

            if (isTrustAI) {
                JsonArray stepArgs;
                stepArgs << S_GUANXING_FINISH;
                zhuge->notify(S_COMMAND_MIRROR_GUANXING_STEP, stepArgs);
            }

        } else {
            JsonArray guanxingArgs;
            guanxingArgs << JsonUtils::toJsonArray(cards);
            guanxingArgs << (guanxing_type != GuanxingBothSides);
            bool success = doRequest(zhuge, S_COMMAND_SKILL_GUANXING, guanxingArgs, true);
            if (!success) {
                foreach (int card_id, cards) {
                    if (guanxing_type == GuanxingDownOnly)
                        m_drawPile->append(card_id);
                    else
                        m_drawPile->prepend(card_id);
                }
                return;
            }
            JsonArray clientReply = zhuge->getClientReply().value<JsonArray>();
            if (clientReply.size() == 2) {
                success &= JsonUtils::tryParse(clientReply[0], top_cards);
                success &= JsonUtils::tryParse(clientReply[1], bottom_cards);
                if (guanxing_type == GuanxingDownOnly) {
                    bottom_cards = top_cards;
                    top_cards.clear();
                }
            }
        }

        stepArgs.clear();
        stepArgs << S_GUANXING_FINISH;
        doBroadcastNotify(S_COMMAND_MIRROR_GUANXING_STEP, stepArgs, zhuge);
    }

    bool length_equal = top_cards.length() + bottom_cards.length() == cards.length();
    bool result_equal = top_cards.toSet() + bottom_cards.toSet() == cards.toSet();
    if (!length_equal || !result_equal) {
        if (guanxing_type == GuanxingDownOnly) {
            bottom_cards = cards;
            top_cards.clear();
        } else {
            top_cards = cards;
            bottom_cards.clear();
        }
    }

    if (guanxing_type == GuanxingBothSides) {
        LogMessage log;
        log.type = "#GuanxingResult";
        log.from = zhuge;
        log.arg = QString::number(top_cards.length());
        log.arg2 = QString::number(bottom_cards.length());
        sendLog(log);
    }

    if (!top_cards.isEmpty()) {
        LogMessage log;
        log.type = "$GuanxingTop";
        log.from = zhuge;
        log.card_str = IntList2StringList(top_cards).join("+");
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
    }
    if (!bottom_cards.isEmpty()) {
        LogMessage log;
        log.type = "$GuanxingBottom";
        log.from = zhuge;
        log.card_str = IntList2StringList(bottom_cards).join("+");
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
    }

    QListIterator<int> i(top_cards);
    i.toBack();
    while (i.hasPrevious())
        m_drawPile->prepend(i.previous());

    i = bottom_cards;
    while (i.hasNext())
        m_drawPile->append(i.next());

    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

    QVariant decisionData = QVariant::fromValue("guanxingViewCards:" + zhuge->objectName() + ":" + IntList2StringList(top_cards).join("+") + ":" + IntList2StringList(bottom_cards).join("+"));
    thread->trigger(ChoiceMade, this, zhuge, decisionData);
}

AskForMoveCardsStruct Room::askForMoveCards(ServerPlayer *zhuge, const QList<int> &upcards, const QList<int> &downcards, bool visible, const QString &reason,
    const QString &pattern, const QString &skillName, int min_num, int max_num, bool can_refuse, bool moverestricted, const QList<int> &notify_visible_list)
{
    QList<int> top_cards, bottom_cards, to_move;
    to_move << upcards << downcards;
    bool success = false;
    tryPause();
    notifyMoveFocus(zhuge, S_COMMAND_SKILL_MOVECARDS);

    JsonArray stepArgs;
    if (visible) {
        QList<int> notify_up, notify_down;
        foreach (int id, upcards) {
            if (notify_visible_list.isEmpty())
                notify_up << id;
            else {
                if (notify_visible_list.contains(id))
                    notify_up << id;
                else
                    notify_up << -1;
            }

        }
        foreach (int id, downcards) {
            if (notify_visible_list.isEmpty())
                notify_down << id;
            else {
                if (notify_visible_list.contains(id))
                    notify_down << id;
                else
                    notify_down << -1;
            }
        }

        stepArgs << S_GUANXING_START;
        stepArgs << zhuge->objectName();
        stepArgs << reason;
        stepArgs << JsonUtils::toJsonArray(notify_up);
        stepArgs << JsonUtils::toJsonArray(notify_down);
        stepArgs << pattern;
        stepArgs << moverestricted;
        stepArgs << min_num;
        stepArgs << max_num;
        doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, stepArgs, zhuge);
    }
    AI *ai = zhuge->getAI();
    if (ai) {
        QMap<QString, QList<int> > map = ai->askForMoveCards(upcards, downcards, reason, pattern, min_num, max_num);

        top_cards = map["top"];
        bottom_cards = map["bottom"];
        bool length_equal = top_cards.length() + bottom_cards.length() == to_move.length();
        bool result_equal = top_cards.toSet() + bottom_cards.toSet() == to_move.toSet();
        if (length_equal && result_equal)
            success = true;

        if (!can_refuse) {
            if (bottom_cards.length() < min_num) {
                if (downcards.length() >= min_num && (max_num == 0 || downcards.length() <= qMax(min_num, max_num))) {
                    bottom_cards = downcards;
                } else {
                    foreach (int id, to_move) {
                        if (!bottom_cards.contains(id) && bottom_cards.length() < min_num)
                            bottom_cards.append(id);
                    }
                }
            }
            foreach (int id, to_move) {
                if (!bottom_cards.contains(id) && !top_cards.contains(id))
                    top_cards << id;
            }
            success = true;
        }

        bool isTrustAI = zhuge->getState() == "trust";
        if (isTrustAI) {
            stepArgs[1] = QVariant();
            stepArgs[3] = JsonUtils::toJsonArray(upcards);
            stepArgs[4] = JsonUtils::toJsonArray(downcards);
            zhuge->notify(S_COMMAND_MIRROR_MOVECARDS_STEP, stepArgs);
        }
        if (success && visible) {
            thread->delay();
            thread->delay();

            if (upcards != top_cards || downcards != bottom_cards) {
                JsonArray movearg_base;
                movearg_base << S_GUANXING_MOVE;

                int fromPos;
                int toPos;
                QList<int> ups = upcards;
                QList<int> downs = downcards;
                int upcount = qMax(upcards.length(), downcards.length());

                for (int i = 0; i < top_cards.length(); ++i) {
                    fromPos = 0;
                    if (top_cards.at(i) != ups.at(i)) {
                        toPos = i + 1;
                        foreach (int id, ups) {
                            if (id == top_cards.at(i)) {
                                fromPos = ups.indexOf(id) + 1;
                                break;
                            }
                        }
                        if (fromPos != 0) {
                            ups.removeOne(top_cards.at(i));
                        } else {
                            foreach (int id, downs) {
                                if (id == top_cards.at(i)) {
                                    fromPos = -downs.indexOf(id) - 1;
                                    break;
                                }
                            }
                            downs.removeOne(top_cards.at(i));
                        }
                        QList<int> to_move, empty;
                        to_move = ups;
                        for (int c = i; c < to_move.length(); ++c) {
                            ups.removeOne(to_move.at(c));
                            empty.append(to_move.at(c));
                        }
                        ups.append(top_cards.at(i));
                        ups << empty;
                        if (ups.length() > upcount) {
                            int adjust_id = ups.last();
                            ups.removeOne(adjust_id);
                            downs.append(adjust_id);
                        }

                        JsonArray movearg = movearg_base;
                        movearg << fromPos << toPos;
                        doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, movearg, isTrustAI ? NULL : zhuge);
                        thread->delay();
                    }
                }

                if (ups.length() > top_cards.length()) {
                    int newcount = ups.length() - top_cards.length();
                    for (int i = 1; i <= newcount; ++i) {
                        fromPos = ups.length();
                        int adjust_id = ups.last();
                        ups.removeOne(adjust_id);
                        toPos = -downs.length() - 1;
                        downs.append(adjust_id);

                        JsonArray movearg = movearg_base;
                        movearg << fromPos << toPos;
                        doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, movearg, isTrustAI ? NULL : zhuge);
                        thread->delay();
                    }
                }

                for (int i = 0; i < bottom_cards.length() - 1; ++i) {
                    fromPos = 0;
                    if (bottom_cards.at(i) != downs.at(i)) {
                        toPos = -i - 1;
                        foreach (int id, downs) {
                            if (id == bottom_cards.at(i)) {
                                fromPos = -downs.indexOf(id) - 1;
                                break;
                            }
                        }
                        downs.removeOne(bottom_cards.at(i));

                        QList<int> to_move, empty;
                        to_move = downs;
                        for (int c = i; c < to_move.length(); ++c) {
                            downs.removeOne(to_move.at(c));
                            empty.append(to_move.at(c));
                        }
                        downs.append(bottom_cards.at(i));
                        downs << empty;

                        JsonArray movearg = movearg_base;
                        movearg << fromPos << toPos;
                        doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, movearg, isTrustAI ? NULL : zhuge);
                        thread->delay();
                    }
                }
            }
            thread->delay();
            thread->delay();
        }

        if (isTrustAI) {
            JsonArray stepArgs;
            stepArgs << S_GUANXING_FINISH;
            zhuge->notify(S_COMMAND_MIRROR_MOVECARDS_STEP, stepArgs);
        }
    } else {
        JsonArray CardChooseArgs;
        CardChooseArgs << JsonUtils::toJsonArray(upcards);
        CardChooseArgs << JsonUtils::toJsonArray(downcards);
        CardChooseArgs << reason;
        CardChooseArgs << pattern;
        CardChooseArgs << skillName;
        CardChooseArgs << moverestricted;
        CardChooseArgs << min_num;
        CardChooseArgs << max_num;
        CardChooseArgs << can_refuse;
        const Skill *mainskill = Sanguosha->getMainSkill(skillName);
        if (mainskill && !getTag(mainskill->objectName() + zhuge->objectName()).toStringList().isEmpty())
            CardChooseArgs << getTag(mainskill->objectName() + zhuge->objectName()).toStringList().last();

        success = doRequest(zhuge, S_COMMAND_SKILL_MOVECARDS, CardChooseArgs, true);
        if (success) {
            JsonArray clientReply = zhuge->getClientReply().value<JsonArray>();
            if (clientReply.size() == 2) {
                success &= JsonUtils::tryParse(clientReply[0], top_cards);
                success &= JsonUtils::tryParse(clientReply[1], bottom_cards);
            }
            bool length_equal = top_cards.length() + bottom_cards.length() == to_move.length();
            bool result_equal = top_cards.toSet() + bottom_cards.toSet() == to_move.toSet();
            if (!length_equal || !result_equal)
                success = false;
        }

        if (!can_refuse) {
            if (bottom_cards.length() < min_num) {
                if (downcards.length() >= min_num && (max_num == 0 || downcards.length() <= qMax(min_num, max_num))) {
                    bottom_cards = downcards;
                } else {
                    foreach (int id, to_move) {
                        if (!bottom_cards.contains(id) && bottom_cards.length() < min_num)
                            bottom_cards.append(id);
                    }
                }
            }
            foreach (int id, to_move) {
                if (!bottom_cards.contains(id) && !top_cards.contains(id))
                    top_cards << id;
            }
            success = true;
        }
    }

    stepArgs.clear();
    stepArgs << S_GUANXING_FINISH;
    doBroadcastNotify(S_COMMAND_MIRROR_MOVECARDS_STEP, stepArgs, zhuge);

    if (!success) {
        top_cards.clear();
        bottom_cards.clear();
    }

    QVariant decisionData = QVariant::fromValue(reason + "chose:" + zhuge->objectName() + ":" + IntList2StringList(top_cards).join("+") + ":" + IntList2StringList(bottom_cards).join("+"));
    thread->trigger(ChoiceMade, this, zhuge, decisionData);
    AskForMoveCardsStruct returns;
    returns.top = top_cards;
    returns.bottom = bottom_cards;
    returns.is_success = success;
    return returns;
}

int Room::doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target, QList<int> enabled_ids, const QString &skill_name)
{
    Q_ASSERT(!target->isKongcheng());
    tryPause();
    notifyMoveFocus(shenlvmeng, S_COMMAND_SKILL_GONGXIN);

    LogMessage log;
    log.type = "$ViewAllCards";
    log.from = shenlvmeng;
    log.to << target;
    log.card_str = IntList2StringList(target->handCards()).join("+");
    doNotify(shenlvmeng, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());

    QVariant decisionData = QVariant::fromValue("viewCards:" + shenlvmeng->objectName() + ":" + target->objectName());
    thread->trigger(ChoiceMade, this, shenlvmeng, decisionData);

    shenlvmeng->tag[skill_name] = QVariant::fromValue(target);
    int card_id;
    AI *ai = shenlvmeng->getAI();
    if (ai) {
        QList<int> hearts;
        foreach (int id, target->handCards()) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart)
                hearts << id;
        }
        if (enabled_ids.isEmpty()) {
            shenlvmeng->tag.remove(skill_name);
            return -1;
        }
        card_id = ai->askForAG(enabled_ids, true, skill_name);
        if (card_id == -1) {
            shenlvmeng->tag.remove(skill_name);
            return -1;
        }
    } else {
        foreach (int cardId, target->handCards()) {
            WrappedCard *card = Sanguosha->getWrappedCard(cardId);
            if (card->isModified())
                notifyUpdateCard(shenlvmeng, cardId, card);
            else
                notifyResetCard(shenlvmeng, cardId);
        }

        JsonArray gongxinArgs;
        gongxinArgs << target->objectName();
        gongxinArgs << true;
        gongxinArgs << JsonUtils::toJsonArray(target->handCards());
        gongxinArgs << JsonUtils::toJsonArray(enabled_ids);
        bool success = doRequest(shenlvmeng, S_COMMAND_SKILL_GONGXIN, gongxinArgs, true);
        const QVariant &clientReply = shenlvmeng->getClientReply();
        if (!success || !JsonUtils::isNumber(clientReply) || !target->handCards().contains(clientReply.toInt())) {
            shenlvmeng->tag.remove(skill_name);
            return -1;
        }

        card_id = clientReply.toInt();
    }
    return card_id; // Do remember to remove the tag later!
}

QList<const Card *> Room::askForPindianRace(ServerPlayer *from,const QList<ServerPlayer *> &to, const QString &reason, const Card *card)
{
    QList<const Card *> cards;
    for (int i = 0; i < to.length(); i ++)
        cards << NULL;
    if (!from->isAlive())
        return cards << NULL;
    Q_ASSERT(!from->isKongcheng());
    QStringList names;
    foreach (ServerPlayer *p, to) {
        Q_ASSERT(!p->isKongcheng());
        names << p->objectName();
        if (!p->isAlive())
            return cards << NULL;
    }

    tryPause();
    Countdown countdown;
    countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_PINDIAN, S_CLIENT_INSTANCE);
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    notifyMoveFocus(QList<ServerPlayer *>() << from << to, countdown);

    JsonArray stepArgs;
    stepArgs << S_GUANXING_START;
    stepArgs << from->objectName();
    stepArgs << reason;
    stepArgs << JsonUtils::toJsonArray(names);
    doBroadcastNotify(S_COMMAND_PINDIAN, stepArgs);

    const Card *from_card = card;
    QList<ServerPlayer *> players;

    if (from->getHandcardNum() == 1)
        from_card = from->getHandcards().first();

    AI *ai;
    if (!from_card) {
        ai = from->getAI();
        if (ai)
            from_card = ai->askForPindian(from, reason);
        else
            players << from;
    }
    if (from_card) {
        stepArgs.clear();
        stepArgs << S_GUANXING_MOVE;
        stepArgs << from->objectName();
        stepArgs << from_card->getEffectiveId();
        doBroadcastNotify(S_COMMAND_PINDIAN, stepArgs);
    }

    QStringList to_names;
    bool check = true;
    for (int i = 0; i < to.length(); i ++) {
        to_names << to.at(i)->objectName();
        if (to.at(i)->getHandcardNum() == 1)
            cards[i] = to.at(i)->getHandcards().first();
        else {
            ai = to.at(i)->getAI();
            if (ai)
                cards[i] = ai->askForPindian(from, reason);
            else
                players << to.at(i);
        }
        if (cards.at(i)) {
            stepArgs.clear();
            stepArgs << S_GUANXING_MOVE;
            stepArgs << to.at(i)->objectName();
            stepArgs << cards.at(i)->getEffectiveId();
            doBroadcastNotify(S_COMMAND_PINDIAN, stepArgs);
        } else
            check = false;
    }

    if (from_card && check) {
        thread->delay();
        return QList<const Card *>() << from_card << cards;
    }


    foreach (ServerPlayer *p, players) {
        JsonArray arr;
        arr << from->objectName() << to_names.join("+");
        p->m_commandArgs = arr;
    }

    doBroadcastRequest(players, S_COMMAND_PINDIAN);

    foreach (ServerPlayer *player, players) {
        const Card *c = NULL;
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!player->m_isClientResponseReady || clientReply.isEmpty() || !JsonUtils::isString(clientReply[0])) {
            int card_id = player->getRandomHandCardId();
            c = Sanguosha->getCard(card_id);
        } else {
            const Card *card = Card::Parse(clientReply[0].toString());
            if (card->isVirtualCard()) {
                const Card *real_card = Sanguosha->getCard(card->getEffectiveId());
                delete card;
                c = real_card;
            } else
                c = card;
        }

        stepArgs.clear();
        stepArgs << S_GUANXING_MOVE;
        stepArgs << player->objectName();
        stepArgs << c->getEffectiveId();
        doBroadcastNotify(S_COMMAND_PINDIAN, stepArgs);

        if (player == from)
            from_card = c;
        else {
            for (int i = 0; i < to.length(); i ++) {
                if (to.at(i) == player)
                    cards[i] = c;
            }
        }
    }
    return QList<const Card *>() << from_card << cards;
}

ServerPlayer *Room::askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &skillName,
    const QString &prompt, bool optional, bool notify_skill)
{
    if (targets.isEmpty()) {
        Q_ASSERT(optional);
        return NULL;
    } else if (targets.length() == 1 && !optional) {
        QVariant data = QString("%1:%2:%3").arg("playerChosen").arg(skillName).arg(targets.first()->objectName());
        thread->trigger(ChoiceMade, this, player, data);
        return targets.first();
    }

    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_PLAYER);
    AI *ai = player->getAI();
    ServerPlayer *choice = NULL;
    if (ai) {
        choice = ai->askForPlayersChosen(targets, skillName, 1, optional ? 0 : 1).first();
        if (choice && notify_skill)
            thread->delay();
    } else {
        JsonArray req;
        JsonArray req_targets;
        foreach (ServerPlayer *target, targets)
            req_targets << target->objectName();
        req << QVariant(req_targets);
        req << skillName;
        req << prompt;
        req << 1;
        req << (optional ? 0 : 1);
        const Skill *mainskill = Sanguosha->getMainSkill(skillName);
        if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
            req << getTag(mainskill->objectName() + player->objectName()).toStringList().last();

        bool success = doRequest(player, S_COMMAND_CHOOSE_PLAYER, req, true);

        const QVariant &clientReply = player->getClientReply();
        if (success && JsonUtils::isString(clientReply))
            choice = findChild<ServerPlayer *>(clientReply.toString());
    }
    if (choice && !targets.contains(choice))
        choice = NULL;
    if (choice == NULL && !optional)
        choice = targets.at(qrand() % targets.length());
    if (choice) {
        if (notify_skill) {
            notifySkillInvoked(player, skillName);
            QVariant decisionData = QVariant::fromValue("skillInvoke:" + skillName + ":yes");
            thread->trigger(ChoiceMade, this, player, decisionData);

            doAnimate(S_ANIMATE_INDICATE, player->objectName(), choice->objectName());
            LogMessage log;
            log.type = "#ChoosePlayerWithSkill";
            log.from = player;
            log.to << choice;
            log.arg = skillName;
            sendLog(log);
        }
        QVariant data = QString("%1:%2:%3").arg("playerChosen").arg(skillName).arg(choice->objectName());
        thread->trigger(ChoiceMade, this, player, data);
    }
    return choice;
}

QList<ServerPlayer *> Room::askForPlayersChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &skillName, int min_num, int max_num, const QString &prompt, bool notify_skill)
{
    if (targets.length() <= min_num) {
        QStringList names;
        foreach (ServerPlayer *p, targets)
            names.append(p->objectName());
        QVariant data = QString("%1:%2:%3").arg("playerChosen").arg(skillName).arg(names.join("+"));
        thread->trigger(ChoiceMade, this, player, data);
        return targets;
    }

    tryPause();
    min_num = qMin(min_num, targets.length());
    max_num = qMin(max_num, targets.length());
    notifyMoveFocus(player, S_COMMAND_CHOOSE_PLAYER);
    AI *ai = player->getAI();
    QList<ServerPlayer *> result;
    if (ai) {
        result = ai->askForPlayersChosen(targets, skillName, max_num, min_num);
        if (!result.isEmpty() && notify_skill)
            thread->delay();
    } else {
        JsonArray req;
        JsonArray req_targets;
        foreach (ServerPlayer *target, targets)
            req_targets << target->objectName();
        req << QVariant(req_targets);
        req << skillName;
        req << prompt;
        req << max_num;
        req << min_num;
        const Skill *mainskill = Sanguosha->getMainSkill(skillName);
        if (mainskill && !getTag(mainskill->objectName() + player->objectName()).toStringList().isEmpty())
            req << getTag(mainskill->objectName() + player->objectName()).toStringList().last();

        bool success = doRequest(player, S_COMMAND_CHOOSE_PLAYER, req, true);

        const QVariant &clientReply = player->getClientReply();
        if (success && JsonUtils::isString(clientReply)) {
            foreach (const QString &name, clientReply.toString().split("+")) {
                if (targets.contains(findChild<ServerPlayer *>(name)))
                    result << findChild<ServerPlayer *>(name);
            }
        }
    }
    if (result.length() < min_num) {
        QList<ServerPlayer *> copy = targets;
        foreach (ServerPlayer *p, result)
            copy.removeOne(p);
        while (result.length() < min_num)
            result << copy.takeAt(qrand() % copy.length());

    }
    if (!result.isEmpty()) {
        if (notify_skill) {
            notifySkillInvoked(player, skillName);
            QVariant decisionData = QVariant::fromValue("skillInvoke:" + skillName + ":yes");
            thread->trigger(ChoiceMade, this, player, decisionData);
            foreach (ServerPlayer *choice, result)
                doAnimate(S_ANIMATE_INDICATE, player->objectName(), choice->objectName());
            LogMessage log;
            log.type = "#ChoosePlayerWithSkill";
            log.from = player;
            log.to << result;
            log.arg = skillName;
            sendLog(log);
        }
        QStringList names;
        foreach (ServerPlayer *p, result)
            names.append(p->objectName());
        QVariant data = QString("%1:%2:%3").arg("playerChosen").arg(skillName).arg(names.join("+"));
        thread->trigger(ChoiceMade, this, player, data);
    }
    return result;
}

QString Room::askForGeneral(ServerPlayer *player, const QStringList &generals, const QString &_default_choice, bool single_result, const QString &skill_name, const QVariant &data, bool can_convert)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_GENERAL);

    if (generals.length() == 1)
        return generals.first();

    if (!single_result && generals.length() == 2)
        return generals.join("+");

    QString default_choice = _default_choice;

    if (default_choice.isEmpty()) {
        default_choice = generals.at(qrand() % generals.length());

        if (!single_result) {
            QStringList heros = generals;
            bool good = false;
            foreach (QString name1, heros) {
                foreach (QString name2, heros) {
                    if (name1 != name2 && Sanguosha->getGeneral(name1)->getKingdom() == Sanguosha->getGeneral(name2)->getKingdom()) {
                        default_choice = name1 + "+" + name2;
                        good = true;
                        break;
                    }
                }
                if (good) break;
            }
        }
    }

    AI *ai = player->getAI();
    if (ai != NULL && !skill_name.isEmpty()) {
        QStringList general = ai->askForChoice(skill_name, generals.join("+"), data).split("+");
        thread->delay();
        bool check = true;
        if (!single_result && general.length() != 2) check = false;
        if (single_result && general.length() !=1) check = false;
        foreach (QString name, general) {
            if (!generals.contains(name)) {
                check = false;
                break;
            }
        }
        if (check) return general.join("+");
    } else if (player->isOnline()) {
        JsonArray options;
        options << JsonUtils::toJsonArray(generals);
        options << single_result;
        options << can_convert;
        bool success = doRequest(player, S_COMMAND_CHOOSE_GENERAL, options, true);

        const QVariant &clientResponse = player->getClientReply();
        QStringList answer = clientResponse.toString().split("+");
        bool valid = true;
        foreach (const QString &name, answer) {
            if (!generals.contains(Sanguosha->getMainGenerals(name))) {
                valid = false;
                break;
            }
        }
        if (!success || !JsonUtils::isString(clientResponse) || (!Config.FreeChoose && !valid))
            return default_choice;
        else
            return clientResponse.toString();
    }

    return default_choice;
}

QString Room::askForGeneral(ServerPlayer *player, const QString &generals, const QString &default_choice, bool single_result, const QString &skill_name, const QVariant &data, bool can_convert)
{
    return askForGeneral(player, generals.split("+"), default_choice, single_result, skill_name, data, can_convert); // For Lua only!!!
}

bool Room::makeCheat(ServerPlayer *player)
{
    JsonArray arg = player->m_cheatArgs.value<JsonArray>();
    player->m_cheatArgs = QVariant();
    if (arg.isEmpty() || !JsonUtils::isNumber(arg[0])) return false;

    CheatCode code = (CheatCode)arg[0].toInt();
    if (code == S_CHEAT_KILL_PLAYER) {
        JsonArray arg1 = arg[1].value<JsonArray>();
        if (!JsonUtils::isStringArray(arg1, 0, 1)) return false;
        makeKilling(arg1[0].toString(), arg1[1].toString());

    } else if (code == S_CHEAT_MAKE_DAMAGE) {
        JsonArray arg1 = arg[1].value<JsonArray>();
        if (arg1.size() != 4 || !JsonUtils::isStringArray(arg1, 0, 1)
            || !JsonUtils::isNumber(arg1[2]) || !JsonUtils::isNumber(arg1[3]))
            return false;
        makeDamage(arg1[0].toString(), arg1[1].toString(),
            (QSanProtocol::CheatCategory)arg1[2].toInt(), arg1[3].toInt());

    } else if (code == S_CHEAT_REVIVE_PLAYER) {
        if (!JsonUtils::isString(arg[1])) return false;
        makeReviving(arg[1].toString());

    } else if (code == S_CHEAT_RUN_SCRIPT) {
        if (!JsonUtils::isString(arg[1])) return false;
        QByteArray data = QByteArray::fromBase64(arg[1].toString().toLatin1());
        data = qUncompress(data);
        doScript(data);

    } else if (code == S_CHEAT_GET_ONE_CARD) {
        if (!JsonUtils::isNumber(arg[1])) return false;
        int card_id = arg[1].toInt();

        LogMessage log;
        log.type = "$CheatCard";
        log.from = player;
        log.card_str = QString::number(card_id);
        sendLog(log);

        obtainCard(player, card_id);
    }

    return true;
}

void Room::makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point)
{
    ServerPlayer *sourcePlayer = findChild<ServerPlayer *>(source);
    ServerPlayer *targetPlayer = findChild<ServerPlayer *>(target);
    if (targetPlayer == NULL) return;
    // damage
    if (nature == S_CHEAT_HP_LOSE) {
        loseHp(targetPlayer, point);
        return;
    } else if (nature == S_CHEAT_MAX_HP_LOSE) {
        loseMaxHp(targetPlayer, point);
        return;
    } else if (nature == S_CHEAT_HP_RECOVER) {
        RecoverStruct recover;
        recover.who = sourcePlayer;
        recover.recover = point;
        this->recover(targetPlayer, recover);
        return;
    } else if (nature == S_CHEAT_MAX_HP_RESET) {
        setPlayerProperty(targetPlayer, "maxhp", point);
        return;
    }

    static QMap<QSanProtocol::CheatCategory, DamageStruct::Nature> nature_map;
    if (nature_map.isEmpty()) {
        nature_map[S_CHEAT_NORMAL_DAMAGE] = DamageStruct::Normal;
        nature_map[S_CHEAT_THUNDER_DAMAGE] = DamageStruct::Thunder;
        nature_map[S_CHEAT_FIRE_DAMAGE] = DamageStruct::Fire;
    }

    if (targetPlayer == NULL) return;
    this->damage(DamageStruct("cheat", sourcePlayer, targetPlayer, point, nature_map[nature]));
}

void Room::makeKilling(const QString &killerName, const QString &victimName)
{
    ServerPlayer *killer = NULL, *victim = NULL;

    killer = findChild<ServerPlayer *>(killerName);
    victim = findChild<ServerPlayer *>(victimName);

    if (victim == NULL) return;
    if (killer == NULL) return killPlayer(victim);

    DamageStruct damage("cheat", killer, victim);
    killPlayer(victim, &damage);
}

void Room::makeReviving(const QString &name)
{
    ServerPlayer *player = findChild<ServerPlayer *>(name);
    Q_ASSERT(player);
    revivePlayer(player);
    setPlayerProperty(player, "maxhp", player->getGeneralMaxHp());
    setPlayerProperty(player, "hp", player->getMaxHp());
}

void Room::fillAG(const QList<int> &card_ids, ServerPlayer *who, const QList<int> &disabled_ids, QList<ServerPlayer *> watchers)
{
    JsonArray arg;
    arg << JsonUtils::toJsonArray(card_ids);
    arg << JsonUtils::toJsonArray(disabled_ids);

    if (who) {
        doNotify(who, S_COMMAND_FILL_AMAZING_GRACE, arg);
        if (!watchers.isEmpty()) {
            foreach (ServerPlayer *player, watchers)
                doNotify(player, S_COMMAND_FILL_AMAZING_GRACE, arg);
        }
    } else
        doBroadcastNotify(S_COMMAND_FILL_AMAZING_GRACE, arg);
}

void Room::takeAG(ServerPlayer *player, int card_id, bool move_cards)
{
    JsonArray arg;
    arg << (player ? QVariant(player->objectName()) : QVariant());
    arg << card_id;
    arg << move_cards;

    if (player) {
        CardsMoveOneTimeStruct moveOneTime;
        if (move_cards) {
            CardsMoveOneTimeStruct move;
            move.from = NULL;
            move.from_places << Player::DrawPile;
            move.to = player;
            move.to_place = Player::PlaceHand;
            move.card_ids << card_id;
            QVariant data = QVariant::fromValue(move);
            foreach (ServerPlayer *p, getAllPlayers())
                thread->trigger(BeforeCardsMove, this, p, data);
            move = data.value<CardsMoveOneTimeStruct>();
            moveOneTime = move;

            if (move.card_ids.length() > 0) {
                player->addCard(Sanguosha->getCard(card_id), Player::PlaceHand);
                setCardMapping(card_id, player, Player::PlaceHand);
                Sanguosha->getCard(card_id)->setFlags("visible");
                QList<const Card *> cards;
                cards << Sanguosha->getCard(card_id);
                filterCards(player, cards, false);
            } else {
                arg[2] = false;
            }
        }
        doBroadcastNotify(S_COMMAND_TAKE_AMAZING_GRACE, arg);
        if (move_cards && moveOneTime.card_ids.length() > 0) {
            QVariant data = QVariant::fromValue(moveOneTime);
            foreach (ServerPlayer *p, getAllPlayers())
                thread->trigger(CardsMoveOneTime, this, p, data);
        }
    } else {
        doBroadcastNotify(S_COMMAND_TAKE_AMAZING_GRACE, arg);
        if (!move_cards) return;
        LogMessage log;
        log.type = "$EnterDiscardPile";
        log.card_str = QString::number(card_id);
        sendLog(log);

        m_discardPile->prepend(card_id);
        setCardMapping(card_id, NULL, Player::DiscardPile);
    }
}

void Room::clearAG(ServerPlayer *player)
{
    if (player)
        doNotify(player, S_COMMAND_CLEAR_AMAZING_GRACE, QVariant());
    else
        doBroadcastNotify(S_COMMAND_CLEAR_AMAZING_GRACE, QVariant());
}

void Room::provide(const Card *card)
{
    Q_ASSERT(provided == NULL);
    Q_ASSERT(!has_provided);

    provided = card;
    has_provided = true;
}

QList<ServerPlayer *> Room::getLieges(const QString &kingdom, ServerPlayer *lord) const
{
    if (lord && lord->getRole() == "careerist") return QList<ServerPlayer *>();
    QList<ServerPlayer *> lieges;
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player != lord && player->hasShownOneGeneral() && player->getKingdom() == kingdom && player->getRole() != "careerist")
            lieges << player;
    }

    return lieges;
}

void Room::sendLog(const LogMessage &log)
{
    if (log.type.isEmpty())
        return;

    doBroadcastNotify(S_COMMAND_LOG_SKILL, log.toVariant());
}

void Room::sendCompulsoryTriggerLog(ServerPlayer *player, const QString &skill_name, bool notify_skill)
{
    if (player->tag.value("JustShownSkill", QString()) != skill_name) {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.arg = skill_name;
        log.from = player;
        sendLog(log);
    }
    if (notify_skill)
        notifySkillInvoked(player, skill_name);
}


void Room::showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer)
{
    if (getCardOwner(card_id) != player) return;

    tryPause();
    notifyMoveFocus(player);
    JsonArray show_arg;
    show_arg << player->objectName();
    show_arg << card_id;

    WrappedCard *card = Sanguosha->getWrappedCard(card_id);
    bool modified = card->isModified();
    if (only_viewer) {
        QList<ServerPlayer *>players;
        players << only_viewer << player;
        if (modified)
            notifyUpdateCard(only_viewer, card_id, card);
        else
            notifyResetCard(only_viewer, card_id);
        doBroadcastNotify(players, S_COMMAND_SHOW_CARD, show_arg);
    } else {
        if (card_id > 0)
            Sanguosha->getCard(card_id)->setFlags("visible");
        if (modified)
            broadcastUpdateCard(getOtherPlayers(player), card_id, card);
        else
            broadcastResetCard(getOtherPlayers(player), card_id);
        doBroadcastNotify(S_COMMAND_SHOW_CARD, show_arg);
    }
}

void Room::showAllCards(ServerPlayer *player, ServerPlayer *to)
{
    if (player->isKongcheng())
        return;
    tryPause();

    JsonArray gongxinArgs;
    gongxinArgs << player->objectName();
    gongxinArgs << false;
    gongxinArgs << JsonUtils::toJsonArray(player->handCards());

    bool isUnicast = (to != NULL);

    foreach (int cardId, player->handCards()) {
        WrappedCard *card = Sanguosha->getWrappedCard(cardId);
        if (card->isModified()) {
            if (isUnicast)
                notifyUpdateCard(to, cardId, card);
            else
                broadcastUpdateCard(getOtherPlayers(player), cardId, card);
        } else {
            if (isUnicast)
                notifyResetCard(to, cardId);
            else
                broadcastResetCard(getOtherPlayers(player), cardId);
        }
    }

    if (isUnicast) {
        LogMessage log;
        log.type = "$ViewAllCards";
        log.from = to;
        log.to << player;
        log.card_str = IntList2StringList(player->handCards()).join("+");
        doNotify(to, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());

        QVariant decisionData = QVariant::fromValue("viewCards:" + to->objectName() + ":" + player->objectName());
        thread->trigger(ChoiceMade, this, to, decisionData);

        doNotify(to, S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    } else {
        LogMessage log;
        log.type = "$ShowAllCards";
        log.from = player;
        foreach (int card_id, player->handCards())
            Sanguosha->getCard(card_id)->setFlags("visible");
        log.card_str = IntList2StringList(player->handCards()).join("+");
        sendLog(log);

        doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    }
}

void Room::retrial(const Card *card, ServerPlayer *player, JudgeStruct *judge, const QString &skill_name, bool exchange)
{
    if (card == NULL) return;

    bool triggerResponded = getCardOwner(card->getEffectiveId()) == player;
    bool isHandcard = (triggerResponded && getCardPlace(card->getEffectiveId()) == Player::PlaceHand);

    const Card *oldJudge = judge->card;
    judge->card = Sanguosha->getCard(card->getEffectiveId());

    CardsMoveStruct move1(QList<int>(), judge->who, Player::PlaceJudge,
        CardMoveReason(CardMoveReason::S_REASON_RETRIAL, player->objectName(), skill_name, QString()));

    move1.card_ids.append(card->getEffectiveId());
    int reasonType;
    if (exchange)
        reasonType = CardMoveReason::S_REASON_OVERRIDE;
    else
        reasonType = CardMoveReason::S_REASON_JUDGEDONE;

    CardMoveReason reason(reasonType, player->objectName(), exchange ? skill_name : QString(), QString());
    CardsMoveStruct move2(QList<int>(), judge->who, exchange ? player : NULL, Player::PlaceUnknown, exchange ? Player::PlaceHand : Player::DiscardPile, reason);

    move2.card_ids.append(oldJudge->getEffectiveId());

    LogMessage log;
    log.type = "$ChangedJudge";
    log.arg = skill_name;
    log.from = player;
    log.to << judge->who;
    log.card_str = QString::number(card->getEffectiveId());
    sendLog(log);

    QList<CardsMoveStruct> moves;
    moves.append(move1);
    moves.append(move2);
    moveCardsAtomic(moves, true);
    //judge->updateResult();

    if (triggerResponded) {
        CardResponseStruct resp(card, judge->who);
        resp.m_isHandcard = isHandcard;
        resp.m_isRetrial = true;
        QVariant data = QVariant::fromValue(resp);
        thread->trigger(CardResponded, this, player, data);
    }
}

bool Room::askForYiji(ServerPlayer *guojia, QList<int> &cards, const QString &skill_name,
    bool is_preview, bool visible, bool optional, int max_num,
    QList<ServerPlayer *> players, CardMoveReason reason, const QString &prompt,
    const QString &expand_pile, bool notify_skill)
{
    if (max_num == -1)
        max_num = cards.length();
    if (players.isEmpty())
        players = getOtherPlayers(guojia);
    if (cards.isEmpty() || max_num == 0)
        return false;
    if (reason.m_reason == CardMoveReason::S_REASON_UNKNOWN) {
        reason.m_playerId = guojia->objectName();
        // when we use ? : here, compiling error occurs under debug mode...
        if (is_preview)
            reason.m_reason = CardMoveReason::S_REASON_PREVIEWGIVE;
        else
            reason.m_reason = CardMoveReason::S_REASON_GIVE;
    }
    tryPause();
    notifyMoveFocus(guojia, S_COMMAND_SKILL_YIJI);

    ServerPlayer *target = NULL;
    QList<int> ids;
    AI *ai = guojia->getAI();
    do {
        if (ai) {
            int card_id;
            ServerPlayer *who = ai->askForYiji(cards, skill_name, card_id);
            if (!who)
                break;
            else {
                target = who;
                ids << card_id;
            }
        } else {
            JsonArray arg;
            arg << JsonUtils::toJsonArray(cards);
            arg << optional;
            arg << max_num;
            JsonArray player_names;
            foreach (ServerPlayer *player, players)
                player_names << player->objectName();
            arg << QVariant(player_names);
            arg << expand_pile;
            if (!prompt.isEmpty())
                arg << prompt;
            bool success = doRequest(guojia, S_COMMAND_SKILL_YIJI, arg, true);

            //Validate client response
            JsonArray clientReply = guojia->getClientReply().value<JsonArray>();
            if (!success || clientReply.size() != 2)
                break;

            if (!JsonUtils::tryParse(clientReply[0], ids) || !JsonUtils::isString(clientReply[1]))
                break;

            foreach (int id, ids) {
                if (!cards.contains(id))
                    break;
            }

            ServerPlayer *who = findChild<ServerPlayer *>(clientReply[1].toString());
            if (!who)
                break;
            else
                target = who;
        }
    } while (false);

    if (target == NULL) {
        if (optional)
            return false;
        else {
            ids.clear();
            ids << cards.at(qrand() % cards.length());
            target = players.at(qrand() % players.length());
        }
    }

    Q_ASSERT(target != NULL);

    DummyCard dummy_card;
    foreach (int card_id, ids) {
        cards.removeOne(card_id);
        dummy_card.addSubcard(card_id);
    }

    QVariant decisionData = QVariant::fromValue(QString("Yiji:%1:%2:%3:%4")
        .arg(skill_name).arg(guojia->objectName()).arg(target->objectName())
        .arg(IntList2StringList(ids).join("+")));
    thread->trigger(ChoiceMade, this, guojia, decisionData);

    if (notify_skill) {
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = guojia;
        log.arg = skill_name;
        sendLog(log);

        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill)
            broadcastSkillInvoke(skill_name, skill->getEffectIndex(target, &dummy_card), guojia);
        notifySkillInvoked(guojia, skill_name);
    }

    guojia->setFlags("Global_GongxinOperator");
    foreach (int id, dummy_card.getSubcards())
        moveCardTo(Sanguosha->getCard(id), target, Player::PlaceHand, reason, visible);

    guojia->setFlags("-Global_GongxinOperator");

    return true;
}

QString Room::generatePlayerName()
{
    static unsigned int id = 0;
    id++;
    return QString("sgs%1").arg(id);
}

QString Room::askForOrder(ServerPlayer *player)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_ORDER);

    bool success = doRequest(player, S_COMMAND_CHOOSE_ORDER, (int)S_REASON_CHOOSE_ORDER_TURN, true);

    Game3v3Camp result = qrand() % 2 == 0 ? S_CAMP_WARM : S_CAMP_COOL;
    const QVariant &clientReply = player->getClientReply();
    if (success && JsonUtils::isNumber(clientReply))
        result = (Game3v3Camp)clientReply.toInt();
    return (result == S_CAMP_WARM) ? "warm" : "cool";
}

void Room::networkDelayTestCommand(ServerPlayer *player, const QVariant &)
{
    qint64 delay = player->endNetworkDelayTest();
    QString reportStr = tr("<font color=#EEB422>The network delay of player <b>%1</b> is %2 milliseconds.</font>")
        .arg(player->screenName()).arg(QString::number(delay));
    speakCommand(player, reportStr);
}

void Room::sortByActionOrder(QList<ServerPlayer *> &players)
{
    if (players.length() > 1)
        qSort(players.begin(), players.end(), ServerPlayer::CompareByActionOrder);
}

void Room::cancelTarget(CardUseStruct &use, const QString &name)
{
    if (use.to.isEmpty())
        return;
    cancelTarget(use, use.to.first()->getRoom()->findPlayer(name)); // use.from can be NULL, but use.to must not have a value that is NULL
}

void Room::cancelTarget(CardUseStruct &use, ServerPlayer *player)
{
    if (player == NULL)
        return;

    Room *room = player->getRoom();
    LogMessage log;
    if (use.from) {
        log.type = "$CancelTarget";
        log.from = use.from;
    } else {
        log.type = "$CancelTargetNoUser";
    }
    log.to << player;
    log.arg = use.card->objectName();
    room->sendLog(log);

    room->setEmotion(player, "cancel");

    use.to.removeOne(player);

    if (use.card != NULL && use.card->isKindOf("Slash"))
        player->slashSettlementFinished(use.card);
}
