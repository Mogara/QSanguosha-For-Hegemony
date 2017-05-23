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

#include "formation.h"
#include "standard-basics.h"
#include "standard-tricks.h"
#include "client.h"
#include "engine.h"
#include "structs.h"
#include "gamerule.h"
#include "settings.h"
#include "json.h"

class Tuntian : public TriggerSkill
{
public:
    Tuntian() : TriggerSkill("tuntian")
    {
        events << CardsMoveOneTime << FinishJudge;
        frequency = Frequent;
        priority.insert(CardsMoveOneTime, 3);
        priority.insert(FinishJudge, -1);
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip)))
                player->addMark("tuntian_postpone");
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct>  skill_list;
        if (triggerEvent == CardsMoveOneTime) {
            if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::NotActive || player->getMark("tuntian_postpone") <= 0)
                return skill_list;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
                if (room->getTag("judge").toInt() == 0)
                    skill_list << TriggerStruct(objectName(), player);
            }
        } else {
            QList<ServerPlayer *> dengais = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *dengai, dengais) {
                int postponed = dengai->getMark("tuntian_postpone");
                if (postponed > 0)
                    skill_list << TriggerStruct(objectName(), dengai);
            }
        }

        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *dengai, const TriggerStruct &info) const
    {
        dengai->removeMark("tuntian_postpone");
        if (dengai->askForSkillInvoke(objectName(), data, info.skill_position)) {
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *dengai, const TriggerStruct &) const
    {
        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = "tuntian";
        judge.who = dengai;
        room->judge(judge);
        return false;
    }
};

class TuntianGotoField : public TriggerSkill
{
public:
    TuntianGotoField() : TriggerSkill("#tuntian-gotofield")
    {
        events << FinishJudge;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->who != NULL && judge->who->isAlive() && judge->who->hasSkill("tuntian")) {
            if (judge->reason == "tuntian" && judge->isGood() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                player = judge->who;
                return TriggerStruct(objectName(), player);
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->who->askForSkillInvoke("_tuntian", "gotofield")) {
            room->broadcastSkillInvoke("tuntian", "male", -1, judge->who, info.skill_position);
            return info;
        }
		return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->who->addToPile("field", judge->card);

        return false;
    }
};

class TuntianClear : public DetachEffectSkill
{
public:
    TuntianClear() : DetachEffectSkill("tuntian", "field")
    {
        frequency = Compulsory;
    }
};

class TuntianDistance : public DistanceSkill
{
public:
    TuntianDistance() : DistanceSkill("#tuntian-dist")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasShownSkill("tuntian"))
            return -from->getPile("field").length();
        else
            return 0;
    }
};

class Jixi : public OneCardViewAsSkill
{
public:
    Jixi() : OneCardViewAsSkill("jixi")
    {
        relate_to_place = "head";
        filter_pattern = ".|.|.|field";
        expand_pile = "field";
        skill_type = Attack;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("field").isEmpty();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Snatch *shun = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        shun->addSubcard(originalCard);
        shun->setSkillName(objectName());
        shun->setShowSkill(objectName());
        return shun;
    }
};

ZiliangCard::ZiliangCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ZiliangCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->tag["ziliang"] = subcards.first();
}

class ZiliangVS : public OneCardViewAsSkill
{
public:
    ZiliangVS() : OneCardViewAsSkill("ziliang")
    {
        response_pattern = "@@ziliang";
        filter_pattern = ".|.|.|field";
        expand_pile = "field";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ZiliangCard *c = new ZiliangCard;
        c->addSubcard(originalCard);
        c->setSkillName(objectName());
        c->setShowSkill(objectName());
        return c;
    }
};

class Ziliang : public TriggerSkill
{
public:
    Ziliang() : TriggerSkill("ziliang")
    {
        events << Damaged;
        relate_to_place = "deputy";
        view_as_skill = new ZiliangVS;
        skill_type = Replenish;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL || player->isDead()) return skill_list;
        QList<ServerPlayer *> dengais = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *dengai, dengais) {
            if (!dengai->getPile("field").isEmpty() && dengai->isFriendWith(player))
                skill_list << TriggerStruct(objectName(), dengai);
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ServerPlayer *player = ask_who;
        player->tag.remove("ziliang");
        player->tag["ziliang_aidata"] = data;
        if (room->askForUseCard(player, "@@ziliang", "@ziliang-give", -1, Card::MethodNone, true, info.skill_position))
            return info;

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        ServerPlayer *dengai = ask_who;
        if (!dengai) return false;

        bool ok = false;
        int id = dengai->tag["ziliang"].toInt(&ok);

        if (!ok) return false;

        if (player == dengai) {
            LogMessage log;
            log.type = "$MoveCard";
            log.from = player;
            log.to << player;
            log.card_str = QString::number(id);
            room->sendLog(log);
        } else
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, dengai->objectName(), player->objectName());
        room->obtainCard(player, id);

        return false;
    }
};

