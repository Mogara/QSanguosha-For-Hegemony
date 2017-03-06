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

#ifndef _STRUCTS_H
#define _STRUCTS_H

class Room;
class TriggerSkill;
class Card;
class Slash;

#include "player.h"
#include "serverplayer.h"
#include "namespace.h"

#include <QVariant>

struct DamageStruct
{
    enum Nature
    {
        Normal, // normal slash, duel and most damage caused by skill
        Fire,  // fire slash, fire attack and few damage skill (Yeyan, etc)
        Thunder // lightning, thunder slash, and few damage skill (Leiji, etc)
    };

    DamageStruct();
    DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);
    DamageStruct(const QString &reason, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *card;
    int damage;
    Nature nature;
    bool chain;
    bool transfer;
    bool by_user;
    QString reason;
    QString transfer_reason;
    bool prevented;

    QString getReason() const;
};

struct CardEffectStruct
{
    CardEffectStruct();

    const Card *card;

    ServerPlayer *from;
    ServerPlayer *to;

    bool multiple; // helper to judge whether the card has multiple targets
    // does not make sense if the card inherits SkillCard
    bool nullified;
};

struct SlashEffectStruct
{
    SlashEffectStruct();

    int jink_num;

    const Card *slash;
    const Card *jink;

    ServerPlayer *from;
    ServerPlayer *to;

    int drank;

    DamageStruct::Nature nature;

    bool nullified;
};

struct CardUseStruct
{
    enum CardUseReason
    {
        CARD_USE_REASON_UNKNOWN = 0x00,
        CARD_USE_REASON_PLAY = 0x01,
        CARD_USE_REASON_RESPONSE = 0x02,
        CARD_USE_REASON_RESPONSE_USE = 0x12
    } m_reason;

    CardUseStruct();
    CardUseStruct(const Card *card, ServerPlayer *from, QList<ServerPlayer *> to, bool isOwnerUse = true);
    CardUseStruct(const Card *card, ServerPlayer *from, ServerPlayer *target, bool isOwnerUse = true);
    bool isValid(const QString &pattern) const;
    void parse(const QString &str, Room *room);
    bool tryParse(const QVariant &usage, Room *room);

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    bool m_isOwnerUse;
    bool m_addHistory;
    bool m_isHandcard;
    QStringList nullified_list;
};

