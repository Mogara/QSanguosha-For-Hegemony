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

#include "transformation.h"
#include "standard-wu-generals.h"
#include "standard-basics.h"
#include "standard-tricks.h"
#include "client.h"
#include "engine.h"
#include "structs.h"
#include "gamerule.h"
#include "settings.h"
#include "roomthread.h"
#include "json.h"

//xunyou
class Zhiyu : public MasochismSkill
{
public:
    Zhiyu() : MasochismSkill("zhiyu")
    {
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *xunyu, QVariant &, ServerPlayer *) const
    {
        if (MasochismSkill::triggerable(xunyu))
            return TriggerStruct(objectName(), xunyu);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *xunyu, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (xunyu->askForSkillInvoke(this, data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, xunyu, info.skill_position);
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &damage, const TriggerStruct &) const
    {
        Room *room = xunyu->getRoom();
        int aidelay = Config.AIDelay;
        Config.AIDelay = 0;
        xunyu->drawCards(1, objectName());
        room->showAllCards(xunyu);
        bool same = true;
        bool isRed = xunyu->getHandcards().first()->isRed();
        foreach (const Card *card, xunyu->getHandcards()) {
            if (card->isRed() != isRed) {
                same = false;
                break;
            }
        }
        Config.AIDelay = aidelay;
        if (same && damage.from && !damage.from->isKongcheng() && damage.from->canDiscard(damage.from, "h")) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, xunyu->objectName(), damage.from->objectName());
            room->askForDiscard(damage.from, objectName(), 1, 1);
        }
    }
};