HuyuanCard::HuyuanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool HuyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void HuyuanCard::onEffect(const CardEffectStruct &effect) const
{
    const Card *equip = Sanguosha->getCard(subcards[0]);

    effect.from->tag["huyuan_target"] = QVariant::fromValue(effect.to);

    effect.from->getRoom()->moveCardTo(equip, effect.from, effect.to, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_PUT, effect.from->objectName(), "huyuan", QString()));

    LogMessage log;
    log.type = "$ZhijianEquip";
    log.from = effect.to;
    log.card_str = QString::number(equip->getEffectiveId());
    effect.from->getRoom()->sendLog(log);
}

class HuyuanViewAsSkill : public OneCardViewAsSkill
{
public:
    HuyuanViewAsSkill() : OneCardViewAsSkill("huyuan")
    {
        response_pattern = "@@huyuan";
        filter_pattern = "EquipCard";
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        HuyuanCard *first = new HuyuanCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huyuan : public PhaseChangeSkill
{
public:
    Huyuan() : PhaseChangeSkill("huyuan")
    {
        view_as_skill = new HuyuanViewAsSkill;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &, ServerPlayer *) const
    {
		if (!PhaseChangeSkill::triggerable(target)) return TriggerStruct();
        if (target->getPhase() == Player::Finish && !target->isNude())
            return TriggerStruct(objectName(), target);
		return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        target->tag.remove("huyuan_equip");
        target->tag.remove("huyuan_target");
        bool invoke = room->askForUseCard(target, "@@huyuan", "@huyuan-equip", -1, Card::MethodNone, true, info.skill_position);
        if (invoke && target->tag.contains("huyuan_target"))
            return info;

        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *caohong, const TriggerStruct &info) const
    {
        Room *room = caohong->getRoom();

        ServerPlayer *target = caohong->tag["huyuan_target"].value<ServerPlayer *>();

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (target->distanceTo(p) == 1 && caohong->canDiscard(p, "he"))
                targets << p;
        }
        if (!targets.isEmpty()) {
            ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "huyuan", "@huyuan-discard:" + target->objectName(), true, false, info.skill_position);
            if (to_dismantle != NULL) {
                int card_id = room->askForCardChosen(caohong, to_dismantle, "he", "huyuan", false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
            }
        }
        return false;
    }
};

HeyiSummon::HeyiSummon()
    : ArraySummonCard("heyi")
{
    mute = true;
}

class Heyi : public BattleArraySkill
{
public:
    Heyi() : BattleArraySkill("heyi", HegemonyMode::Formation)
    {
        events << EventPhaseStart << GeneralShown;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player != NULL && player->isAlive() && player->getPhase() == Player::RoundStart) {
                ServerPlayer *caohong = room->findPlayerBySkillName("heyi");
                if (caohong && caohong->isAlive() && caohong->hasShownSkill("heyi") && player->inFormationRalation(caohong)) {
                    room->doBattleArrayAnimate(caohong);
                    room->broadcastSkillInvoke(objectName(), caohong);
                }
            }
        } else if (triggerEvent == GeneralShown) {
            if (TriggerSkill::triggerable(player) && player->hasShownSkill(objectName()) && data.toBool() == player->inHeadSkills(objectName())) {
                room->doBattleArrayAnimate(player);
                room->broadcastSkillInvoke(objectName(), "male", -1, player, data.toBool() ? "head" : "deputy");
            }
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        return QList<TriggerStruct>();
    }
};

class FeiyingVH : public ViewHasSkill
{
public:
    FeiyingVH() : ViewHasSkill("feiyingVH")
    {
        global = true;
        viewhas_skills << "feiying";
    }

    virtual bool ViewHas(const Player *player, const QString &) const
    {
        QList<const Player *> caohongs;
        QList<const Player *> sib = player->getAliveSiblings();
        if (sib.length() < 3) return false;

        foreach (const Player *p, sib)
            if (p->hasShownSkill("heyi"))
                caohongs << p;

        foreach (const Player *caohong, caohongs)
            if (caohong->getFormation().contains(player))
                return true;

        return false;
    }
};

class Feiying : public DistanceSkill
{
public:
    Feiying() : DistanceSkill("feiying")
    {
    }

    virtual int getCorrect(const Player *, const Player *to) const
    {
        if (to->hasShownSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

TiaoxinCard::TiaoxinCard()
{
}

bool TiaoxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->inMyAttackRange(Self) && to_select != Self;
}

void TiaoxinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    bool use_slash = false;
    if (effect.to->canSlash(effect.from, NULL, false))
        use_slash = room->askForUseSlashTo(effect.to, effect.from, "@tiaoxin-slash:" + effect.from->objectName());
    if (!use_slash && effect.from->canDiscard(effect.to, "he"))
        room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "tiaoxin", false, Card::MethodDiscard), effect.to, effect.from);
}