class CardMoveReason
{
public:
    int m_reason;
    QString m_playerId; // the cause (not the source) of the movement, such as "lusu" when "dimeng", or "zhanghe" when "qiaobian"
    QString m_targetId; // To keep this structure lightweight, currently this is only used for UI purpose.
    // It will be set to empty if multiple targets are involved. NEVER use it for trigger condition
    // judgement!!! It will not accurately reflect the real reason.
    QString m_skillName; // skill that triggers movement of the cards, such as "longdang", "dimeng"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
    QString m_position; // additional arg such as "head" or "deputy"
    QString m_cardString; // if the card moved by using/responding, then it has this
    inline CardMoveReason()
    {
        m_reason = S_REASON_UNKNOWN;
    }
    inline CardMoveReason(int moveReason, QString playerId)
    {
        m_reason = moveReason;
        m_playerId = playerId;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString targetId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_targetId = targetId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString targetId, QString skillName, QString eventName, QString position)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_targetId = targetId;
        m_skillName = skillName;
        m_eventName = eventName;
        m_position = position;
    }

    bool tryParse(const QVariant &);
    QVariant toVariant() const;

    inline bool operator == (const CardMoveReason &other) const
    {
        return m_reason == other.m_reason
            && m_playerId == other.m_playerId && m_targetId == other.m_targetId
            && m_skillName == other.m_skillName
            && m_eventName == other.m_eventName;
    }

    static const int S_REASON_UNKNOWN = 0x00;
    static const int S_REASON_USE = 0x01;
    static const int S_REASON_RESPONSE = 0x02;
    static const int S_REASON_DISCARD = 0x03;
    static const int S_REASON_RECAST = 0x04;          // ironchain etc.
    static const int S_REASON_PINDIAN = 0x05;
    static const int S_REASON_DRAW = 0x06;
    static const int S_REASON_GOTCARD = 0x07;
    static const int S_REASON_SHOW = 0x08;
    static const int S_REASON_TRANSFER = 0x09;
    static const int S_REASON_PUT = 0x0A;

    //subcategory of use
    static const int S_REASON_LETUSE = 0x11;           // use a card when self is not current

    //subcategory of response
    static const int S_REASON_RETRIAL = 0x12;

    //subcategory of discard
    static const int S_REASON_RULEDISCARD = 0x13;       //  discard at one's Player::Discard for gamerule
    static const int S_REASON_THROW = 0x23;             /*  gamerule(dying or punish)
                                                            as the cost of some skills   */
    static const int S_REASON_DISMANTLE = 0x33;         //  one throw card of another

    //subcategory of gotcard
    static const int S_REASON_GIVE = 0x17;              // from one hand to another hand
    static const int S_REASON_EXTRACTION = 0x27;        // from another's place to one's hand
    static const int S_REASON_GOTBACK = 0x37;           // from placetable to hand
    static const int S_REASON_RECYCLE = 0x47;           // from discardpile to hand
    static const int S_REASON_ROB = 0x57;               // got a definite card from other's hand
    static const int S_REASON_PREVIEWGIVE = 0x67;       // give cards after previewing, i.e. Yiji & Miji

    //subcategory of show
    static const int S_REASON_TURNOVER = 0x18;          // show n cards from drawpile
    static const int S_REASON_JUDGE = 0x28;             // show a card from drawpile for judge
    static const int S_REASON_PREVIEW = 0x38;           // Not done yet, plan for view some cards for self only(guanxing yiji miji)
    static const int S_REASON_DEMONSTRATE = 0x48;       // show a card which copy one to move to table
    static const int S_REASON_DELAYTRICK_EFFECT = 0x58;    //delay trick move to table and start judge

    //subcategory of transfer
    static const int S_REASON_SWAP = 0x19;              // exchange card for two players
    static const int S_REASON_OVERRIDE = 0x29;          // exchange cards from cards in game
    static const int S_REASON_EXCHANGE_FROM_PILE = 0x39;// exchange cards from cards moved out of game (for qixing only)

    //subcategory of put
    static const int S_REASON_NATURAL_ENTER = 0x1A;     //  a card with no-owner move into discardpile
    //  e.g. delayed trick enters discardpile
    static const int S_REASON_REMOVE_FROM_PILE = 0x2A;  //  cards moved out of game go back into discardpile
    static const int S_REASON_JUDGEDONE = 0x3A;         //  judge card move into discardpile
    static const int S_REASON_CHANGE_EQUIP = 0x4A;      //  replace existed equip

    static const int S_MASK_BASIC_REASON = 0x0F;
};

struct CardsMoveOneTimeStruct
{
    QList<int> card_ids;
    QList<Player::Place> from_places;
    Player::Place to_place;
    CardMoveReason reason;
    Player *from, *to;
    QStringList from_pile_names;
    QString to_pile_name;

    QList<Player::Place> origin_from_places;
    Player::Place origin_to_place;
    Player *origin_from, *origin_to;
    QStringList origin_from_pile_names;
    QString origin_to_pile_name; //for case of the movement transitted

    QList<bool> open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;
};

struct CardsMoveStruct
{
    inline CardsMoveStruct()
    {
        from_place = Player::PlaceUnknown;
        to_place = Player::PlaceUnknown;
        from = NULL;
        to = NULL;
        is_last_handcard = false;
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *from, Player *to, Player::Place from_place,
        Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        if (from) this->from_player_name = from->objectName();
        if (to) this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        if (to) this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *from, Player *to, Player::Place from_place,
        Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        if (from) this->from_player_name = from->objectName();
        if (to) this->to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        this->is_last_handcard = false;
        if (to) this->to_player_name = to->objectName();
    }

    inline bool operator == (const CardsMoveStruct &other) const
    {
        return from == other.from && from_place == other.from_place
            && from_pile_name == other.from_pile_name && from_player_name == other.from_player_name;
    }

    inline bool operator < (const CardsMoveStruct &other) const
    {
        return from < other.from || from_place < other.from_place
            || from_pile_name < other.from_pile_name || from_player_name < other.from_player_name;
    }

    QList<int> card_ids;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    CardMoveReason reason;
    bool open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;

    Player::Place origin_from_place, origin_to_place;
    Player *origin_from, *origin_to;
    QString origin_from_pile_name, origin_to_pile_name; //for case of the movement transitted

    bool tryParse(const QVariant &arg);
    QVariant toVariant() const;
    inline bool isRelevant(const Player *player)
    {
        return player != NULL && (from == player || (to == player && to_place != Player::PlaceSpecial));
    }
};

