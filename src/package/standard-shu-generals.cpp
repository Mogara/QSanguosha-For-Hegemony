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

#include "standard-shu-generals.h"
#include "structs.h"
#include "standard-basics.h"
#include "standard-tricks.h"
#include "client.h"
#include "engine.h"
#include "util.h"
#include "roomthread.h"

RendeCard::RendeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void RendeCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *target = card_use.to.first();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), target->objectName(), "rende", QString());
    room->obtainCard(target, this, reason, false);
}

void RendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int old_value = source->getMark("rende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "rende", new_value);

    if (old_value < 3 && new_value >= 3 && source->isWounded()) {
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        room->recover(source, recover);
    }
}

class RendeViewAsSkill : public ViewAsSkill
{
public:
    RendeViewAsSkill() : ViewAsSkill("rende")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        rende_card->setSkillName(objectName());
        rende_card->setShowSkill(objectName());
        return rende_card;
    }
};

class Rende : public TriggerSkill
{
public:
    Rende() : TriggerSkill("rende")
    {
        events << EventPhaseChanging;
        view_as_skill = new RendeViewAsSkill;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *) const
    {
        if (target->getMark("rende") > 0) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(target, "rende", 0);
        }
        return TriggerStruct();
    }
};

class Wusheng : public OneCardViewAsSkill
{
public:
    Wusheng() : OneCardViewAsSkill("wusheng")
    {
        response_or_use = true;
        skill_type = Attack;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *card) const
    {
        const Player *lord = Self->getLord();
        if (lord == NULL || !lord->hasLordSkill("shouyue") || !lord->hasShownGeneral1())
            if (!card->isRed())
                return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(card->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        slash->setShowSkill(objectName());
        return slash;
    }
};

class Paoxiao : public TargetModSkill
{
public:
    Paoxiao() : TargetModSkill("paoxiao")
    {
        skill_type = Attack;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (!Sanguosha->matchExpPattern(pattern, from, card))
            return 0;

        if (from->hasSkill(objectName()))
            return 1000;
        else
            return 0;
    }
};

class PaoxiaoArmorNullificaion : public TriggerSkill
{
public:
    PaoxiaoArmorNullificaion() : TriggerSkill("#paoxiao-null")
    {
        events << TargetChosen;
        frequency = Compulsory;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!player || !player->isAlive())
            return TriggerStruct();