class Tiaoxin : public ZeroCardViewAsSkill
{
public:
    Tiaoxin() : ZeroCardViewAsSkill("tiaoxin")
    {
        skill_type = Attack;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("TiaoxinCard");
    }

    virtual const Card *viewAs() const
    {
        TiaoxinCard *card = new TiaoxinCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        if (player->hasArmorEffect("bazhen") || player->hasArmorEffect("EightDiagram"))
            return 3;

        return qrand() % 2 + 1;
    }
};

class Yizhi : public ViewHasSkill
{
public:
    Yizhi() : ViewHasSkill("yizhi")
    {
        relate_to_place = "deputy";
        frequency = Compulsory;
        viewhas_skills << "guanxing";
    }

    virtual bool ViewHas(const Player *player, const QString &skill_name) const
    {
        if (skill_name == "guanxing" && player->hasSkill(this) && !player->inHeadSkills(skill_name))
            return true;
        return false;
    }
};

TianfuSummon::TianfuSummon()
    : ArraySummonCard("tianfu")
{
}

class Tianfu : public BattleArraySkill
{
public:
    Tianfu() : BattleArraySkill("tianfu", HegemonyMode::Formation)
    {
        events << EventPhaseStart << Death << EventLoseSkill << EventAcquireSkill
            << GeneralShown << GeneralHidden << RemoveStateChanged;
        relate_to_place = "head";
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (player == NULL || current == NULL) return;

        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::RoundStart)
                return;
        } else if (triggerEvent == Death) {
            if (player != current && !player->isFriendWith(player))
                return;
        } else if (triggerEvent == GeneralShown) {
            if (player->hasShownAllGenerals() || !current->inFormationRalation(player))
                return;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.value<InfoStruct>().info != "tianfu" || !current->inFormationRalation(player))
                return;
        } else if (triggerEvent == GeneralHidden && player->hasShownOneGeneral()) {
            return;
        }

        foreach (ServerPlayer *p, room->getPlayers()) {
            if (p->getMark("tianfu_kanpo") > 0 && p->getAcquiredSkills("head").contains("kanpo") && !p->getGeneral()->hasSkill("kanpo")) {
                p->setMark("tianfu_kanpo", 0);
                room->detachSkillFromPlayer(p, "kanpo", true, true);
            }
        }

        if (player->aliveCount() < 4)
            return;

        if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
            QList<ServerPlayer *> jiangweis = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *jiangwei, jiangweis) {
                if (jiangwei->hasShownSkill(this) && jiangwei->inFormationRalation(current) && !jiangwei->getGeneral()->hasSkill("kanpo")) {
                    room->doBattleArrayAnimate(jiangwei);
                    jiangwei->setMark("tianfu_kanpo", 1);
                    room->attachSkillToPlayer(jiangwei, "kanpo");
                }
            }
        }

    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return TriggerStruct();
    }
};

class Shengxi : public TriggerSkill
{
public:
    Shengxi() : TriggerSkill("shengxi")
    {
        events << DamageDone << EventPhaseEnd << EventPhaseStart;
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        bool convert = Config.SkillModify.contains(objectName());
        if (triggerEvent == EventPhaseEnd && !convert) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Play && !player->hasFlag("ShengxiDamageInPlayPhase"))
                return TriggerStruct(objectName(), player);
        } else if (triggerEvent == EventPhaseStart && convert) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Discard && !player->hasFlag("ShengxiDamageInPlayPhase"))
                return TriggerStruct(objectName(), player);
        } else if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && !damage.from->hasFlag("ShengxiDamageInPlayPhase")) {
                if (!convert && damage.from->getPhase() == Player::Play)
                    damage.from->setFlags("ShengxiDamageInPlayPhase");
                else if (convert)
                    damage.from->setFlags("ShengxiDamageInPlayPhase");
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        player->drawCards(2);

        return false;
    }
};

class ShengxiClear : public TriggerSkill
{
public:
    ShengxiClear() : TriggerSkill("#shengxi-clear")
    {
        events << EventPhaseEnd << EventPhaseStart;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    virtual TriggerStruct triggerable(TriggerEvent event, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        bool convert = Config.SkillModify.contains("shengxi");
        if (!convert && event == EventPhaseEnd && player->getPhase() == Player::Play && player->hasFlag("ShengxiDamageInPlayPhase"))
            player->setFlags("-ShengxiDamageInPlayPhase");
        else if (convert && event == EventPhaseStart && player->getPhase() == Player::Discard && player->hasFlag("ShengxiDamageInPlayPhase"))
            player->setFlags("-ShengxiDamageInPlayPhase");
        return TriggerStruct();
    }
};

class Shoucheng : public TriggerSkill
{
public:
    Shoucheng() : TriggerSkill("shoucheng")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
		if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from->isAlive() && move.from->getPhase() == Player::NotActive
            && (move.from->isFriendWith(player) || player->willBeFriendWith(move.from))
            && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
            return TriggerStruct(objectName(), player);

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), data.value<CardsMoveOneTimeStruct>().from->objectName());
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from != NULL)
            from->drawCards(1);
        return false;
    }
};

