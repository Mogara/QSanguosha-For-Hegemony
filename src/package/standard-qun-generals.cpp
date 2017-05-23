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

#include "standard-qun-generals.h"
#include "skill.h"
#include "standard-basics.h"
#include "standard-tricks.h"
#include "standard-shu-generals.h"
#include "engine.h"
#include "client.h"
#include "settings.h"
#include "roomthread.h"

class Jijiu : public OneCardViewAsSkill
{
public:
    Jijiu() : OneCardViewAsSkill("jijiu")
    {
        filter_pattern = ".|red";
        response_or_use = true;
        skill_type = Recover;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        Card *peach = Sanguosha->cloneCard("peach");
        peach->deleteLater();
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && player->getPhase() == Player::NotActive
                && peach->isAvailable(player)) {
            return Sanguosha->matchExpPatternType(pattern, peach);
        }

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        peach->setShowSkill(objectName());
        return peach;
    }
};

QingnangCard::QingnangCard()
{
}

bool QingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isWounded();
}

void QingnangCard::onEffect(const CardEffectStruct &effect) const
{
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    effect.to->getRoom()->recover(effect.to, recover);
}

class Qingnang : public OneCardViewAsSkill
{
public:
    Qingnang() : OneCardViewAsSkill("qingnang")
    {
        filter_pattern = ".|.|.|hand!";
        skill_type = Recover;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "h") && !player->hasUsed("QingnangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        qingnang_card->setSkillName(objectName());
        qingnang_card->setShowSkill(objectName());
        return qingnang_card;
    }
};

class Wushuang : public TriggerSkill
{
public:
    Wushuang() : TriggerSkill("wushuang")
    {
        events << TargetChosen << TargetConfirmed << CardFinished;
        frequency = Compulsory;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player == NULL)
            return TriggerStruct();
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetChosen) {
            if (use.card && (use.card->isKindOf("Slash") || use.card->isKindOf("Duel"))) {
                if (TriggerSkill::triggerable(player)) {
                    if (!use.to.isEmpty())
                        return TriggerStruct(objectName(), player, use.to);
                }
            }
        } else if (triggerEvent == TargetConfirmed) {
            if (!use.to.contains(player))
                return TriggerStruct();

            if (use.card && use.card->isKindOf("Duel") && TriggerSkill::triggerable(player))
                return TriggerStruct(objectName(), player, QList<ServerPlayer *>() << use.from);
        } else if (triggerEvent == CardFinished) {
            if (use.card->isKindOf("Duel")) {
                foreach (ServerPlayer *lvbu, room->getAllPlayers()) {
                    if (lvbu->getMark("WushuangTarget") > 0)
                        room->setPlayerMark(lvbu, "WushuangTarget", 0);
                }
            }
            return TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ask_who->tag["WushuangData"] = data; // for AI
        ask_who->tag["WushuangTarget"] = QVariant::fromValue(target); // for AI
        bool invoke = ask_who->hasShownSkill(this) || ask_who->askForSkillInvoke(this, QVariant::fromValue(target), info.skill_position);
        ask_who->tag.remove("WushuangData");
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", -1, ask_who, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(ask_who, objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            if (triggerEvent != TargetChosen)
                return false;

            int x = use.to.indexOf(target);
            QVariantList jink_list = ask_who->tag["Jink_" + use.card->toString()].toList();
            if (jink_list.at(x).toInt() == 1)
                jink_list[x] = 2;
            ask_who->tag["Jink_" + use.card->toString()] = jink_list;
        } else if (use.card->isKindOf("Duel"))
            room->setPlayerMark(ask_who, "WushuangTarget", 1);

        return false;
    }
};

LijianCard::LijianCard()
{
    mute = true;
}

bool LijianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!to_select->isMale())
        return false;

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();

    if (targets.length() == 1 && (to_select->isCardLimited(duel, Card::MethodUse) || to_select->isProhibited(targets.first(), duel)))
        return false;

    return targets.length() < 2 && to_select != Self;
}

bool LijianCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void LijianCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *diaochan = card_use.from;

    LogMessage log;
    log.from = diaochan;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, diaochan, data);
    room->broadcastSkillInvoke("lijian", "male", -1, diaochan, getSkillPosition());

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, diaochan->objectName(), QString(), "lijian", QString());
    room->moveCardTo(this, diaochan, NULL, Player::PlaceTable, reason, true);

    if (diaochan->ownSkill("lijian"))
        diaochan->showSkill("lijian", getSkillPosition());

    QList<int> table_ids = room->getCardIdsOnTable(this);
    if (!table_ids.isEmpty()) {
        DummyCard dummy(table_ids);
        room->moveCardTo(&dummy, diaochan, NULL, Player::DiscardPile, reason, true);
    }

    thread->trigger(CardUsed, room, diaochan, data);
    thread->trigger(CardFinished, room, diaochan, data);
}

void LijianCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *to = targets.at(0);
    ServerPlayer *from = targets.at(1);

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setSkillName(QString("_%1").arg(getSkillName()));
    if (!from->isCardLimited(duel, Card::MethodUse) && !from->isProhibited(to, duel))
        room->useCard(CardUseStruct(duel, from, to));
    else
        delete duel;
}