        CardUseStruct use = data.value<CardUseStruct>();
        if (player->hasSkill("paoxiao")) {
            ServerPlayer *lord = room->getLord(player->getKingdom());
            if (lord != NULL && lord->hasLordSkill("shouyue") && lord->hasShownGeneral1()) {
                if (use.card != NULL && use.card->isKindOf("Slash")) {
                    if (!use.to.isEmpty())
                        return TriggerStruct(objectName(), player, use.to);
                }
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (ask_who != NULL) {
            if (ask_who->hasShownSkill("paoxiao"))
                return info;
            else {
                ask_who->tag["paoxiao_use"] = data; // useless data?
                bool invoke = ask_who->askForSkillInvoke("paoxiao", "armor_nullify:" + target->objectName());
                ask_who->tag.remove("paoxiao_use");
                if (invoke)
                    return info;
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        ServerPlayer *lord = room->getLord(ask_who->getKingdom());
        room->notifySkillInvoked(lord, "shouyue");
        CardUseStruct use = data.value<CardUseStruct>();
        target->addQinggangTag(use.card);
        return false;
    }
};

class Guanxing : public PhaseChangeSkill
{
public:
    Guanxing() : PhaseChangeSkill("guanxing")
    {
        frequency = Frequent;
        skill_type = Wizzard;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getPhase() == Player::Start;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->inDeputySkills("yizhi") && !player->inHeadSkills(this)) {
            if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
                room->broadcastSkillInvoke("yizhi", player);
                player->showGeneral(false);
                return info;
            } else {
                return TriggerStruct();
            }
        }
        if (!player->hasSkill("yizhi")) {
            if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
                room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
                return info;
            } else {
                return TriggerStruct();
            }
        }
        // if it runs here, it means player own both two skill;
        if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            bool show1 = player->hasShownSkill(this);
            bool show2 = player->hasShownSkill("yizhi");
            QStringList choices;
            if (!show1 && player->canShowGeneral("h"))
                choices << "show_head_general";
            if (!show2 && player->canShowGeneral("d"))
                choices << "show_deputy_general";
            if (choices.length() == 2 && player->canShowGeneral("hd"))
                choices << "show_both_generals";
            if (choices.length() != 3)
                choices << "cancel";
            QString choice = room->askForChoice(player, "GuanxingShowGeneral", choices.join("+"));
            if (choice == "cancel") {
                if (show1) {
                    room->broadcastSkillInvoke(objectName(), player);
                    return info;
                } else {
                    room->broadcastSkillInvoke("yizhi", "male", -1, player, "deputy");
                    TriggerStruct trigger = info;
                    trigger.skill_position = "deputy";
                    onPhaseChange(player, trigger);
					return TriggerStruct();
                }
            }
            if (choice != "show_head_general")
                player->showGeneral(false);
            if (choice == "show_deputy_general" && !show1) {
                room->broadcastSkillInvoke("yizhi", "male", -1, player, "deputy");
				player->showGeneral(false);
                TriggerStruct trigger = info;
                trigger.skill_position = "deputy";
                onPhaseChange(player, trigger);
                return TriggerStruct();
            } else {
                room->broadcastSkillInvoke(objectName(), player);
                return info;
            }
        }

        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge, const TriggerStruct &) const
    {
        Room *room = zhuge->getRoom();
        QList<int> guanxing = room->getNCards(getGuanxingNum(zhuge), false);

        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = zhuge;
        log.card_str = IntList2StringList(guanxing).join("+");
        room->doNotify(zhuge->getClient(), QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
        log.type = "$ViewDrawPile2";
        log.arg = QString::number(guanxing.length());
        room->sendLog(log, QList<ServerPlayer *>() << zhuge);

        room->askForGuanxing(zhuge, guanxing, Room::GuanxingBothSides);


        return false;
    }

    virtual int getGuanxingNum(ServerPlayer *zhuge) const
    {
        if (zhuge->inHeadSkills(this) && zhuge->hasShownGeneral1() && zhuge->hasShownSkill("yizhi")) return 5;
        return qMin(5, zhuge->aliveCount());
    }
};

class Kongcheng : public TriggerSkill
{
public:
    Kongcheng() : TriggerSkill("kongcheng")
    {
        events << TargetConfirming;
        frequency = Compulsory;
        skill_type = Defense;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player) && player->isKongcheng()) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isKindOf("Duel")) && use.to.contains(player))
                return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->hasShownSkill(this) || player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        CardUseStruct use = data.value<CardUseStruct>();

        room->cancelTarget(use, player); // Room::cancelTarget(use, player);

        data = QVariant::fromValue(use);
        return false;
    }
};

class LongdanVS : public OneCardViewAsSkill
{
public:
    LongdanVS() : OneCardViewAsSkill("longdan")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
            case CardUseStruct::CARD_USE_REASON_PLAY: {
                return card->isKindOf("Jink");
            }
            case CardUseStruct::CARD_USE_REASON_RESPONSE:
            case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                if (pattern == "slash")
                    return card->isKindOf("Jink");
                else if (pattern == "jink")
                    return card->isKindOf("Slash");
            }
            default:
                return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            jink->setShowSkill(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            slash->setShowSkill(objectName());
            return slash;
        } else
            return NULL;
    }
};

class Longdan : public TriggerSkill
{
public:
    Longdan() : TriggerSkill("longdan")
    {
        view_as_skill = new LongdanVS;
        events << CardUsed << CardResponded;
        skill_type = Alter;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player))
            return TriggerStruct();

        ServerPlayer *lord = room->getLord(player->getKingdom());
        if (lord != NULL && lord->hasLordSkill("shouyue") && lord->hasShownGeneral1()) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;

            if (card != NULL && card->getSkillName() == "longdan")
                return TriggerStruct(objectName(), player);
        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        return info;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        ServerPlayer *lord = room->getLord(player->getKingdom());
        room->notifySkillInvoked(lord, "shouyue");
        player->drawCards(1);
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash")) {
            return 1;
        } else {
            return 2;
        }
    }
};