ShangyiCard::ShangyiCard()
{
    mute = true;
}

bool ShangyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && (!to_select->isKongcheng() || !to_select->hasShownAllGenerals()) && to_select != Self;
}

void ShangyiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->showAllCards(card_use.from, card_use.to.first());
}

void ShangyiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    QStringList choices;
    if (!effect.to->isKongcheng())
        choices << "handcards";
    if (!effect.to->hasShownAllGenerals())
        choices << "hidden_general";

    room->setPlayerFlag(effect.to, "shangyiTarget");        //for AI
    QString choice = room->askForChoice(effect.from, "shangyi%to:" + effect.to->objectName(),
        choices.join("+"), QVariant::fromValue(effect.to));
    room->setPlayerFlag(effect.to, "-shangyiTarget");

    LogMessage log;
    log.type = "#KnownBothView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = choice;
    room->sendLog(log, QList<ServerPlayer *>() << effect.from);

    if (choice.contains("handcards")) {
        room->broadcastSkillInvoke("shangyi", "male", 1, effect.from, getSkillPosition());
        QList<int> blacks;
        foreach (int card_id, effect.to->handCards()) {
            if (Sanguosha->getCard(card_id)->isBlack())
                blacks << card_id;
        }
        int to_discard = room->doGongxin(effect.from, effect.to, blacks, "shangyi");
        if (to_discard == -1) return;

        effect.from->tag.remove("shangyi");
        CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), effect.to->objectName(), "shangyi", NULL);
        room->throwCard(Sanguosha->getCard(to_discard), reason, effect.to, effect.from);
    } else {
        room->broadcastSkillInvoke("shangyi", "male", 2, effect.from, getSkillPosition());
        QStringList list, list2;
        if (!effect.to->hasShownGeneral1()) {
            list << "head_general";
            list2 << effect.to->getActualGeneral1Name();
        }
        if (!effect.to->hasShownGeneral2()) {
            list << "deputy_general";
            list2 << effect.to->getActualGeneral2Name();
        }
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = Sanguosha->translate(name);
            log.arg2 = (name == "head_general" ? effect.to->getActualGeneral1Name() : effect.to->getActualGeneral2Name());
            room->doNotify(effect.from->getClient(), QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
        }
        JsonArray arg;
        arg << "shangyi";
        arg << JsonUtils::toJsonArray(list2);
        room->doNotify(effect.from->getClient(), QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
    }
}

class Shangyi : public ZeroCardViewAsSkill
{
public:
    Shangyi() : ZeroCardViewAsSkill("shangyi")
    {
        skill_type = Attack;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ShangyiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        ShangyiCard *c = new ShangyiCard;
        c->setSkillName(objectName());
        c->setShowSkill(objectName());
        return c;
    }
};

NiaoxiangSummon::NiaoxiangSummon()
    : ArraySummonCard("niaoxiang")
{
}

class Niaoxiang : public BattleArraySkill
{
public:
    Niaoxiang() : BattleArraySkill("niaoxiang", HegemonyMode::Siege)
    {
        events << TargetChosen;
        frequency = Compulsory;
        skill_type = Attack;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> skill_owners = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *skill_owner, skill_owners) {
            if (BattleArraySkill::triggerable(skill_owner) && skill_owner->hasShownSkill(this)
                && use.card != NULL && use.card->isKindOf("Slash")) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *to, use.to) {
                    if (player->inSiegeRelation(skill_owner, to))
                        targets << to;
                }
                if (!targets.isEmpty()) {
                    if (skill_owner->getHeadActivedSkills().contains(Sanguosha->getSkill(objectName())) && skill_owner->hasShownGeneral1()) {
                        TriggerStruct trigger = TriggerStruct(objectName(), skill_owner, targets);
                        trigger.skill_position = "head";
                        skill_list << trigger;
                    }
                    if (skill_owner->getDeputyActivedSkills().contains(Sanguosha->getSkill(objectName())) && skill_owner->hasShownGeneral2()) {
                        TriggerStruct trigger = TriggerStruct(objectName(), skill_owner, targets);
                        trigger.skill_position = "deputy";
                        skill_list << trigger;
                    }
                }
            }
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (ask_who != NULL && ask_who->hasShownSkill(this)) {
            room->doBattleArrayAnimate(ask_who, skill_target);
            room->broadcastSkillInvoke(objectName(), "male", -1, ask_who, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName(), true);
        CardUseStruct use = data.value<CardUseStruct>();
        int x = use.to.indexOf(skill_target);
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        if (jink_list.at(x).toInt() == 1)
            jink_list[x] = 2;
        use.from->tag["Jink_" + use.card->toString()] = jink_list;

        return false;
    }
};