QiceCard::QiceCard()
{
    will_throw = false;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    //Card *mutable_card = Sanguosha->cloneCard(Self->tag["qice"].toString());
    Card *mutable_card = Sanguosha->cloneCard(getUserString());
    if (mutable_card) {
        mutable_card->addSubcards(subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    if (targets.length() >= subcards.length() && !mutable_card->isKindOf("Collateral")) return false;

    if (mutable_card->isKindOf("AllianceFeast")) {
        if (to_select->getRole() == "careerist") {
            if (subcards.length() < 2)
                return false;
        } else {
            QList<const Player *> targets;
            foreach (const Player *p, Self->getAliveSiblings())
                if (p->isFriendWith(to_select) && !Self->isProhibited(p, mutable_card))
                    targets << p;
            if (targets.length() > subcards.length() - 1) return false;
        }
    }

    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool QiceCard::targetFixed() const
{
    Card *mutable_card = Sanguosha->cloneCard(getUserString());
    if (mutable_card) {
        mutable_card->addSubcards(subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    //Card *mutable_card = Sanguosha->cloneCard(Self->tag["qice"].toString());
    Card *mutable_card = Sanguosha->cloneCard(getUserString());
    if (mutable_card) {
        mutable_card->addSubcards(subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    if (mutable_card->isKindOf("Collateral")) {
        if (targets.length()/2 > subcards.length()) return false;
    } else {
        if (targets.length() > subcards.length()) return false;
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *QiceCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    Room *room = source->getRoom();

    QString c = toString().split(":").last();   //getUserString() bug here. damn it!

    Card *use_card = Sanguosha->cloneCard(c);
    use_card->setSkillName("qice");
    use_card->addSubcards(subcards);
    use_card->setCanRecast(false);
    use_card->setShowSkill("qice");

    bool available = true;

    QList<ServerPlayer *> targets;
    if (use_card->isKindOf("AwaitExhausted")) {
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (!source->isProhibited(p, use_card) && source->isFriendWith(p))
                targets << p;
     } else if (use_card->getSubtype() == "global_effect" && !use_card->isKindOf("FightTogether")) {
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (!source->isProhibited(p, use_card))
                targets << p;
    } else if (use_card->isKindOf("FightTogether")) {
        QStringList big_kingdoms = source->getBigKingdoms("qice", MaxCardsType::Normal);
        QList<ServerPlayer *> bigs, smalls;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (source->isProhibited(p, use_card)) continue;
            QString kingdom = p->objectName();
            if (big_kingdoms.length() == 1 && big_kingdoms.first().startsWith("sgs")) { // for JadeSeal
                if (big_kingdoms.contains(kingdom))
                    bigs << p;
                else
                    smalls << p;
            } else {
                if (!p->hasShownOneGeneral()) {
                    smalls << p;
                    continue;
                }
                if (p->getRole() == "careerist")
                    kingdom = "careerist";
                else
                    kingdom = p->getKingdom();
                if (big_kingdoms.contains(kingdom))
                    bigs << p;
                else
                    smalls << p;
            }
        }
        if ((smalls.length() > 0 && smalls.length() < bigs.length() && bigs.length() > 0) || (smalls.length() > 0 && bigs.length() == 0))
            targets = smalls;
        else if ((smalls.length() > 0 && smalls.length() > bigs.length() && bigs.length() > 0) || (smalls.length() == 0 && bigs.length() > 0))
            targets = bigs;
        else if (smalls.length() == bigs.length())
            targets = smalls;
    } else if (use_card->getSubtype() == "aoe" && !use_card->isKindOf("BurningCamps")) {
        foreach (ServerPlayer *p, room->getOtherPlayers(source))
            if (!source->isProhibited(p, use_card))
                targets << p;
    } else if (use_card->isKindOf("BurningCamps")) {
        QList<const Player *> players = source->getNextAlive()->getFormation();
        foreach (const Player *p, players)
            if (!source->isProhibited(p, use_card))
                targets << room->findPlayer(p->objectName());
    }
    if (targets.length() > subcards.length()) return NULL;

    foreach(ServerPlayer *to, card_use.to)
        if (source->isProhibited(to, use_card)) {
            available = false;
            break;
        }
    available = available && use_card->isAvailable(source);
    use_card->deleteLater();
    if (!available) return NULL;
    return use_card;
}

#ifdef Q_OS_ANDROID
class QiceVS : public ViewAsSkill
{
public:
    QiceVS() : ViewAsSkill("qice")
    {
    }
    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getHandcardNum())
            return NULL;
        QString c = Self->tag["qice"].toString();
        if (c != "") {
            QiceCard *card = new QiceCard;
            card->addSubcards(cards);
            card->setUserString(c);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("QiceCard");
    }
};

class Qice : public TriggerSkill
{
public:
    Qice() : TriggerSkill("qice")
    {
        events << CardFinished;
        view_as_skill = new QiceVS;
        guhuo_type = "t";
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeTrick && use.card->getSkillName(true) == "qice" && TriggerSkill::triggerable(player)) {
            return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke("transform")) {
            room->broadcastSkillInvoke("transform", player->isMale());
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        player->showGeneral(false, true, true, false);
        room->transformDeputyGeneral(player);
        return false;
    }
};

#else
class QiceVS : public ViewAsSkill
{
public:
    QiceVS() : ViewAsSkill("qice")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *) const
    {
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 1 && cards.first()->isVirtualCard()) {
            QiceCard *card = new QiceCard;
            card->addSubcards(Self->getHandcards());
            card->setUserString(cards.first()->objectName());
            return card;
        }
        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("QiceCard");
    }

    virtual QList<const Card *> getGuhuoCards(const QList<const Card *> &) const
    {
        QStringList names = ViewAsSkill::getGuhuoCards("t");
        QList<const Card *> all_cards;
        foreach (QString name, names) {
            Card *card = Sanguosha->cloneCard(name);
            card->setCanRecast(false);
            card->addSubcards(Self->getHandcards());
            card->deleteLater();
            if (checkGuhuo(card)) all_cards << card;
        }
        return all_cards;
    }

    bool checkGuhuo(const Card *card) const
    {
        if (!card->isAvailable(Self)) return false;
        QList<const Player *> targets;
        if (card->isKindOf("AwaitExhausted")) {
            if (!Self->isProhibited(Self, card))
                targets << Self;
            foreach (const Player *p, Self->getAliveSiblings())
                if (!Self->isProhibited(p, card) && Self->isFriendWith(p))
                    targets << p;
         } else if (card->getSubtype() == "global_effect" && !card->isKindOf("FightTogether")) {
            if (!Self->isProhibited(Self, card))
                targets << Self;
            foreach (const Player *p, Self->getAliveSiblings())
                if (!Self->isProhibited(p, card))
                    targets << p;
        } else if (card->isKindOf("FightTogether")) {
            QStringList big_kingdoms = Self->getBigKingdoms("qice", MaxCardsType::Normal);
            QList<const Player *> bigs, smalls, all;
            all = Self->getAliveSiblings();
            all << Self;
            foreach (const Player *p, all) {
                if (Self->isProhibited(p, card)) continue;
                QString kingdom = p->objectName();
                if (big_kingdoms.length() == 1 && big_kingdoms.first().startsWith("sgs")) { // for JadeSeal
                    if (big_kingdoms.contains(kingdom))
                        bigs << p;
                    else
                        smalls << p;
                } else {
                    if (!p->hasShownOneGeneral()) {
                        smalls << p;
                        continue;
                    }
                    if (p->getRole() == "careerist")
                        kingdom = "careerist";
                    else
                        kingdom = p->getKingdom();
                    if (big_kingdoms.contains(kingdom))
                        bigs << p;
                    else
                        smalls << p;
                }
            }
            if ((smalls.length() > 0 && smalls.length() < bigs.length() && bigs.length() > 0) || (smalls.length() > 0 && bigs.length() == 0))
                targets = smalls;
            else if ((smalls.length() > 0 && smalls.length() > bigs.length() && bigs.length() > 0) || (smalls.length() == 0 && bigs.length() > 0))
                targets = bigs;
            else if (smalls.length() == bigs.length())
                targets = smalls;
        } else if (card->getSubtype() == "aoe" && !card->isKindOf("BurningCamps")) {
            foreach (const Player *p, Self->getAliveSiblings())
                if (!Self->isProhibited(p, card))
                    targets << p;
        } else if (card->isKindOf("BurningCamps")) {
            QList<const Player *> players = Self->getNextAlive()->getFormation();
            foreach (const Player *p, players)
                if (!Self->isProhibited(p, card))
                    targets << p;
        }
        return (targets.length() <= Self->getHandcardNum());
    }
};

class Qice : public TriggerSkill
{
public:
    Qice() : TriggerSkill("qice")
    {
        events << CardFinished;
        view_as_skill = new QiceVS;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeTrick && use.card->getSkillName(true) == "qice" && TriggerSkill::triggerable(player)) {
            return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke("transform")) {
            room->broadcastSkillInvoke("transform", player->isMale());
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        player->showGeneral(false, true, true, false);
        room->transformDeputyGeneral(player);
        return false;
    }
};
#endif

//bianhuanhou
//stupid design
WanweiCard::WanweiCard()
{
    target_fixed = true;
    will_throw = false;
}

void WanweiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QStringList cards = source->tag["wanwei"].toStringList();
    cards.append((IntList2StringList(this->getSubcards())).join("+"));
    source->tag["wanwei"] = cards;
}

class WanweiViewAsSkill : public ViewAsSkill
{
public:
    WanweiViewAsSkill() : ViewAsSkill("wanwei")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getMark(objectName()) && to_select->hasFlag("can_wanwei");
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@wanwei");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty() || cards.length() < Self->getMark(objectName())) return NULL;
        WanweiCard *card = new WanweiCard;
        card->addSubcards(cards);
        card->setShowSkill(objectName());
        card->setSkillName(objectName());
        return card;
    }
};

class Wanwei : public TriggerSkill
{
public:
    Wanwei() : TriggerSkill("wanwei")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        view_as_skill = new WanweiViewAsSkill;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && player->hasSkill(this)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                    && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
                foreach (int id, move.card_ids)
                    room->setCardFlag(id, "-" + objectName());
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                    && ((move.reason.m_reason == CardMoveReason::S_REASON_DISMANTLE && move.reason.m_playerId != move.reason.m_targetId)
                    || (move.to && move.to != player && move.to_place == Player::PlaceHand
                    && move.reason.m_reason != CardMoveReason::S_REASON_GIVE && move.reason.m_reason != CardMoveReason::S_REASON_SWAP))) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->hasFlag(objectName())) return TriggerStruct();
                }
                if (move.card_ids.length() >= player->getHandcardNum() + player->getEquips().length()) return TriggerStruct();
                return TriggerStruct(objectName(), player);
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int num = move.card_ids.length();
        bool get = false;
        CardMoveReason reason = move.reason;
        QString target_name = reason.m_playerId;
        if (move.to != NULL) {
            target_name = move.to->objectName();
            get = true;
        }
        ServerPlayer *target = room->findPlayer(target_name);
        foreach (const Card *c, player->getCards("he")) {
            if (get) {
                if (target->canGetCard(player, c->getEffectiveId()))
                    room->setCardFlag(c->getEffectiveId(), "can_wanwei");
            } else
                if (target->canDiscard(player, c->getEffectiveId()))
                    room->setCardFlag(c->getEffectiveId(), "can_wanwei");
        }
        room->setPlayerMark(player, objectName(), num);
        QStringList card_names;
        foreach (int id, move.card_ids)
            card_names << Sanguosha->getCard(id)->getFullName(true);
        QString card_name = card_names.join(",");

        const Card *card = room->askForUseCard(player, "@@wanwei", "@wanwei:" + card_name + ":" + QString::number(num) + ":" + (get ? target->objectName() : QString()),
                                               -1, Card::MethodNone, true, info.skill_position);
        foreach (const Card *c, player->getCards("he"))
            room->setCardFlag(c->getEffectiveId(), "-can_wanwei");
        if (card)
            return info;
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        QStringList lirang_card = player->tag[objectName()].toStringList();
        QList<int> result = StringList2IntList(lirang_card.last().split("+"));
        lirang_card.removeLast();
        player->tag[objectName()] = lirang_card;
        if (result.isEmpty()) return false;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        int num = move.card_ids.length();
        move.from_places.clear();
        foreach (int id, result) {
            room->setCardFlag(id, objectName());
            move.from_places << room->getCardPlace(id);
        }
        move.card_ids = result;
        data = QVariant::fromValue(move);

        bool get = false;
        CardMoveReason reason = move.reason;
        QString target_name = reason.m_playerId;
        if (move.to != NULL)
            get = true;
        ServerPlayer *target = room->findPlayer(target_name);

        LogMessage log;
        log.type = "#wanwei";
        log.from = player;
        log.to << target;
        log.arg = QString::number(num);
        log.arg2 = move.reason.m_playerId;
        log.arg2 = get ? "get" : "dismantled";
        room->sendLog(log);
        return false;
    }
};
/*
class Wanwei : public TriggerSkill
{
public:
    Wanwei() : TriggerSkill("wanwei")
    {
        events << TargetConfirming;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card || use.card->getTypeId() != Card::TypeTrick || (!use.card->isKindOf("Snatch") && !use.card->isKindOf("Dismantlement")))
            return QStringList();
        if (!use.to.contains(player)) return QStringList();
        return QStringList(objectName());
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const QString &position) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        player->tag[objectName()] = QVariant::fromValue(use.from);
        if (use.card->isKindOf("Dismantlement")) {
            bool invoke = room->askForDiscard(player, objectName(), 1, 1, true, true, "@wanwei1:" + use.from->objectName(), true);
            player->tag.remove(objectName());
            if (invoke) {
                room->broadcastSkillInvoke(objectName(), "male", -1, player, position);
                return true;
            }
        }
        else {
            QList<int> ints = room->askForExchange(player, position + "?" + objectName(), 1, 0, "@wanwei2:" + use.from->objectName(), "", ".");
            player->tag.remove(objectName());
            if (!ints.isEmpty()) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName());
                room->obtainCard(use.from, Sanguosha->getCard(ints.first()), reason, false);
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                room->broadcastSkillInvoke(objectName(), "male", -1, player, position);
                return true;
            }
        }
        player->tag.remove(objectName());
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const QString &) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->cancelTarget(use, player); // Room::cancelTarget(use, player);
        data = QVariant::fromValue(use);
        return false;
    }
};
*/