Mashu::Mashu(const QString &owner) : DistanceSkill("mashu_" + owner)
{
}

int Mashu::getCorrect(const Player *from, const Player *, const Card *) const
{
    if (from->hasSkill(objectName()) && from->hasShownSkill(this))
        return -1;
    else
        return 0;
}

class Tieqi : public TriggerSkill
{
public:
    Tieqi() : TriggerSkill("tieqi")
    {
        events << TargetChosen;
        frequency = Frequent;
        skill_type = Attack;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<TriggerStruct> result;
        if (TriggerSkill::triggerable(player) && use.card != NULL && use.card->isKindOf("Slash")) {
            TriggerStruct test = TriggerStruct(objectName(), player, use.to);
            result << test;
        }
        return result;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *player, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, QVariant::fromValue(skill_target), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", 1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *skill_target, QVariant &data, ServerPlayer *machao, const TriggerStruct &info) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = machao->tag["Jink_" + use.card->toString()].toList();

        doTieqi(skill_target, machao, use, jink_list, info.skill_position);

        machao->tag["Jink_" + use.card->toString()] = jink_list;
        return false;
    }

private:
    static void doTieqi(ServerPlayer *target, ServerPlayer *source, CardUseStruct use, QVariantList &jink_list, const TriggerStruct &info)
    {
        Room *room = target->getRoom();

        int index = use.to.indexOf(target);

        JudgeStruct judge;

        ServerPlayer *lord = room->getLord(source->getKingdom());
        bool has_lord = false;
        if (lord != NULL && lord->hasLordSkill("shouyue") && lord->hasShownGeneral1()) {
            has_lord = true;
            judge.pattern = ".|spade";
            judge.good = false;
        } else {
            judge.pattern = ".|red";
            judge.good = true;
        }
        judge.reason = "tieqi";
        judge.who = source;

        if (has_lord)
            room->notifySkillInvoked(lord, "shouyue");

        target->setFlags("TieqiTarget"); //for AI
        room->judge(judge);
        target->setFlags("-TieqiTarget");

        if (judge.isGood()) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = target;
            room->sendLog(log);

            jink_list[index] = 0;
            room->broadcastSkillInvoke("tieqi","male", 2, source, info.skill_position);
        }
    }
};

class Jizhi : public TriggerSkill
{
public:
    Jizhi() : TriggerSkill("jizhi")
    {
        frequency = Frequent;
        events << CardUsed;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player))
            return TriggerStruct();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && use.card->isNDTrick()) {
            if (!use.card->isVirtualCard() || use.card->getSubcards().isEmpty())
                return TriggerStruct(objectName(), player);
            else if (use.card->getSubcards().length() == 1) {
                if (Sanguosha->getCard(use.card->getEffectiveId())->objectName() == use.card->objectName())
                    return TriggerStruct(objectName(), player);
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
        player->drawCards(1);
        return false;
    }
};

class Qicai : public TargetModSkill
{
public:
    Qicai() : TargetModSkill("qicai")
    {
        pattern = "TrickCard";
        skill_type = Wizzard;
    }

    virtual bool getDistanceLimit(const Player *from, const Player *, const Card *card) const
    {
        if (!Sanguosha->matchExpPattern(pattern, from, card))
            return false;

        if (from->hasSkill(objectName()))
            return true;
        else
            return false;
    }
};

class Liegong : public TriggerSkill
{
public:
    Liegong() : TriggerSkill("liegong")
    {
        events << TargetChosen;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play && use.card != NULL && use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *to, use.to) {
                int handcard_num = to->getHandcardNum();
                if (handcard_num >= player->getHp() || handcard_num <= player->getAttackRange())
                    targets << to;
            }
            if (!targets.isEmpty())
				return TriggerStruct(objectName(), player, targets);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *player, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, QVariant::fromValue(skill_target), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *huangzhong, const TriggerStruct &) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantList jink_list = huangzhong->tag["Jink_" + use.card->toString()].toList();

        doLiegong(target, use, jink_list);

        huangzhong->tag["Jink_" + use.card->toString()] = jink_list;
        return false;
    }