class Lijian : public OneCardViewAsSkill
{
public:
    Lijian() : OneCardViewAsSkill("lijian")
    {
        filter_pattern = ".!";
        skill_type = Wizzard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getAliveSiblings().length() > 1
            && player->canDiscard(player, "he") && !player->hasUsed("LijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(originalCard->getId());
        lijian_card->setSkillName(objectName());
        lijian_card->setShowSkill(objectName());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class Biyue : public PhaseChangeSkill
{
public:
    Biyue() : PhaseChangeSkill("biyue")
    {
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!PhaseChangeSkill::triggerable(player)) return TriggerStruct();
        if (player->getPhase() == Player::Finish) return TriggerStruct(objectName(), player);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan, const TriggerStruct &) const
    {
        diaochan->drawCards(1);

        return false;
    }
};

class Luanji : public ViewAsSkill
{
public:
    Luanji() : ViewAsSkill("luanji")
    {
        response_or_use = true;
        skill_type = Attack;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            aa->setShowSkill(objectName());
            return aa;
        } else
            return NULL;
    }
};

class ShuangxiongViewAsSkill : public OneCardViewAsSkill
{
public:
    ShuangxiongViewAsSkill() :OneCardViewAsSkill("shuangxiong")
    { //Client::updateProperty() / RoomScene::detachSkill()
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("shuangxiong") != 0 && !player->isKongcheng();
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (card->isEquipped())
            return false;

        int value = Self->getMark("shuangxiong");
        if (value == 1)
            return card->isBlack();
        else if (value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setShowSkill("shuangxiong");
        duel->setSkillName("_shuangxiong");
        return duel;
    }
};

class Shuangxiong : public TriggerSkill
{
public:
    Shuangxiong() : TriggerSkill("shuangxiong")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ShuangxiongViewAsSkill;
        skill_type = Attack;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player != NULL) {
            if (triggerEvent == EventPhaseStart) {
                if (player->getPhase() == Player::Start) {
                    room->setPlayerMark(player, "shuangxiong", 0);
                    return TriggerStruct();
                } else if (player->getPhase() == Player::Draw && TriggerSkill::triggerable(player))
                    return TriggerStruct(objectName(), player);
            } else if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to == Player::NotActive && player->hasFlag("shuangxiong_head"))
                    room->setPlayerFlag(player, "-shuangxiong_head");
                if (change.to == Player::NotActive && player->hasFlag("shuangxiong_deputy"))
                    room->setPlayerFlag(player, "-shuangxiong_deputy");
                return TriggerStruct();
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (shuangxiong->askForSkillInvoke(this, QVariant(), info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", 1, shuangxiong, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (shuangxiong->getPhase() == Player::Draw && TriggerSkill::triggerable(shuangxiong)) {
            room->setPlayerFlag(shuangxiong, "shuangxiong_" + info.skill_position);
            shuangxiong->tag["shuangxiong"] = QVariant::fromValue(info.skill_position);
            JudgeStruct judge;
            judge.good = true;
            judge.play_animation = false;
            judge.reason = objectName();
            judge.who = shuangxiong;

            room->judge(judge);
            room->setPlayerMark(shuangxiong, "shuangxiong", judge.pattern == "red" ? 1 : 2);

            return true;
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class ShuangxiongGet : public TriggerSkill
{
public:
    ShuangxiongGet() : TriggerSkill("#shuangxiong")
    {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "shuangxiong") {
                judge->pattern = judge->card->isRed() ? "red" : "black";
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                    QString head = player->tag["shuangxiong"].toString();
                    TriggerStruct trigger = TriggerStruct(objectName(), player);
                    trigger.skill_position = head;
                    return trigger;
                }
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->who->obtainCard(judge->card);

        return false;
    }
};

class Wansha : public TriggerSkill
{ // Gamerule::effect (AskForPeaches)
public:
    Wansha() : TriggerSkill("wansha")
    {
        events << Dying << TurnStart << GeneralShown << EventAcquireSkill << EventPhaseChanging;
        frequency = Compulsory;
        skill_type = Wizzard;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == TurnStart || triggerEvent == GeneralShown || triggerEvent == EventAcquireSkill) && player->hasShownSkill(this)
                && room->getCurrent() == player && player->getPhase() != Player::NotActive) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                room->setPlayerFlag(p, "Global_PreventPeach");

        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                foreach (ServerPlayer *p, room->getAllPlayers(true))
                    room->setPlayerFlag(p, "-Global_PreventPeach");
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *jiayu = room->getCurrent();
        if (triggerEvent == Dying && jiayu && TriggerSkill::triggerable(jiayu) && jiayu->getPhase() != Player::NotActive)
            return TriggerStruct(objectName(), jiayu);

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player, const TriggerStruct &info) const
    {
        if (player->hasShownSkill(this) || player->askForSkillInvoke(this, data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *jiaxu, const TriggerStruct &) const
    {
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            room->notifySkillInvoked(jiaxu, objectName());

            LogMessage log;
            log.from = jiaxu;
            log.arg = objectName();
            if (jiaxu != dying.who) {
                log.type = "#WanshaTwo";
                log.to << dying.who;
            } else {
                log.type = "#WanshaOne";
            }
            room->sendLog(log);
        }
        return false;
    }
};

class WanshaClear : public DetachEffectSkill
{
public:
    WanshaClear() : DetachEffectSkill("wansha")
    {
        frequency = Compulsory;
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player, QVariant &) const
    {
        if (room->getCurrent() == player)
            foreach (ServerPlayer *p, room->getAllPlayers(true))
                room->setPlayerFlag(p, "-Global_PreventPeach");
    }
};

class WanshaProhibit : public ProhibitSkill
{
public:
    WanshaProhibit() : ProhibitSkill("#wansha-prohibit")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *, const Card *card, const QList<const Player *> &) const
    {
        return card->isKindOf("Peach") && from->hasFlag("Global_PreventPeach") && !from->hasFlag("Global_Dying");
    }
};

LuanwuCard::LuanwuCard()
{
    target_fixed = true;
    mute = true;
}

void LuanwuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->setPlayerMark(card_use.from, "@chaos", 0);
    room->broadcastSkillInvoke("luanwu", "male", -1, card_use.from, getSkillPosition());
    room->doSuperLightbox("jiaxu", "luanwu");