class Yuejian : public TriggerSkill
{
public:
    Yuejian() : TriggerSkill("yuejian")
    {
        events << CardUsed << EventPhaseStart;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed && !player->hasFlag(objectName()) && room->getCurrent() == player) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.from) {
                foreach (ServerPlayer *p, use.to) {
                    if (p != player) {
                        room->setPlayerFlag(player, objectName());
                        break;
                    }
                }
            }
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && !player->hasFlag(objectName()) && player->getPhase() == Player::Discard) {
            QList<ServerPlayer *> huanghous = room->findPlayersBySkillName(objectName());
            QList<TriggerStruct> skill_list;
            foreach (ServerPlayer *huanghou, huanghous)
                if (huanghou->willBeFriendWith(player))
                    skill_list << TriggerStruct(objectName(), huanghou);
            return skill_list;
        }
        return QList<TriggerStruct>();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *huanghou, const TriggerStruct &info) const
    {
        if (huanghou->askForSkillInvoke(this, QVariant::fromValue(player), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, huanghou, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        room->setPlayerFlag(player, "jianyue_keep");
        return false;
    }
};

class YuejianMaxCards : public MaxCardsSkill
{
public:
    YuejianMaxCards() : MaxCardsSkill("#yuejian-maxcard")
    {
    }

    virtual int getFixed(const ServerPlayer *target, MaxCardsType::MaxCardsCount) const
    {
        if (target->hasFlag("jianyue_keep"))
            return target->getMaxHp();
        return -1;
    }
};

//liguo
class Xichou : public TriggerSkill
{
public:
    Xichou() : TriggerSkill("xichou")
    {
        events << GeneralShown << DFDebut << CardUsed << EventPhaseChanging << CardResponded;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DFDebut && TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
            return TriggerStruct(objectName(), player);
        } else if (triggerEvent == GeneralShown && TriggerSkill::triggerable(player) && player->getMark(objectName()) == 0) {
            QString position = data.toBool() ? "head?" : "deputy?";
            bool show = data.toBool() ? player->inHeadSkills(this) : player->inDeputySkills(this);
            TriggerStruct trigger = TriggerStruct(objectName(), player);
            trigger.skill_position = position;
            return show ? trigger : TriggerStruct();
        } else if ((triggerEvent == CardUsed || triggerEvent == CardResponded) && player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded && data.value<CardResponseStruct>().m_isUse)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return TriggerStruct();

			if (player->getMark("xichou_color") == 0) return TriggerStruct();
            if (card->getTypeId() != Card::TypeSkill && ((card->isBlack() && player->getMark("xichou_color") != 1) ||
                    (card->isRed() && player->getMark("xichou_color") != 2) ||
                    (card->getSuit() == Card::NoSuit && player->getMark("xichou_color") != 3))) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->hasFlag("Global_Dying"))
                        return TriggerStruct();

                return TriggerStruct(objectName(), player);
            }
        }
        else if (triggerEvent == EventPhaseChanging && TriggerSkill::triggerable(player)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerMark(player, "xichou_color", 0);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (triggerEvent == DFDebut || triggerEvent == GeneralShown)
            room->setPlayerMark(player, objectName(), 1);
        if (player->hasShownSkill(this) || player->askForSkillInvoke(this, data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == CardUsed)
            room->loseHp(player);
        else {
            room->setPlayerProperty(player, "maxhp", QVariant(player->getMaxHp() + 2));

            RecoverStruct recover;
            recover.recover = 2;
            recover.who = player;
            room->recover(player, recover);
        }
        return false;
    }
};

class Xichou_record : public TriggerSkill
{
public:
    Xichou_record() : TriggerSkill("#xichou-record")
    {
        events << CardUsed << CardResponded;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->hasSkill("xichou") && player->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded && data.value<CardResponseStruct>().m_isUse)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return;

            if (card->getTypeId() != Card::TypeSkill) {
                if (card->isBlack())
                    room->setPlayerMark(player, "xichou_color", 1);
                else if (card->isRed())
                    room->setPlayerMark(player, "xichou_color", 2);
                else
                    room->setPlayerMark(player, "xichou_color", 3);
            }
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return TriggerStruct();
    }
};

//zuoci
class Huashen : public PhaseChangeSkill
{
public:
    Huashen() : PhaseChangeSkill("huashen")
    {
        frequency = Frequent;
    }

    static void playAudioEffect(ServerPlayer *zuoci, const QString &skill_name, const QString &position)
    {
        zuoci->getRoom()->broadcastSkillInvoke(skill_name, zuoci->isMale() ? "male" : "female", -1, zuoci, position);
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n, QString reason, const QString &position)
    {
        Room *room = zuoci->getRoom();
        QStringList huashens = zuoci->tag["Huashens"].toStringList();
        QStringList acquired = GetAvailableGenerals(zuoci, n, reason);

        if (reason == "huashen" && !huashens.isEmpty()) {
            QStringList drops;
            foreach (QString name, huashens) {
                drops << "-" + name;
                room->handleUsedGeneral("-" + name);
            }

            LogMessage log;
            log.type = "#dropHuashenDetail";
            log.from = zuoci;
            log.arg = huashens.join("\\, \\");
            room->sendLog(log);

            huashens.clear();

            QStringList old_skills;
            bool ishead = position == "head";
            foreach (QString name, zuoci->tag["HuashenSkill"].toStringList())
                old_skills << "-" + name + (ishead ? "" : "!");

            foreach (ServerPlayer *p, room->getAllPlayers())
                room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), drops.join(":"), QList<ServerPlayer *>() << p);

            zuoci->tag["HuashenSkill"] = QStringList();
            room->handleAcquireDetachSkills(zuoci, old_skills, true);
        }

        huashens << acquired;
        zuoci->tag["Huashens"] = huashens;

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zuoci->objectName(), acquired.join(":"), QList<ServerPlayer *>() << p);
        LogMessage log;
        log.type = "#GetHuashenDetail";
        log.from = zuoci;
        log.arg = huashens.join("\\, \\");
        room->sendLog(log);
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci, int n, QString reason)
    {
        Room *room = zuoci->getRoom();
        QStringList available, available2;
        foreach (QString name, Sanguosha->getLimitedGeneralNames())
            if (!name.startsWith("lord_") && !room->getUsedGeneral().contains(name))
                available << name;

        if (reason == "xinsheng") {
            foreach (QString name, available) {
                if (Sanguosha->getGeneral(name)->getKingdom() == Sanguosha->getGeneral(zuoci->tag["Huashens"].toStringList().first())->getKingdom())
                    available2 << name;
            }
        } else {
            available2 = available;
        }

        qShuffle(available2);
        if (available2.isEmpty()) return QStringList();
        n = qMin(n, available2.length());

        return available2.mid(0, n);
    }

    static void dohuashen(ServerPlayer *zuoci, QString reason, const QString &position)
    {
        if (!zuoci->isAlive()) return;
        Room *room = zuoci->getRoom();
        QStringList ac_dt_list;
        QStringList old_skills = zuoci->tag["HuashenSkill"].toStringList();

        QStringList huashens = zuoci->tag["Huashens"].toStringList();
        if (huashens.isEmpty()) return;

        QStringList result = room->askForGeneral(zuoci, huashens, QString(), false, reason, QVariant(), true).split("+");
        zuoci->tag["Huashens"] = result;

        foreach (QString name, huashens) {
            if (!result.contains(name))
                room->handleUsedGeneral("-" + name);
        }
        if (!room->getUsedGeneral().contains(result.first())) room->handleUsedGeneral(result.first());
        if (!room->getUsedGeneral().contains(result.last())) room->handleUsedGeneral(result.last());

        QStringList skill_names;
        bool ishead = position == "head";
        const General *general1 = Sanguosha->getGeneral(result.first());
        const General *general2 = Sanguosha->getGeneral(result.last());
        foreach (const Skill *skill, general1->getVisibleSkillList(true, ishead)) {
            if (skill->isLordSkill())
                continue;

                skill_names << skill->objectName();
        }
        foreach (const Skill *skill, general2->getVisibleSkillList(true, ishead)) {
            if (skill->isLordSkill())
                continue;

            skill_names << skill->objectName();
        }

        JsonArray arg;
        arg << QSanProtocol::S_GAME_EVENT_HUASHEN << zuoci->objectName() << result.join("+");
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        zuoci->tag["HuashenSkill"] = skill_names;
        foreach (QString name, skill_names) {
            if (!old_skills.contains(name))
                ac_dt_list.append(name + (ishead ? "" : "!"));
        }
        foreach (QString name, old_skills) {
            if (!skill_names.contains(name))
                ac_dt_list.append("-" + name + (ishead ? "" : "!"));
        }
        room->handleAcquireDetachSkills(zuoci, ac_dt_list, true);
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start;
    }
    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            playAudioEffect(player, objectName(), info.skill_position);
            return info;
        }
        return TriggerStruct();
    }
    virtual bool onPhaseChange(ServerPlayer *zuoci, const TriggerStruct &info) const
    {
        AcquireGenerals(zuoci, 5, objectName(), info.skill_position);
        dohuashen(zuoci, objectName(), info.skill_position);

        return false;
    }
};