private:
    static void doLiegong(ServerPlayer *target, CardUseStruct use, QVariantList &jink_list)
    {
        int index = use.to.indexOf(target);
        LogMessage log;
        log.type = "#NoJink";
        log.from = target;
        target->getRoom()->sendLog(log);
        jink_list[index] = 0;
    }
};

class LiegongRange : public AttackRangeSkill
{
public:
    LiegongRange() : AttackRangeSkill("#liegong-for-lord")
    {
    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasShownSkill("liegong")) {
            const Player *lord = target->getLord();

            if (lord != NULL && lord->hasLordSkill("shouyue") && lord->hasShownGeneral1()) {
                return 1;
            }
        }
        return 0;
    }
};

class Kuanggu : public TriggerSkill
{
public:
    Kuanggu() : TriggerSkill("kuanggu")
    {
        frequency = Compulsory;
        events << Damage << PreDamageDone;
        skill_type = Recover;
    }

    virtual void record(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (player != NULL && event == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *weiyan = damage.from;
            if (weiyan != NULL) {
                if (weiyan->distanceTo(damage.to) != -1 && weiyan->distanceTo(damage.to) <= 1)
                    weiyan->tag["InvokeKuanggu"] = damage.damage;
                else
                    weiyan->tag.remove("InvokeKuanggu");
            }
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player) && event == Damage) {
            bool ok = false;
            int recorded_damage = player->tag["InvokeKuanggu"].toInt(&ok);
            if (ok && recorded_damage > 0 && player->isWounded()) {
                TriggerStruct skill_list = TriggerStruct(objectName(), player);
                DamageStruct damage = data.value<DamageStruct>();
                skill_list.times = damage.damage;
                return skill_list;
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        bool invoke = player->hasShownSkill(this) ? true : player->askForSkillInvoke(this, QVariant(), info.skill_position);
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());

        RecoverStruct recover;
        recover.who = player;
        room->recover(player, recover);

        return false;
    }
};


class Lianhuan : public OneCardViewAsSkill
{
public:
    Lianhuan() : OneCardViewAsSkill("lianhuan")
    {
        filter_pattern = ".|club|.|hand";
        response_or_use = true;
        skill_type = Alter;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        chain->setShowSkill(objectName());
        return chain;
    }
};

class Niepan : public TriggerSkill
{
public:
    Niepan() : TriggerSkill("niepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nirvana";
        skill_type = Recover;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(target) && target->getMark("@nirvana") > 0) {
            DyingStruct dying_data = data.value<DyingStruct>();

            if (target->getHp() > 0)
                return TriggerStruct();

            if (dying_data.who != target)
                return TriggerStruct();
            return TriggerStruct(objectName(), target);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (pangtong->askForSkillInvoke(this, data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, pangtong, info.skill_position);
            room->doSuperLightbox("pangtong", objectName());
            room->setPlayerMark(pangtong, "@nirvana", 0);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        pangtong->throwAllHandCardsAndEquips();
        QList<const Card *> tricks = pangtong->getJudgingArea();
        foreach (const Card *trick, tricks) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
            room->throwCard(trick, reason, NULL);
        }

        RecoverStruct recover;
        recover.recover = qMin(3, pangtong->getMaxHp()) - pangtong->getHp();
        room->recover(pangtong, recover);

        pangtong->drawCards(3);

        if (pangtong->isChained())
            room->setPlayerProperty(pangtong, "chained", false);

        if (!pangtong->faceUp())
            pangtong->turnOver();

        return false; //return pangtong->getHp() > 0 || pangtong->isDead();
    }
};

class Huoji : public OneCardViewAsSkill
{
public:
    Huoji() : OneCardViewAsSkill("huoji")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
        skill_type = Attack;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        fire_attack->setShowSkill(objectName());
        return fire_attack;
    }
};

class Bazhen : public TriggerSkill
{
public:
    Bazhen() : TriggerSkill("bazhen")
    {
        frequency = Compulsory;
        skill_type = Defense;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        room->setEmotion(player, "armor/eight_diagram");
        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = "EightDiagram";
        judge.who = player;

        room->judge(judge);

        if (judge.isGood()) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName("EightDiagram");
            room->provide(jink);

            return true;
        }

        return false;
    }
};