    CardUseStruct new_use = card_use;
    new_use.to << room->getOtherPlayers(card_use.from);

    Card::onUse(room, new_use);
}

void LuanwuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    QList<ServerPlayer *> players = room->getOtherPlayers(effect.to);
    QList<int> distance_list;
    int nearest = 1000;
    foreach (ServerPlayer *player, players) {
        int distance = effect.to->distanceTo(player);
        distance_list << distance;
        if (distance != -1)
            nearest = qMin(nearest, distance);
    }

    QList<ServerPlayer *> luanwu_targets;
    for (int i = 0; i < distance_list.length(); i++) {
        if (distance_list[i] == nearest && effect.to->canSlash(players[i], NULL, false))
            luanwu_targets << players[i];
    }

    if (luanwu_targets.isEmpty() || !room->askForUseSlashTo(effect.to, luanwu_targets, "@luanwu-slash")) {
        room->loseHp(effect.to);
        room->getThread()->delay(Config.AIDelay);
    }
}

class Luanwu : public ZeroCardViewAsSkill
{
public:
    Luanwu() : ZeroCardViewAsSkill("luanwu")
    {
        frequency = Limited;
        limit_mark = "@chaos";
        skill_type = Wizzard;
    }

    virtual const Card *viewAs() const
    {
        LuanwuCard *card = new LuanwuCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@chaos") >= 1;
    }
};

class Weimu : public TriggerSkill
{
public:
    Weimu() : TriggerSkill("weimu")
    {
        events << TargetConfirming;
        frequency = Compulsory;
        skill_type = Defense;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card || use.card->getTypeId() != Card::TypeTrick || !use.card->isBlack()) return TriggerStruct();
        if (!use.to.contains(player)) return TriggerStruct();
        return TriggerStruct(objectName(), player);
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->hasShownSkill(this) || player->askForSkillInvoke(this, data, info.skill_position)) {
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

class Mengjin : public TriggerSkill
{
public:
    Mengjin() :TriggerSkill("mengjin")
    {
        events << SlashMissed;
        frequency = Frequent;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && player->canDiscard(effect.to, "he")) return TriggerStruct(objectName(), player);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (pangde->askForSkillInvoke(this, data, info.skill_position)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, pangde->objectName(), data.value<SlashEffectStruct>().to->objectName());
            room->broadcastSkillInvoke(objectName(), "male", -1, pangde, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        int to_throw = room->askForCardChosen(pangde, effect.to, "he", objectName(), false, Card::MethodDiscard);
        room->throwCard(Sanguosha->getCard(to_throw), effect.to, pangde);

        return false;
    }
};

class Leiji : public TriggerSkill
{
public:
    Leiji() : TriggerSkill("leiji")
    {
        events << CardResponded;// << FinishJudge;
        priority.insert(CardResponded, 3);
        priority.insert(FinishJudge, -1);
        skill_type = Attack;
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardResponded && TriggerSkill::triggerable(player)) {
            const Card *card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Jink"))
                player->addMark("leiji_postpone");
        }
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        if (triggerEvent == CardResponded && TriggerSkill::triggerable(player) && player->getMark("leiji_postpone") > 0) {
            const Card *card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Jink") && room->getTag("judge").toInt() == 0)
                skill_list << TriggerStruct(objectName(), player);
        } else if (triggerEvent == FinishJudge) {
            QList<ServerPlayer *> jiaozhus = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *jiaozhu, jiaozhus) {
                int postponed = jiaozhu->getMark("leiji_postpone");
                if (postponed > 0)
                    skill_list << TriggerStruct(objectName(), jiaozhu);
            }
        }

        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player, const TriggerStruct &info) const
    {
        player->removeMark("leiji_postpone");
        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "leiji-invoke", true, true, info.skill_position);
        if (target) {
            player->tag["leiji-target"] = QVariant::fromValue(target);
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        } else {
            player->tag.remove("leiji-target");
            return TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *zhangjiao, const TriggerStruct &) const
    {
        ServerPlayer *target = zhangjiao->tag["leiji-target"].value<ServerPlayer *>();
        zhangjiao->tag.remove("leiji-target");
        if (target) {
            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            if (judge.isBad())
                room->damage(DamageStruct(objectName(), zhangjiao, target, 2, DamageStruct::Thunder));
        }

        return false;
    }
};

class Guidao : public TriggerSkill
{
public:
    Guidao() : TriggerSkill("guidao")
    {
        events << AskForRetrial;
        skill_type = Wizzard;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(target))
            return TriggerStruct();

        if (target->isKongcheng() && target->getHandPile().isEmpty()) {
            bool has_black = false;
            for (int i = 0; i < 4; i++) {
                const EquipCard *equip = target->getEquip(i);
                if (equip && equip->isBlack()) {
                    has_black = true;
                    break;
                }
            }
            return (has_black) ? TriggerStruct(objectName(), target) : TriggerStruct();
        }
        return TriggerStruct(objectName(), target);
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@guidao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        const Card *card = room->askForCard(player, ".|black", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            room->retrial(card, player, judge, objectName(), true, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->updateResult();
        return false;
    }
};

class Beige : public TriggerSkill
{
public:
    Beige() : TriggerSkill("beige")
    {
        events << Damaged << FinishJudge;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<TriggerStruct> skill_list;
        if (player == NULL) return skill_list;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL || !damage.card->isKindOf("Slash") || damage.to->isDead())
                return skill_list;