class Yicheng : public TriggerSkill
{
public:
    Yicheng() : TriggerSkill("yicheng")
    {
        events << TargetConfirmed;
        frequency = Frequent;
        skill_type = Defense;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash")) return skill_list;
        if (use.to.contains(player)) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->willBeFriendWith(player))
                    skill_list << TriggerStruct(objectName(), p);
            }
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (ask_who->askForSkillInvoke(objectName(), QVariant::fromValue(player), info.skill_position)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ask_who->objectName(), player->objectName());
            room->broadcastSkillInvoke(objectName(), "male", -1, ask_who, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        player->drawCards(1);
        if (player->isAlive() && player->canDiscard(player, "he"))
            room->askForDiscard(player, objectName(), 1, 1, false, true, QString(), false, info.skill_position);
        return false;
    }
};

QianhuanCard::QianhuanCard()
{
    target_fixed = true;
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void QianhuanCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "qianhuan", QString());
    room->throwCard(Sanguosha->getCard(subcards.first()), reason, NULL);
}

class QianhuanVS : public OneCardViewAsSkill
{
public:
    QianhuanVS() : OneCardViewAsSkill("qianhuan")
    {
        filter_pattern = ".|.|.|sorcery";
        response_pattern = "@@qianhuan";
        expand_pile = "sorcery";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        QianhuanCard *c = new QianhuanCard;
        c->addSubcard(originalCard);
        c->setSkillName(objectName());
        return c;
    }
};

class Qianhuan : public TriggerSkill
{
public:
    Qianhuan() : TriggerSkill("qianhuan")
    {
        events << Damaged << TargetConfirming;
        view_as_skill = new QianhuanVS;
        skill_type = Wizzard;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL) return skill_list;
        QList<ServerPlayer *> yujis = room->findPlayersBySkillName(objectName());
        if (triggerEvent == Damaged && player->isAlive()) {
            foreach (ServerPlayer *yuji, yujis) {
                if (yuji->willBeFriendWith(player) && yuji->getPile("sorcery").length() < 4)
                    skill_list<< TriggerStruct(objectName(), yuji);
            }
        } else if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card || use.card->getTypeId() == Card::TypeEquip
                || use.card->getTypeId() == Card::TypeSkill || !use.to.contains(player))
                return skill_list;
            if (use.to.length() != 1) return skill_list;
            foreach (ServerPlayer *yuji, yujis) {
                if (yuji->getPile("sorcery").isEmpty()) continue;
                if (yuji->willBeFriendWith(use.to.first()))
                    skill_list<< TriggerStruct(objectName(), yuji);
            }
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ServerPlayer *yuji = ask_who;
        if (yuji == NULL)
            return TriggerStruct();
        yuji->tag["qianhuan_data"] = data;

        bool invoke = false;

        if (triggerEvent == Damaged) {
            bool convert = Config.SkillModify.contains(objectName());
            if (convert) {
                QStringList patterns = (QStringList() <<  "spade" << "heart" << "club" << "diamond");
                foreach (int id, yuji->getPile("sorcery"))
                    patterns.removeAll(Sanguosha->getCard(id)->getSuitString());
                QList<int> ints = room->askForExchange(yuji, objectName(), 1, 0, "@qianhuan", "", ".|" + patterns.join(","), info.skill_position);
                if (!ints.isEmpty()) {
                    yuji->addToPile("sorcery", ints);
                    invoke = true;
                }
            } else
                if (yuji->askForSkillInvoke(objectName(), "gethuan", info.skill_position))
                    invoke = true;
        } else {
            QString prompt;
            QStringList prompt_list;
            prompt_list << "@qianhuan-cancel";
            CardUseStruct use = data.value<CardUseStruct>();
            prompt_list << "";
            prompt_list << use.to.first()->objectName();
            prompt_list << use.card->objectName();
            prompt = prompt_list.join(":");
            if (room->askForUseCard(yuji, "@@qianhuan", prompt, -1, Card::MethodNone, true, info.skill_position)) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuji->objectName(), use.to.first()->objectName());
                invoke = true;
            }
        }

        yuji->tag.remove("qianhuan_data");
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", -1, yuji, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        ServerPlayer *yuji = ask_who;
        if (!yuji) return false;
        if (triggerEvent == Damaged) {
            if (Config.SkillModify.contains(objectName())) return false;

            int id = room->getNCards(1).first();
            Card::Suit suit = Sanguosha->getCard(id)->getSuit();
            bool duplicate = false;
            foreach (int card_id, yuji->getPile("sorcery")) {
                if (Sanguosha->getCard(card_id)->getSuit() == suit) {
                    duplicate = true;
                    break;
                }
            }
            yuji->addToPile("sorcery", id);
            if (duplicate) {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
            }
        } else if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->cancelTarget(use, use.to.first()); // Room::cancelTarget(use, player);
            room->setPlayerFlag(ask_who, objectName());
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class QianhuanClear : public DetachEffectSkill
{
public:
    QianhuanClear() : DetachEffectSkill("qianhuan", "sorcery")
    {
        frequency = Compulsory;
    }
};