class BazhenVH : public ViewHasSkill
{
public:
    BazhenVH() : ViewHasSkill("#bazhenvh")
    {
        viewhas_armors << "EightDiagram";
    }

    virtual bool ViewHas(const Player *player, const QString &) const
    {
        if (player->isAlive() && player->hasSkill("bazhen") && !player->getArmor())
            return true;
        return false;
    }
};

class Kanpo : public OneCardViewAsSkill
{
public:
    Kanpo() : OneCardViewAsSkill("kanpo")
    {
        filter_pattern = ".|black|.|hand";
        response_or_use = true;
        response_pattern = "nullification";
        skill_type = Alter;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        ncard->setShowSkill(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const Player *player) const
    {
        QList <const Card *> handlist = player->getHandcards();
        foreach (int id, player->getHandPile()) {
            const Card *ca = Sanguosha->getCard(id);
            handlist.append(ca);
        }
        foreach (const Card *ca, handlist) {
            if (ca->isBlack())
                return true;
        }
        return false;
    }
};

SavageAssaultAvoid::SavageAssaultAvoid(const QString &avoid_skill) : TriggerSkill("#sa_avoid_" + avoid_skill), avoid_skill(avoid_skill)
{
    events << CardEffected;
    frequency = Compulsory;
}

TriggerStruct SavageAssaultAvoid::triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
{
    if (!player || !player->isAlive() || !player->hasSkill(avoid_skill)) return TriggerStruct();
    CardEffectStruct effect = data.value<CardEffectStruct>();
    if (effect.card->isKindOf("SavageAssault"))
        return TriggerStruct(objectName(), player);

    return TriggerStruct();
}

TriggerStruct SavageAssaultAvoid::cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
{
    if (player->hasShownSkill(avoid_skill)) return info;
    if (player->askForSkillInvoke(avoid_skill, QVariant(), info.skill_position)) {
        room->broadcastSkillInvoke(avoid_skill, "male", 1, player, info.skill_position);
        player->showSkill(avoid_skill, info.skill_position);
        return info;
    }
    return TriggerStruct();
}

bool SavageAssaultAvoid::effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
{
    room->notifySkillInvoked(player, avoid_skill);

    LogMessage log;
    log.type = "#SkillNullify";
    log.from = player;
    log.arg = avoid_skill;
    log.arg2 = "savage_assault";
    room->sendLog(log);

    return true;
}

class Huoshou : public TriggerSkill
{
public:
    Huoshou() : TriggerSkill("huoshou")
    {
        events << TargetChosen << ConfirmDamage << CardFinished;
        frequency = Compulsory;
        skill_type = Wizzard;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL) return skill_list;
        if (triggerEvent == TargetChosen) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SavageAssault")) {
                ServerPlayer *menghuo = room->findPlayerBySkillName(objectName());
                if (TriggerSkill::triggerable(menghuo) && use.from != menghuo) {
                    skill_list << TriggerStruct(objectName(), menghuo);
                    return skill_list;
                }
            }
        } else if (triggerEvent == ConfirmDamage && !room->getTag("HuoshouSource").isNull()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->isKindOf("SavageAssault"))
                return skill_list;

            ServerPlayer *menghuo = room->getTag("HuoshouSource").value<ServerPlayer *>();
            damage.from = menghuo->isAlive() ? menghuo : NULL;
            data = QVariant::fromValue(damage);
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("SavageAssault"))
                room->removeTag("HuoshouSource");
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        bool invoke = ask_who->hasShownSkill(this) ? true : ask_who->askForSkillInvoke(this, QVariant(), info.skill_position);
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", 2, ask_who, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        room->setTag("HuoshouSource", QVariant::fromValue(ask_who));

        return false;
    }
};