            QList<ServerPlayer *> caiwenjis = room->findPlayersBySkillName(objectName());
            foreach(ServerPlayer *caiwenji, caiwenjis)
                if (caiwenji->canDiscard(caiwenji, "he"))
                    skill_list << TriggerStruct(objectName(), caiwenji);
            return skill_list;
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return skill_list;
            judge->pattern = QString::number(int(judge->card->getSuit()));
            return skill_list;
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        ServerPlayer *caiwenji = ask_who;

        if (caiwenji != NULL) {
            caiwenji->tag["beige_data"] = data;
            bool invoke = room->askForDiscard(caiwenji, objectName(), 1, 1, true, true, "@beige", true, info.skill_position);
            caiwenji->tag.remove("beige_data");

            if (invoke) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, caiwenji->objectName(), data.value<DamageStruct>().to->objectName());
                room->broadcastSkillInvoke(objectName(), "male", -1, caiwenji, info.skill_position);
                return info;
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &) const
    {
        ServerPlayer *caiwenji = ask_who;
        if (caiwenji == NULL) return false;
        DamageStruct damage = data.value<DamageStruct>();

        JudgeStruct judge;
        judge.good = true;
        judge.play_animation = false;
        judge.who = player;
        judge.reason = objectName();

        room->judge(judge);

        Card::Suit suit = (Card::Suit)(judge.pattern.toInt());
        switch (suit) {
            case Card::Heart: {
                RecoverStruct recover;
                recover.who = caiwenji;
                room->recover(player, recover);

                break;
            }
            case Card::Diamond: {
                player->drawCards(2);
                break;
            }
            case Card::Club: {
                if (damage.from && damage.from->isAlive())
                    room->askForDiscard(damage.from, "beige_discard", 2, 2, false, true);

                break;
            }
            case Card::Spade: {
                if (damage.from && damage.from->isAlive())
                    damage.from->turnOver();

                break;
            }
            default:
                break;
        }
        return false;
    }
};

class Duanchang : public TriggerSkill
{
public:
    Duanchang() : TriggerSkill("duanchang")
    {
        events << Death;
        frequency = Compulsory;
        skill_type = Wizzard;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player == NULL || !player->hasSkill(objectName())) return TriggerStruct();
        DeathStruct death = data.value<DeathStruct>();

        if (death.damage && death.damage->from) {
            ServerPlayer *target = death.damage->from;
            if (!(target->getGeneral()->objectName().contains("sujiang") && target->getGeneral2()->objectName().contains("sujiang")))
                return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
        return info;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        room->notifySkillInvoked(player, objectName());

        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *target = death.damage->from;
        QString choice = "head_general";

        if (player->getAI()) {
            QStringList choices;
            if (!target->getGeneral()->objectName().contains("sujiang"))
                choices << "head_general";

            if (!target->getGeneral2()->objectName().contains("sujiang"))
                choices << "deputy_general";

            choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(target));
        } else {
            QStringList generals;
            if (!target->getGeneral()->objectName().contains("sujiang")) {
                QString g = target->getGeneral()->objectName();
                if (g.contains("anjiang"))
                    g.append("_head");
                generals << g;
            }

            if (target->getGeneral2() && !target->getGeneral2()->objectName().contains("sujiang")) {
                QString g = target->getGeneral2()->objectName();
                if (g.contains("anjiang"))
                    g.append("_deputy");
                generals << g;
            }

            QString general = generals.first();
            if (generals.length() == 2)
                general = room->askForGeneral(player, generals.join("+"), generals.first(), true, "#duanchang:" + target->objectName(), QVariant::fromValue(target));

            if (general == target->getGeneral()->objectName() || general == "anjiang_head")
                choice = "head_general";
            else
                choice = "deputy_general";

        }
        LogMessage log;
        log.type = choice == "head_general" ? "#DuanchangLoseHeadSkills" : "#DuanchangLoseDeputySkills";
        log.from = player;
        log.to << target;
        log.arg = objectName();
        room->sendLog(log);

        QStringList duanchangList = target->property("Duanchang").toString().split(",");
        if (choice == "head_general" && !duanchangList.contains("head"))
            duanchangList << "head";
        else if (choice == "deputy_general" && !duanchangList.contains("deputy"))
            duanchangList << "deputy";
        room->setPlayerProperty(target, "Duanchang", duanchangList.join(","));

        QList<const Skill *> skills = choice == "head_general" ? target->getActualGeneral1()->getVisibleSkillList()
            : target->getActualGeneral2()->getVisibleSkillList();
        foreach (const Skill *skill, skills)
            if (!skill->isAttachedLordSkill())
                room->detachSkillFromPlayer(target, skill->objectName(), !target->hasShownSkill(skill), false, choice == "head_general" ? true : false);

        if (death.damage->from->isAlive())
            death.damage->from->gainMark("@duanchang");

        return false;
    }
};

XiongyiCard::XiongyiCard()
{
    mute = true;
    target_fixed = true;
}

void XiongyiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->setPlayerMark(card_use.from, "@arise", 0);
    room->broadcastSkillInvoke("xiongyi", "male", -1, card_use.from, getSkillPosition());
    room->doSuperLightbox("mateng", "xiongyi");
    SkillCard::onUse(room, card_use);
}

void XiongyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->isFriendWith(source)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
            targets << p;
        }
    }
    room->sortByActionOrder(targets);
    Card::use(room, source, targets);

    QStringList kingdom_list = Sanguosha->getKingdoms();
    kingdom_list << "careerist";
    bool invoke = true;
    int n = source->getPlayerNumWithSameKingdom("AI", QString(), MaxCardsType::Normal);
    foreach (const QString &kingdom, Sanguosha->getKingdoms()) {
        if (kingdom == "god") continue;
        if (source->getRole() == "careerist") {
            if (kingdom == "careerist")
                continue;
        } else if (source->getKingdom() == kingdom)
            continue;
        int other_num = source->getPlayerNumWithSameKingdom("AI", kingdom, MaxCardsType::Normal);
        if (other_num > 0 && other_num < n) {
            invoke = false;
            break;
        }
    }

    if (invoke && source->isWounded()) {
        RecoverStruct recover;
        recover.who = source;
        room->recover(source, recover);
    }
}

void XiongyiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(3);
}

class Xiongyi : public ZeroCardViewAsSkill
{
public:
    Xiongyi() : ZeroCardViewAsSkill("xiongyi")
    {
        frequency = Limited;
        limit_mark = "@arise";
        skill_type = Replenish;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@arise") >= 1;
    }

    virtual const Card *viewAs() const
    {
        XiongyiCard *card = new XiongyiCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }
};

class Mingshi : public TriggerSkill
{
public:
    Mingshi() : TriggerSkill("mingshi")
    {
        events << DamageInflicted;
        frequency = Compulsory;
        skill_type = Defense;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && !damage.from->hasShownAllGenerals())
            return TriggerStruct(objectName(), player);

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        bool invoke = player->hasShownSkill(this) ? true : player->askForSkillInvoke(this, data, info.skill_position);
        if (invoke) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#Mingshi";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(--damage.damage);
        room->sendLog(log);

        if (damage.damage < 1)
            return true;
        data = QVariant::fromValue(damage);

        return false;
    }
};

LirangCard::LirangCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LirangCard::onUse(Room *, const CardUseStruct &card_use) const
{
    QStringList targets = card_use.from->tag["lirang_target"].toStringList();
    QStringList cards = card_use.from->tag["lirang_get"].toStringList();
    targets << card_use.to.first()->objectName();
    cards.append((IntList2StringList(this->getSubcards())).join("+"));
    card_use.from->tag["lirang_target"] = targets;
    card_use.from->tag["lirang_get"] = cards;
}

class LirangViewAsSkill : public ViewAsSkill
{
public:
    LirangViewAsSkill() : ViewAsSkill("lirang")
    {
        expand_pile = "#lirang";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return Self->getPile("#lirang").contains(to_select->getId());
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@lirang");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        LirangCard *Lirang_card = new LirangCard;
        Lirang_card->addSubcards(cards);
        return Lirang_card;
    }
};

class Lirang : public TriggerSkill
{
public:
    Lirang() : TriggerSkill("lirang")
    {
        events << CardsMoveOneTime;
        view_as_skill = new LirangViewAsSkill;
        skill_type = Replenish;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!TriggerSkill::triggerable(player))
            return;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)  return;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD && move.to_place == Player::PlaceTable) {
            QStringList lirang_card = player->tag["lirang_to_judge"].toStringList();
            QList<int> cards;
            for (int i = 0; i < move.card_ids.length(); i++) {
                int card_id = move.card_ids[i];
                if (room->getCardPlace(card_id) == Player::PlaceTable && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))
                    cards << card_id;
            }
            if (!cards.isEmpty()) {
                lirang_card.append((IntList2StringList(cards)).join("+"));
                player->tag["lirang_to_judge"] = lirang_card;
            }
        }
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
		if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player || player->tag["lirang_to_judge"].toStringList().isEmpty()) return TriggerStruct();

        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            if (move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile) {
                QStringList lirang_card = player->tag["lirang_to_judge"].toStringList();
                QList<int> cards = StringList2IntList(lirang_card.last().split("+"));
                QList<int> this_cards;
                lirang_card.removeLast();
                player->tag["lirang_to_judge"] = lirang_card;
                foreach (int id, move.card_ids) {
                    if (cards.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        this_cards << id;
                }
                if (!this_cards.isEmpty()) {
                    lirang_card.append((IntList2StringList(this_cards)).join("+"));
                    player->tag["lirang_to_judge"] = lirang_card;
                    return TriggerStruct(objectName(), player);
                }
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QStringList lirang_card = player->tag["lirang_to_judge"].toStringList();
        QList<int> cards = StringList2IntList(lirang_card.last().split("+"));
        foreach (int id, StringList2IntList(lirang_card.last().split("+"))) {
            if (room->getCardPlace(id) != Player::DiscardPile)
                cards.removeOne(id);
        }
        lirang_card.removeLast();
        player->tag["lirang_to_judge"] = lirang_card;
        if (cards.isEmpty()) return TriggerStruct();

        room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, true, true);
        player->tag["lirang_this_time"] = IntList2VariantList(cards);
        const Card *card = room->askForUseCard(player, "@@lirang", "@lirang-distribute:::" + QString::number(cards.length()), -1, Card::MethodNone, true, info.skill_position);

        if (card) {
            lirang_card.append((IntList2StringList(cards)).join("+"));
            player->tag["lirang_to_judge"] = lirang_card;

            LogMessage l;
            l.type = "#InvokeSkill";
            l.from = player;
            l.arg = objectName();
            room->sendLog(l);

            return info;
        }
        room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, false, false);
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QStringList lirang_card = player->tag["lirang_to_judge"].toStringList();
        QList<int> cards = StringList2IntList(lirang_card.last().split("+"));
        lirang_card.removeLast();

        QList<CardsMoveStruct> moves;

        do {
            room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, false, false);

            QStringList targets = player->tag["lirang_target"].toStringList();
            QStringList cards_get = player->tag["lirang_get"].toStringList();
            QList<int> get = StringList2IntList(cards_get.last().split("+"));
            ServerPlayer *target;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->objectName() == targets.last())
                    target = p;
            }
            targets.removeLast();
            cards_get.removeLast();
            player->tag["lirang_target"] = targets;
            player->tag["lirang_get"] = cards_get;

            CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, player->objectName(), target->objectName(), objectName(), QString());
            CardsMoveStruct move(get, target, Player::PlaceHand, reason);
            moves.append(move);

            foreach (int id, get)
                cards.removeOne(id);

            if (!cards.isEmpty()) {
                room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, true, true);
                player->tag["lirang_this_time"] = IntList2VariantList(cards);
            }
        }

        while (!cards.isEmpty() && player->isAlive()
                    && room->askForUseCard(player, "@@lirang", "@lirang-distribute:::" + QString::number(cards.length()), -1, Card::MethodNone, true, info.skill_position));

        if (!cards.isEmpty()) room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, false, false);

        player->tag["lirang_to_judge"] = lirang_card;
        player->tag.remove("lirang_this_time");
        room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
        room->moveCardsAtomic(moves, true);

        return false;
    }
};