class HuashenClear : public DetachEffectSkill
{
public:
    HuashenClear() : DetachEffectSkill("huashen")
    {
    }
    virtual void onSkillDetached(Room *room, ServerPlayer *player, QVariant &data) const
    {
        QStringList huashen_skill = player->tag["HuashenSkill"].toStringList();
        bool head = data.toString().split(":").last() == "head";
        foreach (QString skill_name, huashen_skill) {
            room->detachSkillFromPlayer(player, skill_name, false, true, head);
        }
        player->tag.remove("Huashens");
        room->removePlayerDisableShow(player, "zuoci");
    }
};

class HuashenDisable : public TriggerSkill
{
public:
    HuashenDisable() : TriggerSkill("#huashen-disable")
    {
        events << GeneralShown << EventAcquireSkill;
    }
    virtual TriggerStruct triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (event = GeneralShown) {
            bool head = data.toBool();
            if ((head && player->inHeadSkills(objectName())) || (!head && player->inDeputySkills(objectName()))) {
                player->hideGeneral(!head);
                room->setPlayerDisableShow(player, (head ? "d" : "h"), "zuoci");
            }
        } else if (data.toString().split(":").first() == "huashen") {
            bool head = data.toString().split(":").last() == "head";
            player->hideGeneral(!head);
            room->setPlayerDisableShow(player, (head ? "d" : "h"), "zuoci");
        }

        return TriggerStruct();
    }
};

class Xinsheng : public MasochismSkill
{
public:
    Xinsheng() : MasochismSkill("xinsheng")
    {
        frequency = Frequent;
    }

    virtual int getPriority() const
    {
        return -4;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &, ServerPlayer *) const
    {
        if (MasochismSkill::triggerable(zuoci))
            return TriggerStruct(objectName(), zuoci);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *zuoci, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (zuoci->askForSkillInvoke(this, data, info.skill_position)) {
            Huashen::playAudioEffect(zuoci, objectName(), info.skill_position);
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *zuoci, const DamageStruct &, const TriggerStruct &info) const
    {
        if (zuoci->tag["Huashens"].toStringList().isEmpty()) {
			Huashen::AcquireGenerals(zuoci, 5, "huashen", info.skill_position);
			Huashen::dohuashen(zuoci, "huashen", info.skill_position);
        } else {
			Huashen::AcquireGenerals(zuoci, 1, objectName(), info.skill_position);
			Huashen::dohuashen(zuoci, objectName(), info.skill_position);
        }
    }
};

//shamoke
class JiliTM : public TargetModSkill
{
public:
    JiliTM() : TargetModSkill("#jili-slash")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (!Sanguosha->matchExpPattern(pattern, from, card))
            return 0;

        if (from->hasSkill(objectName()) && from->getWeapon()) {
            const Weapon *weapon_card = qobject_cast<const Weapon *>(from->getWeapon()->getRealCard());
            Q_ASSERT(weapon_card);
            if (!card->getSubcards().contains(weapon_card->getEffectiveId()))
                return weapon_card->getRange();
        }
        return 0;
    }
};

class JiliRecord : public TriggerSkill
{
public:
    JiliRecord() : TriggerSkill("#jili-record")
    {
        events << CardUsed << CardResponded;
    }

    virtual int getPriority() const
    {
        return 7;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded && data.value<CardResponseStruct>().m_isUse)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return;

            if (card->getTypeId() != Card::TypeSkill)
                room->setPlayerMark(player, "jili", player->getMark("jili") + 1);
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
		return TriggerStruct();
    }

};

class Jili : public TriggerSkill
{
public:
    Jili() : TriggerSkill("jili")
    {
        events << CardUsed << EventPhaseChanging << CardResponded;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if ((triggerEvent == CardUsed || triggerEvent == CardResponded) && player->getPhase() == Player::Play && player->getWeapon() && TriggerSkill::triggerable(player)) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded && data.value<CardResponseStruct>().m_isUse)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return TriggerStruct();

            const Weapon *wcard = qobject_cast<const Weapon *>(player->getWeapon()->getRealCard());
            if (card->getTypeId() != Card::TypeSkill && player->getMark("jili") == wcard->getRange()) {
                return TriggerStruct(objectName(), player);
            }
        } else if (triggerEvent == EventPhaseChanging && TriggerSkill::triggerable(player)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerMark(player, "jili", 0);
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        if (player->getWeapon()) {
            const Weapon *card = qobject_cast<const Weapon *>(player->getWeapon()->getRealCard());
            player->drawCards(card->getRange(), "jili");
        }
        return false;
    }
};

//masu
SanyaoCard::SanyaoCard()
{
}

bool SanyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) return false;
    QList<const Player *> players = Self->getAliveSiblings();
    players << Self;
    int max = -1000;
    foreach (const Player *p, players) {
        if (max < p->getHp())
            max = p->getHp();
    }
    return to_select->getHp() == max;
}

void SanyaoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("sanyao", effect.from, effect.to));
}

class Sanyao : public OneCardViewAsSkill
{
public:
    Sanyao() : OneCardViewAsSkill("sanyao")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("SanyaoCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        SanyaoCard *first = new SanyaoCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        first->setShowSkill(objectName());
        return first;
    }
};

class Zhiman : public TriggerSkill
{
public:
    Zhiman() : TriggerSkill("zhiman")
    {
        events << DamageCaused;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to && player != damage.to)
                return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }
    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["zhiman_data"] = data;  // for AI
        bool invoke = player->askForSkillInvoke(this, QVariant::fromValue(damage.to), info.skill_position);
        player->tag.remove("zhiman_data");
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        player->tag[objectName()] = QVariant::fromValue(info.skill_position);
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *to = damage.to;
        LogMessage log;
        log.type = "#Zhiman";
        log.from = player;
        log.arg = objectName();
        log.to << to;
        room->sendLog(log);
        room->setPlayerMark(to, objectName(), 1);
        to->tag["zhiman_from"] = QVariant::fromValue(player);
        return true;
    }
};

class ZhimanSecond : public TriggerSkill
{
public:
    ZhimanSecond() : TriggerSkill("#zhiman-second")
    {
        events << DamageComplete;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to == player && player->getMark("zhiman") > 0) {
            ServerPlayer *masu = player->tag["zhiman_from"].value<ServerPlayer *>();
            if (damage.from == masu && masu->hasShownSkill("zhiman")) {
                QList<TriggerStruct> skill_list;
                TriggerStruct trigger = TriggerStruct(objectName(), masu);
                trigger.skill_position = masu->tag["zhiman"].toString();
                skill_list << trigger;
                return skill_list;
            }
        }
        return QList<TriggerStruct>();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *to, QVariant &, ServerPlayer *player, const TriggerStruct &) const
    {
        to->tag.remove("zhiman_from");
        room->setPlayerMark(to, "zhiman", 0);
        if (player->canGetCard(to, "ej")) {
            int card_id = room->askForCardChosen(player, to, "ej", "zhiman", false, Card::MethodGet);
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, Sanguosha->getCard(card_id), reason);
        }
        if (to->isFriendWith(player) &&
                to->askForSkillInvoke("transform")) {

            room->broadcastSkillInvoke("transform", to->isMale());
            to->showGeneral(true, true, true, false);
            to->showGeneral(false, true, true, false);
            if (to->canTransform())
                room->transformDeputyGeneral(to);
        }
        return false;
    }
};