class Zhendu : public TriggerSkill
{
public:
    Zhendu() : TriggerSkill("zhendu")
    {
        events << EventPhaseStart << CardEffectConfirmed;
        skill_type = Attack;
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == CardEffectConfirmed) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card && effect.card->getSkillName(false) == "_zhendu")
                effect.to->setFlags(objectName());
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL || triggerEvent == CardEffectConfirmed) return skill_list;
        if (player->getPhase() != Player::Play) return skill_list;
        QList<ServerPlayer *> hetaihous = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *hetaihou, hetaihous) {
            if (hetaihou->canDiscard(hetaihou, "h") && hetaihou != player)
                skill_list << TriggerStruct(objectName(), hetaihou);
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ServerPlayer *hetaihou = ask_who;
        if (hetaihou && room->askForDiscard(hetaihou, objectName(), 1, 1, true, false, "@zhendu-discard", true, info.skill_position)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hetaihou->objectName(), player->objectName());
            room->broadcastSkillInvoke(objectName(), "male", -1, hetaihou, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        ServerPlayer *hetaihou = ask_who;
        if (!hetaihou) return false;

        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("_zhendu");
        if (room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>(), true), true, true)) {
            if (player->isAlive() && player->hasFlag(objectName()))
                room->damage(DamageStruct(objectName(), hetaihou, player));
        } else
            analeptic->deleteLater();
        player->setFlags("-" + objectName());

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *c) const
    {
        if (c->isKindOf("Analeptic"))
            return 0;
        return -1;
    }
};

class Qiluan : public TriggerSkill
{
public:
    Qiluan() : TriggerSkill("qiluan")
    {
        events << Death << EventPhaseStart;
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (!death.damage || !death.damage->from)
                return;

            ServerPlayer *current = room->getCurrent();
            if (current && (current->isAlive() || death.who == current) && current->getPhase() != Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (TriggerSkill::triggerable(p) && death.damage->from == p)
                        room->setPlayerMark(p, objectName(), 1);
                }
            }
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL) return skill_list;
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark(objectName()) > 0 && TriggerSkill::triggerable(p) && p->isAlive())
                        skill_list << TriggerStruct(objectName(), p);
                }
            }
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *hetaihou, const TriggerStruct &info) const
    {
        room->setPlayerMark(hetaihou, objectName(), 0);
        if (hetaihou && hetaihou->askForSkillInvoke(objectName(), QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, hetaihou, info.skill_position);
            return info;
        }

		return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *hetaihou, const TriggerStruct &) const
    {
        if (hetaihou)
            hetaihou->drawCards(3);

        return false;
    }
};


class Zhangwu : public TriggerSkill
{
public:
    Zhangwu() : TriggerSkill("zhangwu")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player))
            return TriggerStruct();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DrawPileBottom)
            return TriggerStruct();
        int fldfid = -1;
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->isKindOf("DragonPhoenix")) {
                fldfid = id;
                break;
            }
        }

        if (fldfid == -1)
            return TriggerStruct();

        if (triggerEvent == CardsMoveOneTime) {
            if (move.to_place == Player::DiscardPile || (move.to_place == Player::PlaceEquip && move.to != player))
				return TriggerStruct(objectName(), player);
        } else if (triggerEvent == BeforeCardsMove) {
            if ((move.from == player && (move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceHand ||
                                        move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceEquip))
                                        && (move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip))) {
                if (move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceHand && move.to_place == Player::PlaceTable &&
                        move.reason.m_reason == CardMoveReason::S_REASON_USE && move.reason.m_cardString == Sanguosha->getCard(fldfid)->toString())
						return TriggerStruct();

                return TriggerStruct(objectName(), player);
            }
        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        bool invoke = player->hasShownSkill(this) ? true : player->askForSkillInvoke(this);
        if (invoke) {
            if (triggerEvent == CardsMoveOneTime) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.to != NULL)
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), move.to->objectName());
            }

            room->broadcastSkillInvoke(objectName(), (triggerEvent == BeforeCardsMove) ? 1 : 2, player);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *dragonPhoenix = NULL;
        int dragonPhoenixId = -1;
        foreach (int id, move.card_ids) {
            const Card *card = Sanguosha->getCard(id);
            if (card->isKindOf("DragonPhoenix")) {
                dragonPhoenixId = id;
                dragonPhoenix = card;
                break;
            }
        }

        if (triggerEvent == CardsMoveOneTime) {
            player->obtainCard(dragonPhoenix);
        } else {
            room->showCard(player, dragonPhoenixId);
            player->setFlags("fldf_removing");
            move.from_places.removeAt(move.card_ids.indexOf(dragonPhoenixId));
            move.card_ids.removeOne(dragonPhoenixId);
            data = QVariant::fromValue(move);

            room->moveCardTo(dragonPhoenix, NULL, Player::DrawPileBottom);
        }
        return false;
    }
};