class Shuangren : public PhaseChangeSkill
{
public:
    Shuangren() : PhaseChangeSkill("shuangren")
    {
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *jiling, QVariant &, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(jiling)) return TriggerStruct();
        if (jiling->getPhase() == Player::Play && !jiling->isKongcheng()) {
            Room *room = jiling->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(jiling);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            return can_invoke ? TriggerStruct(objectName(), jiling) : TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *jiling, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(jiling)) {
            if (!p->isKongcheng())
                targets << p;
        }
        ServerPlayer *victim;
        if ((victim = room->askForPlayerChosen(jiling, targets, "shuangren", "@shuangren", true, true, info.skill_position)) != NULL) {
            room->broadcastSkillInvoke(objectName(), "male", 1, jiling, info.skill_position);
            PindianStruct *pd = jiling->pindianSelect(victim, "shuangren");
            jiling->tag["shuangren_pd"] = QVariant::fromValue(pd);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *jiling, const TriggerStruct &info) const
    {
        PindianStruct *pd = jiling->tag["shuangren_pd"].value<PindianStruct *>();
        jiling->tag.remove("shuangren_pd");
        if (pd != NULL) {
            ServerPlayer *target = pd->to;
            bool success = jiling->pindian(pd);
            pd = NULL;
            Room *room = jiling->getRoom();
            if (success) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (jiling->canSlash(p, NULL, false) && (p->isFriendWith(target) || target == p))
                        targets << p;
                }
                if (targets.isEmpty())
                    return false;

                ServerPlayer *slasher = room->askForPlayerChosen(jiling, targets, "shuangren-slash", "@dummy-slash");
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_shuangren");
                room->useCard(CardUseStruct(slash, jiling, slasher), false);
            } else {
                room->broadcastSkillInvoke("shuangren", "male", 3, jiling, info.skill_position);
                return true;
            }
        } else
            Q_ASSERT(false);
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class Sijian : public TriggerSkill
{
public:
    Sijian() : TriggerSkill("sijian")
    {
        events << CardsMoveOneTime;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(tianfeng)) return TriggerStruct();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == tianfeng && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            QList<ServerPlayer *> other_players = room->getOtherPlayers(tianfeng);
            foreach (ServerPlayer *p, other_players) {
                if (tianfeng->canDiscard(p, "he"))
                    return TriggerStruct(objectName(), tianfeng);
            }
        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QList<ServerPlayer *> other_players = room->getOtherPlayers(tianfeng);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, other_players) {
            if (tianfeng->canDiscard(p, "he"))
                targets << p;
        }
        ServerPlayer *to = room->askForPlayerChosen(tianfeng, targets, objectName(), "sijian-invoke", true, true, info.skill_position);
        if (to) {
            tianfeng->tag["sijian_target"] = QVariant::fromValue(to);
            room->broadcastSkillInvoke(objectName(), tianfeng);
            return info;
        } else tianfeng->tag.remove("sijian_target");
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *tianfeng, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        ServerPlayer *to = tianfeng->tag["sijian_target"].value<ServerPlayer *>();
        tianfeng->tag.remove("sijian_target");
        if (to && tianfeng->canDiscard(to, "he")) {
            int card_id = room->askForCardChosen(tianfeng, to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, to, tianfeng);
        }
        return false;
    }
};

class Suishi : public TriggerSkill
{
public:
    Suishi() : TriggerSkill("suishi")
    {
        events << Dying << Death;
        frequency = Compulsory;
    }