class Zaiqi : public PhaseChangeSkill
{
public:
    Zaiqi() : PhaseChangeSkill("zaiqi")
    {
        frequency = Frequent;
        skill_type = Recover;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *menghuo, QVariant &, ServerPlayer *) const
    {
        if (!PhaseChangeSkill::triggerable(menghuo))
            return TriggerStruct();

        if (menghuo->getPhase() == Player::Draw && menghuo->isWounded())
            return TriggerStruct(objectName(), menghuo);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", 1, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *menghuo, const TriggerStruct &info) const
    {
        Room *room = menghuo->getRoom();

        bool has_heart = false;
        int x = menghuo->getLostHp();
        QList<int> ids = room->getNCards(x);
        CardsMoveStruct move(ids, menghuo, Player::PlaceTable,
            CardMoveReason(CardMoveReason::S_REASON_TURNOVER, menghuo->objectName(), "zaiqi", QString()));
        room->moveCardsAtomic(move, true);

        room->getThread()->delay();
        room->getThread()->delay();

        QList<int> card_to_throw;
        QList<int> card_to_gotback;
        for (int i = 0; i < x; i++) {
            if (Sanguosha->getCard(ids[i])->getSuit() == Card::Heart)
                card_to_throw << ids[i];
            else
                card_to_gotback << ids[i];
        }
        if (!card_to_throw.isEmpty()) {
            DummyCard dummy(card_to_throw);

            RecoverStruct recover;
            recover.who = menghuo;
            recover.recover = card_to_throw.length();
            room->recover(menghuo, recover);

            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, menghuo->objectName(), "zaiqi", QString());
            room->throwCard(&dummy, reason, NULL);
            has_heart = true;
        }
        if (!card_to_gotback.isEmpty()) {
            DummyCard dummy2(card_to_gotback);
            CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, menghuo->objectName());
            room->obtainCard(menghuo, &dummy2, reason);
        }

        if (has_heart)
            room->broadcastSkillInvoke(objectName(), "male", 2, menghuo, info.skill_position);

        return true;
    }
};

class Juxiang : public TriggerSkill
{
public:
    Juxiang() : TriggerSkill("juxiang")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        skill_type = Wizzard;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player == NULL) return TriggerStruct();
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile && player != move.from
                && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                const Card *card = Card::Parse(move.reason.m_cardString);
                if (card && card->isKindOf("SavageAssault") && move.card_ids.length() == card->getSubcards().length()) {
                    bool check = true;
                    foreach (int id, card->getSubcards())
                       if (room->getCardPlace(id) != Player::DiscardPile || !move.card_ids.contains(id))
                           check = false;

                    if (check) return TriggerStruct(objectName(), player);
                }
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        bool invoke = player->hasShownSkill(this) ? true : player->askForSkillInvoke(this, QVariant(), info.skill_position);
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", 2, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        room->sendCompulsoryTriggerLog(player, objectName());

        DummyCard sa(move.card_ids);
        player->obtainCard(&sa);

        return false;
    }
};

class Lieren : public TriggerSkill
{
public:
    Lieren() : TriggerSkill("lieren")
    {
        events << Damage;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *zhurong, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(zhurong)) return TriggerStruct();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && !zhurong->isKongcheng()
            && !damage.to->isKongcheng() && damage.to != zhurong && !damage.chain && !damage.transfer && !damage.to->hasFlag("Global_DFDebut"))
            return TriggerStruct(objectName(), zhurong);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (zhurong->askForSkillInvoke(objectName(), data, info.skill_position)) {
            DamageStruct damage = data.value<DamageStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, zhurong->objectName(), damage.to->objectName());
            room->broadcastSkillInvoke(objectName(), "male", 1, zhurong, info.skill_position);
            PindianStruct *pd = zhurong->pindianSelect(damage.to, "lieren");
            zhurong->tag["lieren_pd"] = QVariant::fromValue(pd);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhurong, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        PindianStruct *pd = zhurong->tag["lieren_pd"].value<PindianStruct *>();
        zhurong->tag.remove("lieren_pd");
        if (pd != NULL) {
            ServerPlayer *target = pd->to;

            bool success = zhurong->pindian(pd);
            pd = NULL;
            if (!success) return false;

            room->broadcastSkillInvoke(objectName(), "male", 2, zhurong, info.skill_position);
            if (zhurong->canGetCard(target, "he")) {
                int card_id = room->askForCardChosen(zhurong, target, "he", objectName(), false, Card::MethodGet);
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhurong->objectName());
                room->obtainCard(zhurong, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
            }
        } else
            Q_ASSERT(false);

        return false;
    }
};

