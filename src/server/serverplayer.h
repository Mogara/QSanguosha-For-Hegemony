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

#ifndef _SERVER_PLAYER_H
#define _SERVER_PLAYER_H

class Room;
class ServerClient;
class AI;

class CardMoveReason;
struct PhaseStruct;
struct PindianStruct;

#include "structs.h"
#include "player.h"
#include "protocol.h"
#include "namespace.h"

#include <QSemaphore>
#include <QDateTime>

#ifndef QT_NO_DEBUG
#include <QEvent>

class ServerPlayerEvent: public QEvent {
public:
    ServerPlayerEvent(char *property_name, QVariant &value);

    char *property_name;
    QVariant value;
};
#endif

class ServerPlayer : public Player
{
    Q_OBJECT

public:
    explicit ServerPlayer(Room *room);
    ~ServerPlayer();

    void notify(QSanProtocol::CommandType type, const QVariant &arg = QVariant());
    QString reportHeader() const;
    void drawCard(const Card *card);
    Room *getRoom() const;
    ServerClient *getClient() const;
    void setClient(ServerClient *client);
    void broadcastSkillInvoke(const Card *card) const;
    void broadcastSkillInvoke(const QString &card_name) const;
    int getRandomHandCardId() const;
    const Card *getRandomHandCard() const;
    void obtainCard(const Card *card, bool unhide = true);
    void throwAllEquips();
    void throwAllHandCards();
    void throwAllHandCardsAndEquips();
    void throwAllCards();
    void bury();
    void throwAllMarks(bool visible_only = true);
    void clearOnePrivatePile(const QString &pile_name);
    void clearPrivatePiles();
    int getMaxCards(MaxCardsType::MaxCardsCount type = MaxCardsType::Max) const;
    void drawCards(int n, const QString &reason = QString());
    bool askForSkillInvoke(const QString &skill_name, const QVariant &data = QVariant(), const QString &position = QString());
    bool askForSkillInvoke(const Skill *skill, const QVariant &data = QVariant(), const QString &position = QString());
    QList<int> forceToDiscard(int discard_num, bool include_equip, bool is_discard = true);
    QList<int> forceToDiscard(int discard_num, const QString &pattern, const QString &expand_pile , bool is_discard);
    QList<int> handCards() const;
    virtual QList<const Card *> getHandcards() const;
    QList<const Card *> getCards(const QString &flags) const;
    DummyCard *wholeHandCards() const;
    bool hasNullification() const;
    PindianStruct *pindianSelect(ServerPlayer *target, const QString &reason, const Card *card1 = NULL);
    PindianStruct *pindianSelect(const QList<ServerPlayer *> &target, const QString &reason, const Card *card1 = NULL);
    bool pindian(PindianStruct *pd, int index = 1); //pd is deleted after this function

    void turnOver();
    void play(QList<Player::Phase> set_phases = QList<Player::Phase>());
    bool changePhase(Player::Phase from, Player::Phase to);

    QList<Player::Phase> &getPhases();
    void skip(Player::Phase phase, bool sendLog = true);
    void skip(bool sendLog = true);
    void insertPhase(Player::Phase phase);
    bool isSkipped(Player::Phase phase);

    void gainMark(const QString &mark, int n = 1);
    void loseMark(const QString &mark, int n = 1);
    void loseAllMarks(const QString &mark_name);

    virtual void addSkill(const QString &skill_name, bool head_skill = true);
    virtual void loseSkill(const QString &skill_name, bool head = true);
    virtual void setGender(General::Gender gender);

    void setAI(AI *ai);
    AI *getAI() const;
    AI *getSmartAI() const;

    bool isOnline() const;
    inline bool isOffline() const
    {
        return getState() == "robot" || getState() == "offline";
    }

    virtual int aliveCount(bool includeRemoved = true) const;
    int getPlayerNumWithSameKingdom(const QString &reason, const QString &_to_calculate = QString(),
        MaxCardsType::MaxCardsCount type = MaxCardsType::Max) const;
    virtual int getHandcardNum() const;
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual bool isLastHandCard(const Card *card, bool contain = false) const;

    void addVictim(ServerPlayer *victim);
    QList<ServerPlayer *> getVictims() const;

    // 3v3 methods
    void addToSelected(const QString &general);
    QStringList getSelected() const;
    QString findReasonable(const QStringList &generals, bool no_unreasonable = false);
    void clearSelected();

    int getGeneralMaxHp() const;
    virtual QString getGameMode() const;

    void introduceTo(ServerClient *player);
    void marshal(ServerClient *player) const;

    void addToPile(const QString &pile_name, const Card *card, bool open = true, QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, int card_id, bool open = true, QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, QList<int> card_ids, bool open = true, QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void addToPile(const QString &pile_name, QList<int> card_ids, bool open, QList<ServerPlayer *> open_players, CardMoveReason reason);
    void pileAdd(const QString &pile_name, QList<int> card_ids);
    void gainAnExtraTurn();

    void copyFrom(ServerPlayer *sp);

    // static function
    static bool CompareByActionOrder(ServerPlayer *a, ServerPlayer *b);

    bool showSkill(const QString &skill_name, const QString &skill_position = QString());
    void showGeneral(bool head_general = true, bool trigger_event = true, bool sendLog = true, bool ignore_rule = true);
    void hideGeneral(bool head_general = true);
    void removeGeneral(bool head_general = true);
    void sendSkillsToOthers(bool head_skill = true);
    void disconnectSkillsFromOthers(bool head_skill = true, bool trigger = true);
    bool askForGeneralShow(bool one = true, bool refusable = false);
    void notifyPreshow(const QString &flag = "hd");

    bool inSiegeRelation(const ServerPlayer *skill_owner, const ServerPlayer *victim) const;
    bool inFormationRalation(ServerPlayer *teammate) const;
    void summonFriends(const HegemonyMode::ArrayType type);

    // remove QinggangTag and BladeDisableShow
    void slashSettlementFinished(const Card *slash);

    bool event_received;

    void changeToLord();

    void setActualGeneral1Name(const QString &name);
    void setActualGeneral2Name(const QString &name);

protected:
    //Synchronization helpers
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;
#ifndef QT_NO_DEBUG
    bool event(QEvent *event);
#endif

private:
    QList<const Card *> handcards;
    Room *room;
    ServerClient *client;
    AI *ai;
    AI *trust_ai;
    QList<ServerPlayer *> victims;
    QList<Phase> phases;
    int _m_phases_index;
    QList<PhaseStruct> _m_phases_state;
    QStringList selected; // 3v3 mode use only
    QVariant _m_clientResponse;
};

#endif