class Zhangwu_Draw : public TriggerSkill
{
public:
    Zhangwu_Draw() : TriggerSkill("#zhangwu-draw")
    {
        frequency = Compulsory;
        events << CardsMoveOneTime;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!(player != NULL && player->isAlive() && player->hasSkill("zhangwu")))
            return TriggerStruct();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DrawPileBottom) {
            int fldfid = -1;
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->isKindOf("DragonPhoenix")) {
                    fldfid = id;
                    break;
                }
            }

            if (fldfid == -1)
				return TriggerStruct();

            if (player->hasFlag("fldf_removing")) {
                return TriggerStruct(objectName(), player);
            }

        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        player->setFlags("-fldf_removing");
        return player->hasShownSkill(this) ? info : TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        player->drawCards(2);
        return false;
    }
};

class Shouyue : public TriggerSkill
{
public:
    Shouyue() : TriggerSkill("shouyue$")
    {
        frequency = Compulsory;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return TriggerStruct();
    }
};

class Jizhao : public TriggerSkill
{
public:
    Jizhao() : TriggerSkill("jizhao")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@jizhao";
        skill_type = Recover;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player))
            return TriggerStruct();

        if (player->getMark("@jizhao") == 0 || player->getHp() > 0)
            return TriggerStruct();

        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player)
            return TriggerStruct();

        return TriggerStruct(objectName(), player);
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, data)) {
            player->broadcastSkillInvoke(objectName());
            room->doSuperLightbox("lord_liubei", objectName());
            room->setPlayerMark(player, limit_mark, 0);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        if (player->getHandcardNum() < player->getMaxHp())
            room->drawCards(player, player->getMaxHp() - player->getHandcardNum());

        if (player->getHp() < 2) {
            RecoverStruct rec;
            rec.recover = 2 - player->getHp();
            rec.who = player;
            room->recover(player, rec);
        }

        room->handleAcquireDetachSkills(player, "-shouyue|rende");
        return false; //return player->getHp() > 0 || player->isDead();
    }
};


FormationPackage::FormationPackage()
    : Package("formation")
{
    General *dengai = new General(this, "dengai", "wei"); // WEI 015
    dengai->addSkill(new Tuntian);
    dengai->addSkill(new TuntianGotoField);
    dengai->addSkill(new TuntianClear);
    dengai->addSkill(new TuntianDistance);
    dengai->addSkill(new Jixi);
    dengai->setHeadMaxHpAdjustedValue(-1);
    dengai->addSkill(new Ziliang);
    insertRelatedSkills("tuntian", 3, "#tuntian-dist", "#tuntian-gotofield", "#tuntian-clear");

    General *caohong = new General(this, "caohong", "wei"); // WEI 018
    caohong->addCompanion("caoren");
    caohong->addSkill(new Huyuan);
    caohong->addSkill(new Heyi);
    caohong->addRelateSkill("feiying");

    General *jiangwei = new General(this, "jiangwei", "shu"); // SHU 012 G
    jiangwei->addSkill(new Tiaoxin);
    jiangwei->addSkill(new Yizhi);
    jiangwei->setDeputyMaxHpAdjustedValue(-1);
    jiangwei->addSkill(new Tianfu);
    jiangwei->addRelateSkill("kanpo");

    General *jiangwanfeiyi = new General(this, "jiangwanfeiyi", "shu", 3); // SHU 018
    jiangwanfeiyi->addSkill(new Shengxi);
    jiangwanfeiyi->addSkill(new Shoucheng);
    jiangwanfeiyi->addSkill(new ShengxiClear);
    insertRelatedSkills("shengxi", "#shengxi-clear");

    General *jiangqin = new General(this, "jiangqin", "wu"); // WU 017
    jiangqin->addCompanion("zhoutai");
    jiangqin->addSkill(new Shangyi);
    jiangqin->addSkill(new Niaoxiang);

    General *xusheng = new General(this, "xusheng", "wu"); // WU 020
    xusheng->addCompanion("dingfeng");
    xusheng->addSkill(new Yicheng);

    General *yuji = new General(this, "yuji", "qun", 3); // QUN 011 G
    yuji->addSkill(new Qianhuan);
    yuji->addSkill(new QianhuanClear);
    insertRelatedSkills("qianhuan", "#qianhuan-clear");

    General *hetaihou = new General(this, "hetaihou", "qun", 3, false); // QUN 020
    hetaihou->addSkill(new Zhendu);
    hetaihou->addSkill(new Qiluan);

    General *liubei = new General(this, "lord_liubei$", "shu", 4, true, true);
    liubei->addSkill(new Zhangwu);
    liubei->addSkill(new Zhangwu_Draw);
    insertRelatedSkills("zhangwu", "#zhangwu-draw");
    liubei->addSkill(new Shouyue);
    liubei->addSkill(new Jizhao);

    addMetaObject<HuyuanCard>();
    addMetaObject<ZiliangCard>();
    addMetaObject<TiaoxinCard>();
    addMetaObject<ShangyiCard>();
    addMetaObject<HeyiSummon>();
    addMetaObject<TianfuSummon>();
    addMetaObject<NiaoxiangSummon>();
    addMetaObject<QianhuanCard>();

    skills << new Feiying << new FeiyingVH;
}