//LengTong
class LieFeng : public TriggerSkill
{
public:
    LieFeng() : TriggerSkill("liefeng")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &data, ServerPlayer *) const
    {
		if (!TriggerSkill::triggerable(lengtong)) return TriggerStruct();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == lengtong && move.from_places.contains(Player::PlaceEquip)) {
            QList<ServerPlayer *> other_players = room->getOtherPlayers(lengtong);
            foreach (ServerPlayer *p, other_players) {
                if (lengtong->canDiscard(p, "he"))
                    return TriggerStruct(objectName(), lengtong);
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QList<ServerPlayer *> other_players = room->getOtherPlayers(lengtong);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, other_players) {
            if (lengtong->canDiscard(p, "he"))
                targets << p;
        }
        ServerPlayer *to = room->askForPlayerChosen(lengtong, targets, objectName(), "liefeng-invoke", true, true, info.skill_position);
        if (to) {
            lengtong->tag["liefeng_target"] = QVariant::fromValue(to);
            room->broadcastSkillInvoke(objectName(), "male", -1, lengtong, info.skill_position);
            return info;
        } else lengtong->tag.remove("liefeng_target");
        /*
        if (room->askForSkillInvoke(lengtong, objectName())) {
            room->broadcastSkillInvoke(objectName(), lengtong);
            return true;
        }
        */
        return TriggerStruct();

    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        ServerPlayer *to = lengtong->tag["liefeng_target"].value<ServerPlayer *>();
        lengtong->tag.remove("liefeng_target");
        if (to && lengtong->canDiscard(to, "he")) {
            int card_id = room->askForCardChosen(lengtong, to, "he", objectName(), false, Card::MethodDiscard);
            CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_DISMANTLE, lengtong->objectName(), to->objectName(), objectName(), NULL);
            room->throwCard(Sanguosha->getCard(card_id), reason, to, lengtong);
        }
        /*
        QList<int> ids = room->GlobalCardChosen(lengtong, room->getOtherPlayers(lengtong), "he", objectName(), "@liefeng", 1, 1,
            Room::OnebyOne, false, Card::MethodDiscard);
        ServerPlayer *to = room->getCardOwner(ids.first());
        CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_DISMANTLE, lengtong->objectName(), to->objectName(), objectName(), NULL);
        room->throwCard(Sanguosha->getCard(ids.first()), reason, to, lengtong);
        */
        return false;
    }
};

XuanlueequipCard::XuanlueequipCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool XuanlueequipCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty())
        return false;
    foreach (int card_id, subcards) {
        const Card *card = Sanguosha->getCard(card_id);
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        int equip_index = static_cast<int>(equip->location());
        if (to_select->getEquip(equip_index))
            return false;
        if ((equip_index == 0 && to_select->hasFlag("xuanlue_gotweapon")) || (equip_index == 1 && to_select->hasFlag("xuanlue_gotarmor")) ||
                (equip_index == 2 && to_select->hasFlag("xuanlue_gotoffensivehorse")) || (equip_index == 3 && to_select->hasFlag("xuanlue_gotdefensivehorse")) ||
                (equip_index == 4 && to_select->hasFlag("xuanlue_gottreasure")))
            return false;
    }
    return true;
}

void XuanlueequipCard::onUse(Room *, const CardUseStruct &card_use) const
{
    QStringList targets = card_use.from->tag["xuanlue_target"].toStringList();
    QStringList cards = card_use.from->tag["xuanlue_get"].toStringList();
    targets << card_use.to.first()->objectName();
    cards.append((IntList2StringList(this->getSubcards())).join("+"));
    card_use.from->tag["xuanlue_target"] = targets;
    card_use.from->tag["xuanlue_get"] = cards;
    foreach (int card_id, subcards) {
        const Card *card = Sanguosha->getCard(card_id);
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        int equip_index = static_cast<int>(equip->location());
        if (equip_index == 0)
            card_use.to.first()->setFlags("xuanlue_gotweapon");
        else if (equip_index == 1)
            card_use.to.first()->setFlags("xuanlue_gotarmor");
        else if (equip_index == 2)
            card_use.to.first()->setFlags("xuanlue_gotoffensivehorse");
        else if (equip_index == 3)
            card_use.to.first()->setFlags("xuanlue_gotdefensivehorse");
        else if (equip_index == 4)
            card_use.to.first()->setFlags("xuanlue_gottreasure");
    }
}

class XuanlueViewAsSkill : public ViewAsSkill
{
public:
    XuanlueViewAsSkill() : ViewAsSkill("xuanlue_equip")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &cards, const Card *to_select) const
    {
        foreach (const Card *card, cards) {
            if ((card->isKindOf("Weapon") && to_select->isKindOf("Weapon")) || (card->isKindOf("Armor") && to_select->isKindOf("Armor")) ||
                    (card->isKindOf("OffensiveHorse") && to_select->isKindOf("OffensiveHorse")) ||
                    (card->isKindOf("DefensiveHorse") && to_select->isKindOf("DefensiveHorse")) ||
                    (card->isKindOf("Treasure") && to_select->isKindOf("Treasure")))
                return false;
        }
        QList<int> card_ids = StringList2IntList(Self->property("xuanlue_cards").toString().split("+"));
        return card_ids.contains(to_select->getId()) && to_select->isKindOf("EquipCard");
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@xuanlue_equip");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        XuanlueequipCard *xuanlue_card = new XuanlueequipCard;
        xuanlue_card->addSubcards(cards);
        return xuanlue_card;
    }
};

XuanlueCard::XuanlueCard()
{
    target_fixed = true;
    mute = true;
}

void XuanlueCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->setPlayerMark(card_use.from, "@lue", 0);
    room->broadcastSkillInvoke("xuanlue", "male", -1, card_use.from, getSkillPosition());
    room->doSuperLightbox("lingtong", "xuanlue");
    SkillCard::onUse(room, card_use);
}

bool XuanlueCard::canPutEquipment(Room *room, QList<int> &cards) const
{
    foreach (int card_id, cards) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(card_id)->getRealCard());
        int equip_index = static_cast<int>(equip->location());
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (!p->getEquip(equip_index) && !(equip_index == 0 && p->hasFlag("xuanlue_gotweapon")) && !(equip_index == 1 && p->hasFlag("xuanlue_gotarmor")) &&
                        !(equip_index == 2 && p->hasFlag("xuanlue_gotoffensivehorse")) && !(equip_index == 3 && p->hasFlag("xuanlue_gotdefensivehorse")) &&
                        !(equip_index == 4 && p->hasFlag("xuanlue_gottreasure")))
                return true;
    }
    return false;
}

void XuanlueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> cards, cards_copy;
    QList<CardsMoveStruct> moves;
    cards = room->GlobalCardChosen(source, room->getOtherPlayers(source), "e", "xuanlue", "@xuanlue", 1, 3, Room::NoLimited, false, Card::MethodGet);
    foreach (int id, cards) {
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
        CardsMoveStruct move(id, source, Player::PlaceHand, reason);
        moves.append(move);
    }

    if (!cards.isEmpty()) {
        cards = room->moveCardsAtomic(moves, false);
        cards_copy = cards;
        foreach (int id, cards_copy)
            if (room->getCardPlace(id) != Player::PlaceHand || room->getCardOwner(id) != source || !Sanguosha->getCard(id)->isKindOf("EquipCard"))
                cards.removeOne(id);
    }

    QList<CardsMoveStruct> moves2;
    if (!cards.isEmpty()) {
        room->setPlayerProperty(source, "xuanlue_cards", QVariant(IntList2StringList(cards).join("+")));
        while (!cards.isEmpty() && canPutEquipment(room, cards)
               && room->askForUseCard(source, "@@xuanlue_equip!", "@xuanlue-distribute:::" + QString::number(cards.length()), -1, Card::MethodNone)) {

            QStringList targets = source->tag["xuanlue_target"].toStringList();
            QStringList cards_get = source->tag["xuanlue_get"].toStringList();
            QList<int> get = StringList2IntList(cards_get.last().split("+"));
            ServerPlayer *target;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->objectName() == targets.last())
                    target = p;
            }
            targets.removeLast();
            cards_get.removeLast();
            source->tag["xuanlue_target"] = targets;
            source->tag["xuanlue_get"] = cards_get;

            CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), target->objectName(), "xuanlue", QString());
            CardsMoveStruct move(get, source, target, Player::PlaceHand, Player::PlaceEquip, reason);
            moves2.append(move);

            foreach (int id, get)
                cards.removeOne(id);

            QList<ServerPlayer *> _player;
            _player.append(source);
            room->notifyMoveCards(true, moves2, true, _player);
            room->notifyMoveCards(false, moves2, true, _player);
            room->setPlayerProperty(source, "xuanlue_cards", QVariant(IntList2StringList(cards).join("+")));
        }
        if (!cards.isEmpty()) {
            cards_copy = cards;
            foreach (int card_id, cards_copy) {
                const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(card_id)->getRealCard());
                int equip_index = static_cast<int>(equip->location());
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!p->getEquip(equip_index) && !(equip_index == 0 && p->hasFlag("xuanlue_gotweapon")) && !(equip_index == 1 && p->hasFlag("xuanlue_gotarmor")) &&
                                !(equip_index == 2 && p->hasFlag("xuanlue_gotoffensivehorse")) && !(equip_index == 3 && p->hasFlag("xuanlue_gotdefensivehorse")) &&
                                !(equip_index == 4 && p->hasFlag("xuanlue_gottreasure")) && cards.contains(card_id)) {
                        CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), p->objectName(), "xuanlue", QString());
                        CardsMoveStruct move(card_id, source, p, Player::PlaceHand, Player::PlaceEquip, reason);
                        moves2.append(move);
                        cards.removeOne(card_id);
                        if (equip_index == 0)
                            p->setFlags("xuanlue_gotweapon");
                        else if (equip_index == 1)
                            p->setFlags("xuanlue_gotarmor");
                        else if (equip_index == 2)
                            p->setFlags("xuanlue_gotoffensivehorse");
                        else if (equip_index == 3)
                            p->setFlags("xuanlue_gotdefensivehorse");
                        else if (equip_index == 4)
                            p->setFlags("xuanlue_gottreasure");
                    }
                }
            }
        }
        room->moveCardsAtomic(moves2, true);
    }
}

class Xuanlue : public ZeroCardViewAsSkill
{
public:
    Xuanlue() : ZeroCardViewAsSkill("xuanlue")
    {
        frequency = Limited;
        limit_mark = "@lue";
    }

    virtual const Card *viewAs() const
    {
        XuanlueCard *card = new XuanlueCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        bool check = false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p != player && p->hasEquip()) {
                check = true;
                break;
            }
        }
        return check && player->getMark("@lue") >= 1;
    }
};

//lvfan
DiaoduequipCard::DiaoduequipCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool DiaoduequipCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.isEmpty() && to_select != Self && Self->isFriendWith(to_select)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        int equip_index = static_cast<int>(equip->location());
        return (!to_select->getEquip(equip_index));
    }
    return false;
}

void DiaoduequipCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from, *to = effect.to;
    Room *room = player->getRoom();

    room->moveCardTo(this, player, to, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, player->objectName(), "diaodu", QString()));

    LogMessage log;
    log.type = "$DiaoduEquip";
    log.from = to;
    log.card_str = QString::number(getEffectiveId());
    room->sendLog(log);
}

class Diaoduequip : public OneCardViewAsSkill
{
public:
    Diaoduequip() : OneCardViewAsSkill("diaodu_equip")
    {
        response_pattern = "@@diaodu_equip";
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (!to_select->isKindOf("EquipCard")) return false;
        if (!Self->hasEquip(to_select)) return !Self->isJilei(to_select);
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        foreach (const Card *c, Self->getHandcards())
            if (c->getEffectiveId() == originalcard->getEffectiveId())
                return originalcard;
        foreach (int id, Self->getHandPile())
            if (id == originalcard->getEffectiveId())
                return originalcard;
        if (Self->hasEquip(originalcard)) {
            DiaoduequipCard *first = new DiaoduequipCard;
            first->addSubcard(originalcard->getId());
            return first;
        }
        return NULL;
    }
};

DiaoduCard::DiaoduCard()
{
    target_fixed = true;
    mute = true;
}

void DiaoduCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->broadcastSkillInvoke("diaodu", "male", -1, card_use.from, getSkillPosition());

    CardUseStruct new_use = card_use;
    new_use.to << card_use.from;
    if (card_use.from->getRole() != "careerist")
        foreach (ServerPlayer *p, room->getOtherPlayers(card_use.from))
            if (card_use.from->isFriendWith(p))
                new_use.to << p;
    room->sortByActionOrder(new_use.to);

    Card::onUse(room, new_use);
}

void DiaoduCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->askForUseCard(effect.to, "@@diaodu_equip", "@Diaodu-distribute", -1, Card::MethodUse);
}

class Diaodu : public ZeroCardViewAsSkill
{
public:
    Diaodu() : ZeroCardViewAsSkill("diaodu")
    {
    }
    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DiaoduCard");
    }
    virtual const Card *viewAs() const
    {
        DiaoduCard *dd = new DiaoduCard;
        dd->setSkillName(objectName());
        dd->setShowSkill(objectName());
        return dd;
    }
};

class Diancai : public TriggerSkill
{
public:
    Diancai() : TriggerSkill("diancai")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play)
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                    p->setMark(objectName(), 0);
            return;
        }
        if (!(triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && player != room->getCurrent() && room->getCurrent()->getPhase() == Player::Play))
            return;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
            player->setMark(objectName(), player->getMark(objectName()) + move.card_ids.length());
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!(triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play)) return QList<TriggerStruct>();
        QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
        QList<TriggerStruct> skill_list;
        foreach (ServerPlayer *p, players) {
            if (TriggerSkill::triggerable(p))
                if (p->isWounded() && p->getMark(objectName()) >= p->getLostHp())
                    skill_list << TriggerStruct(objectName(), p);
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ask_who->setMark(objectName(), 0);
        if (ask_who->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, ask_who, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        if (ask_who->getHandcardNum() < ask_who->getMaxHp())
            ask_who->drawCards(ask_who->getMaxHp() - ask_who->getHandcardNum(), objectName());

        if (ask_who->askForSkillInvoke("transform")) {
            room->broadcastSkillInvoke("transform", ask_who->isMale());
            ask_who->showGeneral(false, true, true, false);
            if (ask_who->canTransform())
                room->transformDeputyGeneral(ask_who);
        }
        return false;
    }
};

//lord_sunquan
LianziCard::LianziCard()
{
    target_fixed = true;
    will_throw = true;
}

void LianziCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int num = 0;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasShownOneGeneral() && p->getKingdom() == "wu" && p->getRole() != "careerist")
            num = num + p->getEquips().length();
    }

    QList<int> card_ids = room->getNCards(num);
    if (num == 0) return;

    LogMessage log;
    log.type = "$ViewDrawPile";
    log.from = source;
    log.card_str = IntList2StringList(card_ids).join("+");
    room->doNotify(source, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());

    Card::CardType type = Sanguosha->getCard(this->getEffectiveId())->getTypeId();
    foreach (int id, card_ids)
        if (Sanguosha->getCard(id)->getTypeId() == type)
            room->setCardFlag(id, "lianzi");

    AskForMoveCardsStruct result = room->askForMoveCards(source, card_ids, QList<int>(), false, "lianzi", "c++", 0, num, true, true, QList<int>() << -1, getSkillPosition());

    foreach (int id, result.bottom) {
        card_ids.removeOne(id);
        room->setCardFlag(id, "-lianzi");
        room->moveCardTo(Sanguosha->getCard(id), source, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "lianzi", ""), false);
        room->getThread()->delay();
    }
    room->moveCards(CardsMoveStruct(result.bottom, source, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, source->objectName(), "lianzi", "")), true);

    if (!card_ids.isEmpty()) {
        foreach (int id, card_ids)
            room->setCardFlag(id, "-lianzi");

        room->returnToDrawPile(card_ids, false);
    }
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length()));
}

class Lianzi : public OneCardViewAsSkill
{
public:
    Lianzi() : OneCardViewAsSkill("lianzi")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && !player->hasUsed("LianziCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        LianziCard *first = new LianziCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        first->setShowSkill(objectName());
        return first;
    }

    virtual bool chooseFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->hasFlag("lianzi");
    }
};

class Jubao : public TriggerSkill
{
public:
    Jubao() : TriggerSkill("jubao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Finish && TriggerSkill::triggerable(player))
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getTreasure() && p->getTreasure()->isKindOf("Luminouspearl") && player->canGetCard(p, "he"))
                return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->hasShownSkill(this) || player->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName(), player);
            return info;
        }
        return TriggerStruct();
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        QList<CardsMoveStruct> moves;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getTreasure() && p->getTreasure()->isKindOf("Luminouspearl") && player->canGetCard(p, "he")) {
                int card_id = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodGet);
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                CardsMoveStruct move(card_id, player, Player::PlaceHand, reason);
                moves.append(move);
            }
        }
        if (!moves.isEmpty()) room->moveCardsAtomic(moves, true);
        player->drawCards(1);
        return false;
    }
};

class JubaoCardFixed : public FixCardSkill
{
public:
    JubaoCardFixed() : FixCardSkill("#jubao-treasure")
    {
    }
    virtual bool isCardFixed(const Player *from, const Player *to, const QString &flags, Card::HandlingMethod method) const
    {
        if (from != to && method == Card::MethodGet && to->hasShownSkill(this) && (flags.contains("t")))
            return true;

        return false;
    }
};

GongxinCard::GongxinCard()
{
    mute = true;
}

bool GongxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && to_select != Self && !to_select->isKongcheng();
}

void GongxinCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->broadcastSkillInvoke("gongxin", "male", 2, source, getSkillPosition());
    ServerPlayer *target = targets.first();
    QList<int> ids;
    foreach (const Card *card, target->getHandcards())
        if (card->getSuit() == Card::Heart)
            ids.append(card->getEffectiveId());

    LogMessage log;
    log.type = "#KnownBothView";
    log.from = source;
    log.to << target;
    log.arg = "handcards";
    foreach (ServerPlayer *p, room->getOtherPlayers(source, true))
        room->doNotify(p, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());

    int card_id = room->doGongxin(source, target, ids, "gongxin");
    if (card_id == -1) return;
    room->getThread()->delay();
    room->broadcastSkillInvoke("gongxin", "male", 1, source, getSkillPosition());
    source->tag["gongxin"] = QVariant::fromValue(target);
    QStringList operation;
    operation << "put";
    if (source->canDiscard(target, card_id)) operation << "discard";
    QString result;
    if (operation.length() > 1)
        result = room->askForChoice(source, "gongxin%log:" + Sanguosha->getCard(card_id)->getFullName(true), operation.join("+"));
    else
        result = operation.first();
    source->tag.remove("gongxin");
    if (result == "discard") {
        CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_DISMANTLE, source->objectName(), target->objectName(), "gongxin", NULL);
        room->throwCard(Sanguosha->getCard(card_id), reason, target, source);
    }
    else {
        source->setFlags("Global_GongxinOperator");
        CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_PUT, source->objectName(), target->objectName(), "gongxin", NULL);
        room->moveCardTo(Sanguosha->getCard(card_id), target, NULL, Player::DrawPile, reason, true);
        source->setFlags("-Global_GongxinOperator");
    }
}

class Gongxin : public ZeroCardViewAsSkill
{
public:
    Gongxin() : ZeroCardViewAsSkill("gongxin")
    {
    }
    virtual const Card *viewAs() const
    {
        GongxinCard *card = new GongxinCard;
        card->setShowSkill("showforviewhas");
        return card;
    }
    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GongxinCard") && player->canShowGeneral();
    }
};

FlameMapCard::FlameMapCard()
{
    mute = true;
    target_fixed = true;
}

void FlameMapCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    source->showSkill(this->showSkill());
    ServerPlayer *sunquan = room->getLord(source->getKingdom());
    room->setCardFlag((subcards.first()), "flame_map");
    sunquan->addToPile("flame_map", subcards, true, room->getAllPlayers(), CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, source->objectName()));
}

class FlameMapVS : public OneCardViewAsSkill
{
public:
    FlameMapVS() : OneCardViewAsSkill("flamemap")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard");
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        const Player *sunquan = player->getLord();
        if (!sunquan || !sunquan->hasLordSkill("jiahe") || !player->willBeFriendWith(sunquan))
            return false;
        return !player->hasUsed("FlameMapCard") && player->canShowGeneral();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FlameMapCard *slash = new FlameMapCard;
        slash->addSubcard(originalCard);
        slash->setShowSkill("showforviewhas");
        return slash;
    }
};

class FlameMap : public TriggerSkill
{
public:
    FlameMap() : TriggerSkill("flamemap")
    {
        events << Damaged;
        view_as_skill = new FlameMapVS;
        attached_lord_skill = true;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *sunquan = room->getLord(player->getKingdom());
		if (!sunquan || sunquan->isDead() || !TriggerSkill::triggerable(player)) return TriggerStruct();
        QList<int> ids = player->getPile("flame_map");
        if (!ids.isEmpty())
            return TriggerStruct(objectName(), player);

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        QList<int> maps = player->getPile("flame_map");
        if (!maps.isEmpty()) {
            int card_id;
            if (maps.length() == 1)
                card_id = maps.first();
            else {
                room->fillAG(maps, player);
                int aidelay = Config.AIDelay;
                Config.AIDelay = 0;
                card_id = room->askForAG(player, maps, false, objectName());
                Config.AIDelay = aidelay;
            }

            LogMessage log;
            log.type = "$FlameMapRemove";
            log.from = player;
            log.card_str = Sanguosha->getCard(card_id)->toString();
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            room->clearAG(player);
        }
        return false;
    }
};

class YingziEtra : public TriggerSkill
{
public:
    YingziEtra() : TriggerSkill("yingziextra")
    {
        events << DrawNCards;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player) && !player->hasSkill("yingzi_zhouyu") && !player->hasSkill("yingzi_sunce"))
            return TriggerStruct("yingziextra", player);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this)) {
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        data = QVariant::fromValue(data.toInt() + 1);
        return false;
    }
};