class Xiangle : public TriggerSkill
{
public:
    Xiangle() : TriggerSkill("xiangle")
    {
        events << TargetConfirming;
        frequency = Compulsory;
        skill_type = Defense;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && TriggerSkill::triggerable(player) && use.to.contains(player))
            return TriggerStruct(objectName(), player);

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->hasShownSkill(this) || player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->setEmotion(player, "xiangle");
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(liushan, objectName());
        CardUseStruct use = data.value<CardUseStruct>();

        QVariant dataforai = QVariant::fromValue(liushan);
        if (!room->askForCard(use.from, ".Basic", "@xiangle-discard:" + liushan->objectName(), dataforai)) {
            use.nullified_list << liushan->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

FangquanCard::FangquanCard()
{
}

bool FangquanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void FangquanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ServerPlayer *player = effect.to;

    LogMessage log;
    log.type = "#Fangquan";
    log.to << player;
    room->sendLog(log);

    player->gainAnExtraTurn();
}

class FangquanViewAsSkill : public OneCardViewAsSkill
{
public:
    FangquanViewAsSkill() : OneCardViewAsSkill("fangquan")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@fangquan";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FangquanCard *fangquan = new FangquanCard;
        fangquan->addSubcard(originalCard);
        fangquan->setSkillName(objectName());
        //fangquan->setShowSkill(objectName());
        return fangquan;
    }
};

class Fangquan : public TriggerSkill
{
public:
    Fangquan() : TriggerSkill("fangquan")
    {
        events << EventPhaseChanging;
        view_as_skill = new FangquanViewAsSkill;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player))
            return TriggerStruct();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play) {
            return (player->isSkipped(Player::Play)) ? TriggerStruct() : TriggerStruct(objectName(), player);
        } else if (change.to == Player::NotActive) {
            TriggerStruct trigger = TriggerStruct(objectName(), player);
            trigger.skill_position = player->tag[objectName()].toString();
            return (player->hasFlag(objectName()) && player->canDiscard(player, "h")) ? trigger : TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play) {
            if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
                player->skip(Player::Play);
                room->broadcastSkillInvoke(objectName(), "male", 1, player, info.skill_position);
                return info;
            }
        } else if (change.to == Player::NotActive)
            room->askForUseCard(player, "@@fangquan", "@fangquan-discard", -1, Card::MethodDiscard, true, info.skill_position);
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *liushan, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play) {
            liushan->tag[objectName()] = info.skill_position;
            liushan->setFlags(objectName());
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class Shushen : public TriggerSkill
{
public:
    Shushen() : TriggerSkill("shushen")
    {
        events << HpRecover;
        skill_type = Replenish;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player)) {
            QList<ServerPlayer *> friends;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->willBeFriendWith(p))
                    friends << p;
            }

            if (friends.isEmpty()) return TriggerStruct();

            TriggerStruct trigger = TriggerStruct(objectName(), player);
            RecoverStruct recover = data.value<RecoverStruct>();
            trigger.times = recover.recover;
            return trigger;
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QList<ServerPlayer *> friends;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->willBeFriendWith(p))
                friends << p;
        }

        if (friends.isEmpty()) return TriggerStruct();
        ServerPlayer *target = room->askForPlayerChosen(player, friends, objectName(), "shushen-invoke", true, true, info.skill_position);
        if (target != NULL) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);

            QStringList target_list = player->tag["shushen_target"].toStringList();
            target_list.append(target->objectName());
            player->tag["shushen_target"] = target_list;

            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        QStringList target_list = player->tag["shushen_target"].toStringList();
        QString target_name = target_list.last();
        target_list.removeLast();
        player->tag["shushen_target"] = target_list;

        ServerPlayer *to = NULL;

        foreach (ServerPlayer *p, player->getRoom()->getPlayers()) {
            if (p->objectName() == target_name) {
                to = p;
                break;
            }
        }
        if (to != NULL)
            to->drawCards(1);
        return false;
    }
};