ADD_PACKAGE(Formation)


DragonPhoenix::DragonPhoenix(Suit suit, int number) : Weapon(suit, number, 2)
{
    setObjectName("DragonPhoenix");
}

class DragonPhoenixSkill : public WeaponSkill
{
public:
    DragonPhoenixSkill() : WeaponSkill("DragonPhoenix")
    {
        events << TargetChosen;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (WeaponSkill::triggerable(player) && use.card != NULL && use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *to, use.to) {
                if (to->canDiscard(to, "he"))
                    targets << to;
            }
            if (!targets.isEmpty())
                return TriggerStruct(objectName(), player, targets);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (ask_who->askForSkillInvoke(this, QVariant::fromValue(player))) {
            room->setEmotion(ask_who, "weapon/dragonphoenix");
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        room->askForDiscard(player, objectName(), 1, 1, false, true, "@dragonphoenix-discard");
        return false;
    }
};

class DragonPhoenixSkill2 : public WeaponSkill
{
public:
    DragonPhoenixSkill2() : WeaponSkill("#DragonPhoenix")
    {
        events << BuryVictim;
    }

    virtual int getPriority() const
    {
        return -4;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        if (!room->getMode().endsWith('p'))
            return false;

        ServerPlayer *dfowner = NULL;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->hasWeapon("DragonPhoenix")) {
                dfowner = p;
                break;
            }
        }
        if (dfowner == NULL || dfowner->getRole() == "careerist" || !dfowner->hasShownOneGeneral())
            return false;

        DeathStruct death = data.value<DeathStruct>();
        DamageStruct *damage = death.damage;
        if (!damage || !damage->from || damage->from != dfowner) return false;
        if (!damage->card || !damage->card->isKindOf("Slash")) return false;

        QStringList kingdom_list = Sanguosha->getKingdoms();
        kingdom_list << "careerist";
        bool broken = false;
        int n = dfowner->getPlayerNumWithSameKingdom("AI", QString(), MaxCardsType::Min); // could be canceled later
        foreach (const QString &kingdom, Sanguosha->getKingdoms()) {
            if (kingdom == "god") continue;
            if (dfowner->getRole() == "careerist") {
                if (kingdom == "careerist")
                    continue;
            } else if (dfowner->getKingdom() == kingdom)
                continue;
            int other_num = dfowner->getPlayerNumWithSameKingdom("AI", kingdom, MaxCardsType::Normal);
            if (other_num > 0 && other_num < n) {
                broken = true;
                break;
            }
        }

        if (broken) return false;

        QStringList generals = Sanguosha->getLimitedGeneralNames(true);
        qShuffle(generals);
        foreach (QString name, room->getUsedGeneral())
            if (generals.contains(name)) generals.removeAll(name);
        QStringList avaliable_generals;
        foreach (const QString &general, generals) {
            if (Sanguosha->getGeneral(general)->getKingdom() != dfowner->getKingdom())
                continue;
            avaliable_generals << general;
            if (avaliable_generals.length() >= 3) break;
        }

        if (avaliable_generals.isEmpty())
            return false;

        int aidelay = Config.AIDelay;
        Config.AIDelay = 0;
        bool invoke = room->askForSkillInvoke(dfowner, "DragonPhoenix", data) && room->askForSkillInvoke(player, "DragonPhoenix", "revive");
        Config.AIDelay = aidelay;
        if (invoke) {
            room->setEmotion(dfowner, "weapon/dragonphoenix");
            room->setPlayerProperty(player, "Duanchang", QVariant());
            QString to_change = room->askForGeneral(player, avaliable_generals, QString(), true, "dpRevive", dfowner->getKingdom(), true, true);

            if (!to_change.isEmpty()) {
                room->doDragonPhoenix(player, to_change, QString(), false, dfowner->getKingdom(), true, "h");
                room->setPlayerProperty(player, "hp", 2);

                player->setChained(false);
                room->broadcastProperty(player, "chained");

                player->setFaceUp(true);
                room->broadcastProperty(player, "faceup");

                LogMessage l;
                l.type = "#DragonPhoenixRevive";
                l.from = player;
                l.arg = to_change;
                room->sendLog(l);

                player->drawCards(1);
            }
        }
        return false;
    }
};


FormationEquipPackage::FormationEquipPackage() : Package("formation_equip", CardPack)
{
    DragonPhoenix *dp = new DragonPhoenix();
    dp->setParent(this);

    skills << new DragonPhoenixSkill << new DragonPhoenixSkill2;
    insertRelatedSkills("DragonPhoenix", "#DragonPhoenix");
}

ADD_PACKAGE(FormationEquip)