struct DyingStruct
{
    DyingStruct();

    ServerPlayer *who; // who is ask for help
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
};

struct DeathStruct
{
    DeathStruct();

    ServerPlayer *who; // who is dead
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
};

struct RecoverStruct
{
    RecoverStruct();

    int recover;
    ServerPlayer *who;
    const Card *card;
};

struct PindianStruct
{
    PindianStruct();
    bool isSuccess() const;

    ServerPlayer *from;
    QList<ServerPlayer *>tos;
    ServerPlayer *to;
    const Card *from_card;
    QList<const Card *> to_cards;
    const Card *to_card;
    int from_number, to_number;
    QList<int> to_numbers;
    QString reason;
    bool success;
};

struct JudgeStruct
{
    JudgeStruct();
    bool isGood() const;
    bool isBad() const;
    bool isEffected() const;
    void updateResult();

    bool isGood(const Card *card) const; // For AI

    ServerPlayer *who;
    const Card *card;
    QString pattern;
    bool good;
    QString reason;
    bool time_consuming;
    bool negative;
    bool play_animation;

private:
    enum TrialResult
    {
        TRIAL_RESULT_UNKNOWN,
        TRIAL_RESULT_GOOD,
        TRIAL_RESULT_BAD
    } _m_result;
};

struct PhaseChangeStruct
{
    PhaseChangeStruct();
    Player::Phase from;
    Player::Phase to;
};

struct PhaseStruct
{
    inline PhaseStruct()
    {
        phase = Player::PhaseNone;
        finished = false;
    }

    Player::Phase phase;
    bool finished;
};

struct CardResponseStruct
{
    inline CardResponseStruct()
    {
        m_card = NULL;
        m_who = NULL;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card)
    {
        m_card = card;
        m_who = NULL;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, ServerPlayer *who)
    {
        m_card = card;
        m_who = who;
        m_isUse = false;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, bool isUse)
    {
        m_card = card;
        m_who = NULL;
        m_isUse = isUse;
        m_isRetrial = false;
    }

    inline CardResponseStruct(const Card *card, ServerPlayer *who, bool isUse)
    {
        m_card = card;
        m_who = who;
        m_isUse = isUse;
        m_isRetrial = false;
    }

    const Card *m_card;
    ServerPlayer *m_who;
    bool m_isUse;
    bool m_isHandcard;
    bool m_isRetrial;
};

struct PlayerNumStruct
{
    inline PlayerNumStruct()
    {
        m_num = 0;
        m_toCalculate = QString();
        m_type = MaxCardsType::Max;
        m_reason = QString();
    }

    inline PlayerNumStruct(int num, const QString &toCalculate)
    {
        m_num = num;
        m_toCalculate = toCalculate;
        m_type = MaxCardsType::Max;
        m_reason = QString();
    }

    inline PlayerNumStruct(int num, const QString &toCalculate, MaxCardsType::MaxCardsCount type)
    {
        m_num = num;
        m_toCalculate = toCalculate;
        m_type = type;
        m_reason = QString();
    }

    inline PlayerNumStruct(int num, const QString &toCalculate, MaxCardsType::MaxCardsCount type, const QString &reason)
    {
        m_num = num;
        m_toCalculate = toCalculate;
        m_type = type;
        m_reason = reason;
    }

    MaxCardsType::MaxCardsCount m_type;
    int m_num;
    QString m_toCalculate;
    QString m_reason;
};

enum TriggerEvent
{
    NonTrigger,

    GameStart,
    TurnStart,
    EventPhaseStart,
    EventPhaseProceeding,
    EventPhaseEnd,
    EventPhaseChanging,
    EventPhaseSkipping,

    ConfirmPlayerNum, // hongfa only

    DrawNCards,
    AfterDrawNCards,
    DrawPileChanged,

    PreHpRecover,
    HpRecover,
    PreHpLost,
    HpChanged,
    MaxHpChanged,
    PostHpReduced,
    HpLost,

    EventLoseSkill,
    EventAcquireSkill,

    StartJudge,
    AskForRetrial,
    FinishRetrial,
    FinishJudge,

    PindianVerifying,
    Pindian,

    TurnedOver,
    ChainStateChanged,
    RemoveStateChanged,