class Shenzhi : public PhaseChangeSkill
{
public:
    Shenzhi() : PhaseChangeSkill("shenzhi")
    {
        skill_type = Recover;
        frequency = Frequent;
        //This skill can't be frequent in game actually.
        //because the frequency = Frequent has no effect in UI currently, we use this to reduce the AI delay
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!PhaseChangeSkill::triggerable(player))
            return TriggerStruct();
        if (player->getPhase() != Player::Start || player->isKongcheng())
            return TriggerStruct();
        return TriggerStruct(objectName(), player);
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male",- 1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren, const TriggerStruct &) const
    {
        int handcard_num = 0;
        foreach (const Card *card, ganfuren->getHandcards()) {
            if (!ganfuren->isJilei(card))
                handcard_num++;
        }
        ganfuren->throwAllHandCards();
        if (handcard_num >= ganfuren->getHp()) {
            RecoverStruct recover;
            recover.who = ganfuren;
            ganfuren->getRoom()->recover(ganfuren, recover);
        }
        return false;
    }
};

void StandardPackage::addShuGenerals()
{
    General *liubei = new General(this, "liubei", "shu"); // SHU 001
    liubei->addCompanion("guanyu");
    liubei->addCompanion("zhangfei");
    liubei->addCompanion("ganfuren");
    liubei->addSkill(new Rende);

    General *guanyu = new General(this, "guanyu", "shu", 5); // SHU 002
    guanyu->addSkill(new Wusheng);

    General *zhangfei = new General(this, "zhangfei", "shu"); // SHU 003
    zhangfei->addSkill(new Paoxiao);
    zhangfei->addSkill(new PaoxiaoArmorNullificaion);
    insertRelatedSkills("paoxiao", "#paoxiao-null");

    General *zhugeliang = new General(this, "zhugeliang", "shu", 3); // SHU 004
    zhugeliang->addCompanion("huangyueying");
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);

    General *zhaoyun = new General(this, "zhaoyun", "shu"); // SHU 005
    zhaoyun->addCompanion("liushan");
    zhaoyun->addSkill(new Longdan);

    General *machao = new General(this, "machao", "shu"); // SHU 006
    machao->addSkill(new Tieqi);
    machao->addSkill(new Mashu("machao"));

    General *huangyueying = new General(this, "huangyueying", "shu", 3, false); // SHU 007
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Qicai);

    General *huangzhong = new General(this, "huangzhong", "shu"); // SHU 008
    huangzhong->addCompanion("weiyan");
    huangzhong->addSkill(new Liegong);
    huangzhong->addSkill(new LiegongRange);
    insertRelatedSkills("liegong", "#liegong-for-lord");

    General *weiyan = new General(this, "weiyan", "shu"); // SHU 009
    weiyan->addSkill(new Kuanggu);

    General *pangtong = new General(this, "pangtong", "shu", 3); // SHU 010
    pangtong->addSkill(new Lianhuan);
    pangtong->addSkill(new Niepan);

    General *wolong = new General(this, "wolong", "shu", 3); // SHU 011
    wolong->addCompanion("huangyueying");
    wolong->addCompanion("pangtong");
    wolong->addSkill(new Huoji);
    wolong->addSkill(new Kanpo);
    wolong->addSkill(new Bazhen);
    wolong->addSkill(new BazhenVH);
    insertRelatedSkills("bazhen", "#bazhenvh");

    General *liushan = new General(this, "liushan", "shu", 3); // SHU 013
    liushan->addSkill(new Xiangle);
    liushan->addSkill(new Fangquan);

    General *menghuo = new General(this, "menghuo", "shu"); // SHU 014
    menghuo->addCompanion("zhurong");
    menghuo->addSkill(new SavageAssaultAvoid("huoshou"));
    menghuo->addSkill(new Huoshou);
    menghuo->addSkill(new Zaiqi);
    insertRelatedSkills("huoshou", "#sa_avoid_huoshou");

    General *zhurong = new General(this, "zhurong", "shu", 4, false); // SHU 015
    zhurong->addSkill(new SavageAssaultAvoid("juxiang"));
    zhurong->addSkill(new Juxiang);
    zhurong->addSkill(new Lieren);
    insertRelatedSkills("juxiang", "#sa_avoid_juxiang");

    General *ganfuren = new General(this, "ganfuren", "shu", 3, false); // SHU 016
    ganfuren->addSkill(new Shushen);
    ganfuren->addSkill(new Shenzhi);

    addMetaObject<RendeCard>();
    addMetaObject<FangquanCard>();
}