	virtual QList <TriggerStruct> triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList <TriggerStruct> triggers;
        QList<ServerPlayer *> tianfengs = room->findPlayersBySkillName(objectName());
        ServerPlayer *target = NULL;
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.damage && dying.damage->from)
                target = dying.damage->from;
            foreach (ServerPlayer *tianfeng, tianfengs) {
                if (player != tianfeng && target && (tianfeng->isFriendWith(target) || tianfeng->willBeFriendWith(target)))
                    triggers << TriggerStruct(objectName(), tianfeng);
            }
        } else if (triggerEvent == Death) {
            foreach (ServerPlayer *tianfeng, tianfengs)
                if (player && player != tianfeng && (tianfeng->isFriendWith(player) || tianfeng->willBeFriendWith(player)))
                    triggers << TriggerStruct(objectName(), tianfeng);
        }

        return triggers;
    }

    virtual TriggerStruct cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player, const TriggerStruct &info) const
    {
        bool invoke = player->hasShownSkill(this) ? true : player->askForSkillInvoke(this, (int)triggerEvent, info.skill_position);
        if (invoke) {
            if (triggerEvent == Dying)
                room->broadcastSkillInvoke(objectName(), "male", 1, player, info.skill_position);
            else if (triggerEvent == Death)
                room->broadcastSkillInvoke(objectName(), "male", 2, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player, const TriggerStruct &) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        if (triggerEvent == Dying)
            player->drawCards(1);
        else if (triggerEvent == Death)
            room->loseHp(player);
        return false;
    }
};

class Kuangfu : public TriggerSkill
{
public:
    Kuangfu() : TriggerSkill("kuangfu")
    {
        events << Damage;
        frequency = Frequent;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *panfeng, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(panfeng)) return TriggerStruct();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && damage.card->isKindOf("Slash") && target->hasEquip() && !damage.chain && !damage.transfer && !damage.to->hasFlag("Global_DFDebut")) {
            for (int i = 0; i < 5; i++) {
                if (!target->getEquip(i)) continue;
                if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                    return TriggerStruct(objectName(), panfeng);
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (panfeng->askForSkillInvoke(this, data, info.skill_position)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, panfeng->objectName(), data.value<DamageStruct>().to->objectName());
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;

        /*
        QStringList equiplist;
        for (int i = 0; i < 5; i++) {
            if (!target->getEquip(i)) continue;
            if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                equiplist << QString::number(i);
        }

        int equip_index = room->askForChoice(panfeng, "kuangfu_equip", equiplist.join("+"), QVariant::fromValue(target)).toInt();
        const Card *card = target->getEquip(equip_index);
        int card_id = card->getEffectiveId();

        QStringList choicelist;
        if (panfeng->canDiscard(target, card_id))
            choicelist << "throw";
        if (equip_index > -1 && panfeng->getEquip(equip_index) == NULL)
            choicelist << "move";
        */

        QList<int> disable_equiplist, equiplist;
        for (int i = 0; i < 5; i++) {
            if (target->getEquip(i) && !panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) && panfeng->getEquip(i))
                disable_equiplist << target->getEquip(i)->getEffectiveId();
            if (target->getEquip(i) && panfeng->getEquip(i))
                equiplist << target->getEquip(i)->getEffectiveId();
        }
        int card_id = room->askForCardChosen(panfeng, target, "e", objectName(), false, Card::MethodNone, disable_equiplist);
        const Card *card = Sanguosha->getCard(card_id);

        QStringList choicelist;
        if (panfeng->canDiscard(target, card_id))
            choicelist << "throw";
        if (!equiplist.contains(card_id))
            choicelist << "move";

        QString choice = room->askForChoice(panfeng, "kuangfu%log:" + card->getFullName(true), choicelist.join("+"));

        if (choice.contains("move")) {
            room->broadcastSkillInvoke(objectName(), "male", 2, panfeng, info.skill_position);
            room->moveCardTo(card, panfeng, Player::PlaceEquip);
        } else {
            room->broadcastSkillInvoke(objectName(), "male", 1, panfeng, info.skill_position);
            room->throwCard(card, target, panfeng);
        }

        return false;
    }
};

HuoshuiCard::HuoshuiCard()
{
    target_fixed = true;
    mute = true;
}

class HuoshuiVS : public ZeroCardViewAsSkill
{
public:
    HuoshuiVS() : ZeroCardViewAsSkill("huoshui")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasShownSkill(objectName());
    }

    virtual const Card *viewAs() const
    {
        HuoshuiCard *card = new HuoshuiCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }
};

class Huoshui : public TriggerSkill
{
public:
    Huoshui() : TriggerSkill("huoshui")
    {
        events << GeneralShown << EventPhaseStart << Death << EventAcquireSkill << EventLoseSkill;
        view_as_skill = new HuoshuiVS;
        skill_type = Wizzard;
    }

    void doHuoshui(Room *room, ServerPlayer *zoushi, bool set) const
    {
        if (set && !zoushi->tag["huoshui"].toBool()) {
            room->broadcastSkillInvoke(objectName(), zoushi);
            room->notifySkillInvoked(zoushi, objectName());
            foreach(ServerPlayer *p, room->getOtherPlayers(zoushi))
                room->setPlayerDisableShow(p, "hd", "huoshui");

            zoushi->tag["huoshui"] = true;
        } else if (!set && zoushi->tag["huoshui"].toBool()) {
            foreach(ServerPlayer *p, room->getOtherPlayers(zoushi))
                room->removePlayerDisableShow(p, "huoshui");

            zoushi->tag["huoshui"] = false;
        }
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player == NULL)
            return;
        if (triggerEvent != Death && !player->isAlive())
            return;
        ServerPlayer *c = room->getCurrent();
        if (c == NULL || (triggerEvent != EventPhaseStart && c->getPhase() == Player::NotActive) || c != player)
            return;