class Jiahe : public TriggerSkill
{
public:
    Jiahe() : TriggerSkill("jiahe$")
    {
        events << GeneralShown << Death << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player == NULL)
            return TriggerStruct();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
             if (player->isLord() && player->hasLordSkill(objectName())) {
                 if (move.from_pile_names.contains("flame_map") && player->getPile("flame_map").length() < 3) {
                     foreach(ServerPlayer *p, room->getAlivePlayers())
                         room->detachSkillFromPlayer(p, "gongxin");
                 } else if (move.to_pile_name == "flame_map" && player->getPile("flame_map").length() >= 3)
                     foreach(ServerPlayer *p, room->getAlivePlayers())
                         if (p->willBeFriendWith(player) && !p->getAcquiredSkills().contains("gongxin"))
                             room->attachSkillToPlayer(p, "gongxin");
             }
        } else if (triggerEvent == GeneralShown && player->hasShownGeneral1()) {
            if (player && player->isAlive() && player->hasLordSkill(objectName())) {
                foreach(ServerPlayer *p, room->getAlivePlayers())
                    if (p->willBeFriendWith(player))
                        room->attachSkillToPlayer(p, "flamemap");
            } else {
               ServerPlayer *lord = room->getLord(player->getKingdom());
                if (lord && lord->isAlive() && lord->hasLordSkill(objectName()))
                    room->attachSkillToPlayer(player, "flamemap");
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && player->hasLordSkill(objectName())) {
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    room->detachSkillFromPlayer(p, "flamemap");
                    room->detachSkillFromPlayer(p, "gongxin");
                }
            }
        } else if (triggerEvent == DFDebut) {
            ServerPlayer *lord = room->getLord(player->getKingdom());
            if (lord && lord->isAlive() && lord->hasLordSkill(objectName()) && !player->getAcquiredSkills().contains("flamemap")) {
                room->attachSkillToPlayer(player, "flamemap");
                if (lord->getPile("flame_map").length() >= 3) room->attachSkillToPlayer(player, "gongxin");
            }
        }
        return TriggerStruct();
    }
};

class JiaheClear : public DetachEffectSkill
{
public:
    JiaheClear() : DetachEffectSkill("jiahe", "flame_map")
    {
    }
    virtual void onSkillDetached(Room *room, ServerPlayer *, QVariant &) const
    {
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            room->detachSkillFromPlayer(p, "flamemap");
            room->detachSkillFromPlayer(p, "gongxin");
        }
    }
};

class FlameMapVH : public ViewHasSkill
{
public:
    FlameMapVH() : ViewHasSkill("flamemap-viewhas")
    {
        viewhas_skills << "yingzi" << "yingziextra" << "haoshi" << "gongxin" << "qianxun";
    }

    virtual bool ViewHas(const Player *player, const QString &skill_name) const
    {
        if (!player->canShowGeneral()) return false;
        const Player *sunquan = player->getLord();
        if (!sunquan) return false;

        if (skill_name == "yingzi" || skill_name == "yingziextra") {
            if (sunquan && sunquan->hasLordSkill("jiahe") && player->willBeFriendWith(sunquan) && sunquan->getPile("flame_map").length() >= 1) return true;
        }

        if (skill_name == "haoshi") {
            if (sunquan && sunquan->hasLordSkill("jiahe") && player->willBeFriendWith(sunquan) && sunquan->getPile("flame_map").length() >= 2) return true;
        }

        if (skill_name == "gongxin") {
            if (sunquan && sunquan->hasLordSkill("jiahe") && player->willBeFriendWith(sunquan) && sunquan->getPile("flame_map").length() >= 3) return true;
        }

        if (skill_name == "qianxun") {
            if (sunquan && sunquan->hasLordSkill("jiahe") && player->willBeFriendWith(sunquan) && sunquan->getPile("flame_map").length() >= 4) return true;
        }
        return false;
    }
};

TransformationPackage::TransformationPackage()
    : Package("transformation")
{
    General *Xunyou = new General(this, "xunyou", "wei", 3); // Wei
    Xunyou->addSkill(new Zhiyu);
    Xunyou->addSkill(new Qice);
    Xunyou->addCompanion("xunyu");

    General *Bianhuanghou = new General(this, "bianhuanghou", "wei", 3, false);
    Bianhuanghou->addSkill(new Wanwei);
    Bianhuanghou->addSkill(new Yuejian);
    Bianhuanghou->addSkill(new YuejianMaxCards);
    insertRelatedSkills("yuejian", "#yuejian-maxcard");
    Bianhuanghou->addCompanion("caocao");

    General *Liguo = new General(this, "liguo", "qun"); // Qun
    Liguo->addSkill(new Xichou);
    Liguo->addSkill(new Xichou_record);
    insertRelatedSkills("xichou", "#xichou-record");
    Liguo->addCompanion("jiaxu");

    General *Zuoci = new General(this, "zuoci", "qun", 3);
    Zuoci->addSkill(new Huashen);
    Zuoci->addSkill(new HuashenDisable);
    Zuoci->addSkill(new HuashenClear);
    insertRelatedSkills("huashen", "#huashen-clear");
    Zuoci->addSkill(new Xinsheng);
    insertRelatedSkills("huashen", 2, "#huashen-clear", "#huashen-disable");
    Zuoci->addCompanion("yuji");

    General *Shamoke = new General(this, "shamoke", "shu"); // Shu
    Shamoke->addSkill(new Jili);
    Shamoke->addSkill(new JiliRecord);
    Shamoke->addSkill(new JiliTM);
    insertRelatedSkills("jili", 2, "#jili-record", "#jili-slash");

    General *Masu = new General(this, "masu", "shu", 3);
    Masu->addSkill(new Sanyao);
    Masu->addSkill(new Zhiman);
    Masu->addSkill(new ZhimanSecond);
    insertRelatedSkills("zhiman", "#zhiman-second");

    General *Lingtong = new General(this, "lingtong", "wu"); // Wu
    Lingtong->addSkill(new LieFeng);
    Lingtong->addSkill(new Xuanlue);
    Lingtong->addCompanion("ganning");

    General *lvfan = new General(this, "lvfan", "wu", 3);
    lvfan->addSkill(new Diaodu);
    lvfan->addSkill(new Diancai);

    General *sunquan = new General(this, "lord_sunquan$", "wu", 4, true, true);
    sunquan->addSkill(new Lianzi);
    sunquan->addSkill(new Jubao);
    sunquan->addSkill(new JubaoCardFixed);
    sunquan->addSkill(new Jiahe);
    sunquan->addSkill(new JiaheClear);
    insertRelatedSkills("jiahe", "#jiahe-clear");
    sunquan->addRelateSkill("flamemap");
    sunquan->addRelateSkill("gongxin");
    insertRelatedSkills("jubao", "#jubao-treasure");

    addMetaObject<XuanlueCard>();
    addMetaObject<XuanlueequipCard>();
    addMetaObject<DiaoduequipCard>();
    addMetaObject<DiaoduCard>();
    addMetaObject<QiceCard>();
    addMetaObject<WanweiCard>();
    addMetaObject<SanyaoCard>();
    addMetaObject<LianziCard>();
    addMetaObject<GongxinCard>();
    addMetaObject<FlameMapCard>();

    skills << new XuanlueViewAsSkill;
    skills << new Diaoduequip;
    skills << new Gongxin;
    skills << new FlameMap << new FlameMapVH;
    skills << new YingziEtra;
}

ADD_PACKAGE(Transformation)

Luminouspearl::Luminouspearl(Suit suit, int number) : Treasure(suit, number)
{
    setObjectName("Luminouspearl");
}

class LuminouspearlSkill : public ViewAsSkill
{
public:
    LuminouspearlSkill() : ViewAsSkill("Luminouspearl")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && selected.length() < Self->getMaxHp() && to_select != Self->getTreasure();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);
        return zhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ZhihengCard")
                && ((!player->ownSkill("zhiheng") && !player->getAcquiredSkills().contains("zhiheng"))
                    || !((player->inHeadSkills("zhiheng") && player->hasShownGeneral1()) || (player->inDeputySkills("zhiheng") && player->hasShownGeneral2()))) ;
    }
};

class ZhihengVH : public ViewHasSkill
{
public:
    ZhihengVH() : ViewHasSkill("zhiheng-viewhas")
    {
        global = true;
        viewhas_skills << "zhiheng";
    }
    virtual bool ViewHas(const Player *player, const QString &) const
    {
        if (player->hasTreasure("Luminouspearl")) return true;
        return false;
    }
};

TransformationEquipPackage::TransformationEquipPackage() : Package("transformation_equip", CardPack)
{
    Luminouspearl *np = new Luminouspearl();
    np->setParent(this);

    skills << new LuminouspearlSkill << new ZhihengVH;
}

ADD_PACKAGE(TransformationEquip)