    ConfirmDamage,    // confirm the damage's count and damage's nature
    Predamage,        // trigger the certain skill -- jueqing
    DamageForseen,    // the first event in a damage -- kuangfeng dawu
    DamageCaused,     // the moment for -- qianxi..
    DamageInflicted,  // the moment for -- tianxiang..
    PreDamageDone,    // before reducing Hp
    DamageDone,       // it's time to do the damage
    Damage,           // the moment for -- lieren..
    Damaged,          // the moment for -- yiji..
    DamageComplete,   // the moment for trigger iron chain

    Dying,
    QuitDying,
    AskForPeaches,
    AskForPeachesDone,
    Death,
    BuryVictim,
    BeforeGameOverJudge,
    GameOverJudge,
    GameFinished,

    SlashEffected,
    SlashProceed,
    SlashHit,
    SlashMissed,

    JinkEffect,

    CardAsked,
    CardResponded,
    BeforeCardsMove, // sometimes we need to record cards before the move
    CardsMoveOneTime,

    PreCardUsed,
    CardUsed,
    TargetChoosing, //distinguish "choose target" and "confirm target"
    TargetConfirming,
    TargetChosen,
    TargetConfirmed,
    CardEffect,
    CardEffected,
    CardEffectConfirmed, //after Nullification
    PostCardEffected,
    CardFinished,
    TrickCardCanceling,

    ChoiceMade,

    StageChange, // For hulao pass only
    FetchDrawPileCard, // For miniscenarios only

    TurnBroken, // For the skill 'DanShou'. Do not use it to trigger events

    GeneralShown, // For Official Hegemony mode
    GeneralHidden, // For Official Hegemony mode
    GeneralStartRemove, // For Official Hegemony mode
    GeneralRemoved, // For Official Hegemony mode

    DFDebut, // for Dragon Phoenix Debut

    NumOfEvents
};

struct LogMessage
{
    LogMessage();
    QString toString() const;
    QVariant toVariant() const;

    QString type;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    QString card_str;
    QString arg;
    QString arg2;
};

struct AskForMoveCardsStruct
{
    AskForMoveCardsStruct();

    QList<int> top;
    QList<int> bottom;

    bool is_success;
};

struct TriggerStruct
{
    TriggerStruct();
    TriggerStruct(const QString &skill_name);
    TriggerStruct(const QString &skill_name, ServerPlayer *invoker);
    TriggerStruct(const QString &skill_name, ServerPlayer *invoker, ServerPlayer *skill_owner);
    TriggerStruct(const QString &skill_name, ServerPlayer *invoker, QList<ServerPlayer *> targets);

    QVariant toVariant() const;
    bool tryParse(const QVariant &trigger);

    QString skill_name;
    QString invoker;
    QStringList targets;
    QString skill_owner;
    QString skill_position;
    int times;
    QString result_target;
};

struct PromoteStruct
{
    PromoteStruct();
    PromoteStruct(const QString &skill_name, const QString &pattern, CardUseStruct::CardUseReason reason, const QString &skill_position);

    QVariant toVariant() const;
    bool tryParse(const QVariant &ask_str);

    QString skill_name;
    QString pattern;
    CardUseStruct::CardUseReason reason;
    QString skill_position;
};

struct InfoStruct
{
    InfoStruct();
    InfoStruct(const QString &info, bool head);
    QString info;
    bool head;
};

Q_DECLARE_METATYPE(DamageStruct)
Q_DECLARE_METATYPE(CardEffectStruct)
Q_DECLARE_METATYPE(SlashEffectStruct)
Q_DECLARE_METATYPE(CardUseStruct)
Q_DECLARE_METATYPE(CardsMoveStruct)
Q_DECLARE_METATYPE(CardsMoveOneTimeStruct)
Q_DECLARE_METATYPE(DyingStruct)
Q_DECLARE_METATYPE(DeathStruct)
Q_DECLARE_METATYPE(RecoverStruct)
Q_DECLARE_METATYPE(PhaseChangeStruct)
Q_DECLARE_METATYPE(CardResponseStruct)
Q_DECLARE_METATYPE(PlayerNumStruct)
Q_DECLARE_METATYPE(const Card *)
Q_DECLARE_METATYPE(ServerPlayer *)
Q_DECLARE_METATYPE(JudgeStruct *)
Q_DECLARE_METATYPE(PindianStruct *)
Q_DECLARE_METATYPE(AskForMoveCardsStruct)
Q_DECLARE_METATYPE(TriggerStruct)
Q_DECLARE_METATYPE(PromoteStruct)
Q_DECLARE_METATYPE(InfoStruct)
#endif