        if ((triggerEvent == GeneralShown || triggerEvent == EventPhaseStart || triggerEvent == EventAcquireSkill) && !player->hasShownSkill(this))
            return;
        if (triggerEvent == GeneralShown) {
            bool show = data.toBool() ? player->inHeadSkills(this) : player->inDeputySkills(this);
            if (!show) return;
        }
        if (triggerEvent == EventPhaseStart && !(player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
            return;
        if (triggerEvent == Death && !player->hasShownSkill(this))
            return;
        if ((triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) && data.value<InfoStruct>().info != objectName())
            return;
        if (triggerEvent == EventLoseSkill) {
            bool head = data.value<InfoStruct>().head;
            bool show;
            if (head)
                show = player->getDeputyActivedSkills(false).contains(this) && player->hasShownGeneral2() || player->getAcquiredSkills("d").contains(objectName());
            else
                show = player->getHeadActivedSkills(false).contains(this) && player->hasShownGeneral1() || player->getAcquiredSkills("h").contains(objectName());
            if (show) return;
        }
        bool set = false;
        if (triggerEvent == GeneralShown || triggerEvent == EventAcquireSkill || (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart))
            set = true;

        doHuoshui(room, player, set);
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        return QList<TriggerStruct>();
    }
};

QingchengCard::QingchengCard()
{
    handling_method = Card::MethodDiscard;
}

bool QingchengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if ((to_select->isLord() || to_select->getGeneralName().contains("sujiang")) && to_select->getGeneral2() != NULL && to_select->getGeneral2Name().contains("sujiang")) return false;
    return targets.isEmpty() && to_select != Self && to_select->hasShownAllGenerals();
}

void QingchengCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *player = effect.from, *to = effect.to;
    Room *room = player->getRoom();

    QStringList choices;
    if (!to->isLord() && !to->getGeneralName().contains("sujiang"))
        choices << to->getGeneral()->objectName();
    if (to->getGeneral2() != NULL && !to->getGeneral2Name().contains("sujiang"))
        choices << to->getGeneral2()->objectName();

    if (choices.length() == 0)
        return;
    QString choice = choices.first();
    if (choices.length() == 2)
        choice = room->askForGeneral(player, choices, QString(), true, "@qingcheng:" + to->objectName());

    to->hideGeneral(choice == to->getGeneral()->objectName());
}

class Qingcheng : public OneCardViewAsSkill
{
public:
    Qingcheng() : OneCardViewAsSkill("qingcheng")
    {
        filter_pattern = "EquipCard!";
        skill_type = Wizzard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        QingchengCard *first = new QingchengCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        first->setShowSkill(objectName());
        return first;
    }
};

void StandardPackage::addQunGenerals()
{
    General *huatuo = new General(this, "huatuo", "qun", 3); // QUN 001
    huatuo->addSkill(new Jijiu);
    huatuo->addSkill(new Qingnang);

    General *lvbu = new General(this, "lvbu", "qun", 5); // QUN 002
    lvbu->addCompanion("diaochan");
    lvbu->addSkill(new Wushuang);

    General *diaochan = new General(this, "diaochan", "qun", 3, false); // QUN 003
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);

    General *yuanshao = new General(this, "yuanshao", "qun"); // QUN 004
    yuanshao->addCompanion("yanliangwenchou");
    yuanshao->addSkill(new Luanji);

    General *yanliangwenchou = new General(this, "yanliangwenchou", "qun"); // QUN 005
    yanliangwenchou->addSkill(new Shuangxiong);
    yanliangwenchou->addSkill(new ShuangxiongGet);
    insertRelatedSkills("shuangxiong", "#shuangxiong");

    General *jiaxu = new General(this, "jiaxu", "qun", 3); // QUN 007
    jiaxu->addSkill(new Wansha);
    jiaxu->addSkill(new WanshaProhibit);
    jiaxu->addSkill(new WanshaClear);
    insertRelatedSkills("wansha", 2, "#wansha-prohibit", "#wansha-clear");
    jiaxu->addSkill(new Luanwu);
    jiaxu->addSkill(new Weimu);

    General *pangde = new General(this, "pangde", "qun"); // QUN 008
    pangde->addSkill(new Mashu("pangde"));
    pangde->addSkill(new Mengjin);

    General *zhangjiao = new General(this, "zhangjiao", "qun", 3); // QUN 010
    zhangjiao->addSkill(new Leiji);
    zhangjiao->addSkill(new Guidao);

    General *caiwenji = new General(this, "caiwenji", "qun", 3, false); // QUN 012
    caiwenji->addSkill(new Beige);
    caiwenji->addSkill(new Duanchang);

    General *mateng = new General(this, "mateng", "qun"); // QUN 013
    mateng->addSkill(new Mashu("mateng"));
    mateng->addSkill(new Xiongyi);

    General *kongrong = new General(this, "kongrong", "qun", 3); // QUN 014
    kongrong->addSkill(new Mingshi);
    kongrong->addSkill(new Lirang);

    General *jiling = new General(this, "jiling", "qun"); // QUN 015
    jiling->addSkill(new Shuangren);
    jiling->addSkill(new SlashNoDistanceLimitSkill("shuangren"));
    insertRelatedSkills("shuangren", "#shuangren-slash-ndl");

    General *tianfeng = new General(this, "tianfeng", "qun", 3); // QUN 016
    tianfeng->addSkill(new Sijian);
    tianfeng->addSkill(new Suishi);

    General *panfeng = new General(this, "panfeng", "qun"); // QUN 017
    panfeng->addSkill(new Kuangfu);

    General *zoushi = new General(this, "zoushi", "qun", 3, false); // QUN 018
    zoushi->addSkill(new Huoshui);
    zoushi->addSkill(new Qingcheng);

    addMetaObject<QingnangCard>();
    addMetaObject<LijianCard>();
    addMetaObject<LirangCard>();
    addMetaObject<LuanwuCard>();
    addMetaObject<XiongyiCard>();
    addMetaObject<HuoshuiCard>();
    addMetaObject<QingchengCard>();
}
