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

#include "standard-wei-generals.h"
#include "skill.h"
#include "engine.h"
#include "standard-basics.h"
#include "standard-tricks.h"
#include "client.h"
#include "settings.h"
#include "roomthread.h"

class Jianxiong : public MasochismSkill
{
public:
    Jianxiong() : MasochismSkill("jianxiong")
    {
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (MasochismSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            const Card *card = damage.card;
            if (damage.card == NULL)
                return TriggerStruct();

            QList<int> table_cardids = room->getCardIdsOnTable(card);

            return (table_cardids.length() != 0 && card->getSubcards() == table_cardids) ? TriggerStruct(objectName(), player) : TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage, const TriggerStruct &) const
    {
        player->obtainCard(damage.card);
    }
};

class Fankui : public MasochismSkill
{
public:
    Fankui() : MasochismSkill("fankui")
    {
        skill_type = Wizzard;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *simayi, QVariant &data, ServerPlayer *) const
    {
        if (MasochismSkill::triggerable(simayi)) {
            ServerPlayer *from = data.value<DamageStruct>().from;
            return (from && simayi->canGetCard(from, "he")) ? TriggerStruct(objectName(), simayi) : TriggerStruct();
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *simayi, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from->isNude() && simayi->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, simayi, info.skill_position);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, simayi->objectName(), damage.from->objectName());
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage, const TriggerStruct &) const
    {
        Room *room = simayi->getRoom();
        int aidelay = Config.AIDelay;
        Config.AIDelay = 0;
        int card_id = room->askForCardChosen(simayi, damage.from, "he", objectName(), false, Card::MethodGet);
        Config.AIDelay = aidelay;
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
        room->obtainCard(simayi, Sanguosha->getCard(card_id), reason, false);
    }
};

class Guicai : public TriggerSkill
{
public:
    Guicai() : TriggerSkill("guicai")
    {
        events << AskForRetrial;
        skill_type = Wizzard;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player)) return TriggerStruct();
        if (player->isKongcheng() && player->getHandPile().isEmpty())
            return TriggerStruct();
        return TriggerStruct(objectName(), player);
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@guicai-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");

        const Card *card = room->askForCard(player, ".", prompt, data, Card::MethodResponse, judge->who, true);

        if (card) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            room->retrial(card, player, judge, objectName(), false, info.skill_position);
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

class Ganglie : public MasochismSkill
{
public:
    Ganglie() : MasochismSkill("ganglie")
    {
        skill_type = Defense;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != NULL)
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage, const TriggerStruct &) const
    {
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();

        JudgeStruct judge;
        judge.pattern = ".|heart";
        judge.good = false;
        judge.reason = objectName();
        judge.who = xiahou;

        room->judge(judge);
        if (!from || from->isDead()) return;
        if (judge.isGood()) {
            if (from->getHandcardNum() < 2 || !room->askForDiscard(from, objectName(), 2, 2, true))
                room->damage(DamageStruct(objectName(), xiahou, from));
        }
    }
};

class Tuxi : public TriggerSkill
{
public:
    Tuxi() : TriggerSkill("tuxi")
    {
        events << EventPhaseStart << EventPhaseProceeding;
        skill_type = Attack;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseProceeding && player->getPhase() == Player::Draw && player->hasFlag(objectName()))
            data = -1;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(player) || triggerEvent != EventPhaseStart)
            return TriggerStruct();

        if (player->getPhase() == Player::Draw) {
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(player);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke)
                return TriggerStruct(objectName(), player);
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        QList<ServerPlayer *> to_choose;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->canGetCard(p, "h"))
                to_choose << p;
        }

        QList<ServerPlayer *> choosees = room->askForPlayersChosen(player, to_choose, objectName(), 0, 2, "@tuxi-card", true, info.skill_position);
        if (choosees.length() > 0) {
            room->sortByActionOrder(choosees);
            player->tag["tuxi_invoke"] = QVariant::fromValue(choosees);
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }

        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *source, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        QList<ServerPlayer *> targets = source->tag["tuxi_invoke"].value<QList<ServerPlayer *> >();
        source->tag.remove("tuxi_invoke");

        QList<CardsMoveStruct> moves;
        CardsMoveStruct move1;
        move1.card_ids << room->askForCardChosen(source, targets[0], "h", "tuxi", false, Card::MethodGet);
        move1.to = source;
        move1.to_place = Player::PlaceHand;
        moves.push_back(move1);
        if (targets.length() == 2) {
            CardsMoveStruct move2;
            move2.card_ids << room->askForCardChosen(source, targets[1], "h", "tuxi", false, Card::MethodGet);
            move2.to = source;
            move2.to_place = Player::PlaceHand;
            moves.push_back(move2);
        }
        room->moveCardsAtomic(moves, false);

        if (ServerInfo.SkillModify.contains("xunxun")) {
            source->setFlags(objectName());
            return false;
        } else
            return true;
    }
};

class Luoyi : public TriggerSkill
{
public:
    Luoyi() : TriggerSkill("luoyi")
    {
        events << EventPhaseProceeding << PreCardUsed;
        skill_type = Attack;
    }

    virtual TriggerStruct triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseProceeding) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw && data.toInt() > 0)
                return TriggerStruct(objectName(), player);
        } else {
            if (player != NULL && player->isAlive() && player->hasFlag("luoyi")) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isKindOf("Duel"))) {
                    room->setCardFlag(use.card, objectName());
                }
            }
        }
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            data = data.toInt() - 1;
            room->broadcastSkillInvoke(objectName(), "male", 2, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        player->tag[objectName()] = QVariant::fromValue(info.skill_position);
        room->setPlayerFlag(player, objectName());

        return false;
    }
};

class LuoyiDamage : public TriggerSkill
{
public:
    LuoyiDamage() : TriggerSkill("luoyi-damage")
    {
        events << DamageCaused;
        frequency = Skill::Compulsory;
        global = true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player != NULL && player->isAlive() && player->hasFlag("luoyi")) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->hasFlag("luoyi") && !damage.chain && !damage.transfer && damage.by_user) {
                TriggerStruct trigger = TriggerStruct(objectName(), player);
                trigger.skill_position = player->tag["luoyi"].toString();
                return trigger;
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        room->broadcastSkillInvoke("luoyi", "male", 1, player, info.skill_position);
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#LuoyiBuff";
        log.from = player;
        log.to << damage.to;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);

        return false;
    }
};

class Tiandu : public TriggerSkill
{
public:
    Tiandu() : TriggerSkill("tiandu")
    {
        frequency = Frequent;
        events << FinishJudge;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && TriggerSkill::triggerable(player))
            return TriggerStruct(objectName(), player);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        player->obtainCard(judge->card);
        return false;
    }
};

class Yiji : public MasochismSkill
{
public:
    Yiji() : MasochismSkill("yiji")
    {
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            TriggerStruct trigger = TriggerStruct(objectName(), player);
            trigger.times = damage.damage;
            return trigger;
        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *guojia, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (guojia->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, guojia, info.skill_position);
            return info;
        }

		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *guojia, const DamageStruct &, const TriggerStruct &) const
    {
        Room *room = guojia->getRoom();

        QList<ServerPlayer *> _guojia;
        _guojia.append(guojia);
        QList<int> yiji_cards = room->getNCards(2);

        CardMoveReason preview_reason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString());

        CardsMoveStruct move(yiji_cards, NULL, guojia, Player::PlaceTable, Player::PlaceHand, preview_reason);
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _guojia);
        room->notifyMoveCards(false, moves, false, _guojia);
        QList<int> origin_yiji = yiji_cards;
        while (room->askForYiji(guojia, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
            CardsMoveStruct move(QList<int>(), guojia, NULL, Player::PlaceHand, Player::PlaceTable, preview_reason);
            foreach (int id, origin_yiji) {
                if (room->getCardPlace(id) != Player::DrawPile) {
                    move.card_ids << id;
                    yiji_cards.removeOne(id);
                }
            }
            origin_yiji = yiji_cards;
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);
            if (!guojia->isAlive())
                return;
        }

        if (!yiji_cards.isEmpty()) {
            CardsMoveStruct move(yiji_cards, guojia, NULL, Player::PlaceHand, Player::PlaceTable, preview_reason);
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);

            foreach (int id, yiji_cards)
                guojia->obtainCard(Sanguosha->getCard(id), false);

        }
    }
};

class Luoshen : public TriggerSkill
{
public:
    Luoshen() : TriggerSkill("luoshen")
    {
        events << EventPhaseStart;
        frequency = Frequent;
        skill_type = Replenish;
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Start) {
            if (TriggerSkill::triggerable(player))
                return TriggerStruct(objectName(), player);
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

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhenji, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        zhenji->tag["luoshen_position"] = QVariant::fromValue(info.skill_position);
        JudgeStruct judge;
        judge.pattern = ".|black";
        judge.good = true;
        judge.reason = objectName();
        judge.play_animation = false;
        judge.who = zhenji;
        judge.time_consuming = true;

        do {
            room->judge(judge);
        } while (judge.isGood() && zhenji->askForSkillInvoke(objectName(), QVariant(), info.skill_position));

        QList<int> card_list = VariantList2IntList(zhenji->tag[objectName()].toList());
        zhenji->tag.remove(objectName());
        QList<int> subcards;
        foreach(int id, card_list)
            if (room->getCardPlace(id) == Player::PlaceTable && !subcards.contains(id))
                subcards << id;
        if (subcards.length() != 0) {
            DummyCard dummy(subcards);
            zhenji->obtainCard(&dummy);
        }

        return false;
    }
};

class LuoshenMove : public TriggerSkill
{
public:
    LuoshenMove() : TriggerSkill("#luoshen-move")
    {
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player != NULL) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "luoshen") {
                if (judge->isGood()) {
                    QVariantList luoshen_list = player->tag["luoshen"].toList();
                    luoshen_list << judge->card->getEffectiveId();
                    player->tag["luoshen"] = luoshen_list;

                    if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                        TriggerStruct trigger = TriggerStruct(objectName(), player);
                        trigger.skill_position = player->tag["luoshen_position"].toString();
                        return trigger;
                    }
                }
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhenji, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, zhenji->objectName(), QString(), judge->reason);
        room->moveCardTo(judge->card, NULL, Player::PlaceTable, reason, true);

        return false;
    }
};

class Qingguo : public OneCardViewAsSkill
{
public:
    Qingguo() : OneCardViewAsSkill("qingguo")
    {
        filter_pattern = ".|black|.|hand";
        response_pattern = "jink";
        response_or_use = true;
        skill_type = Defense;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(originalCard->getId());
        jink->setShowSkill(objectName());
        return jink;
    }
};

ShensuCard::ShensuCard()
{
    mute = true;
}

bool ShensuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shensu");
    slash->setDistanceLimit(false);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void ShensuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shensu");
    slash->setDistanceLimit(false);
    slash->deleteLater();
    foreach (ServerPlayer *target, targets)
        if (!source->canSlash(target, slash))
            targets.removeOne(target);

    if (targets.length() > 0) {
        QString index = "2";
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            index = "1";

        QVariantList target_list;
        foreach (ServerPlayer *target, targets) {
            target_list << QVariant::fromValue(target);
        }

        source->tag["shensu_invoke" + index] = target_list;
        source->setFlags("shensu" + index);
    }
}

class ShensuViewAsSkill : public ViewAsSkill
{
public:
    ShensuViewAsSkill() : ViewAsSkill("shensu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@shensu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")) {
            if (cards.isEmpty()) {
                ShensuCard *shensu = new ShensuCard;
                shensu->setSkillName(objectName());
                return shensu;
            }
        } else if (cards.length() == 1) {
            ShensuCard *card = new ShensuCard;
            card->setSkillName(objectName());
            card->addSubcards(cards);
            return card;
        }
        return NULL;
    }
};

class Shensu : public TriggerSkill
{
public:
    Shensu() : TriggerSkill("shensu")
    {
        events << EventPhaseChanging;
        view_as_skill = new ShensuViewAsSkill;
        skill_type = Attack;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *) const
    {
        if (!TriggerSkill::triggerable(xiahouyuan) || !Slash::IsAvailable(xiahouyuan))
            return TriggerStruct();

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge) && !xiahouyuan->isSkipped(Player::Draw)) {
            xiahouyuan->tag.remove("shensu_invoke1");
            return TriggerStruct(objectName(), xiahouyuan);
        } else if (change.to == Player::Play && xiahouyuan->canDiscard(xiahouyuan, "he") && !xiahouyuan->isSkipped(Player::Play)) {
            xiahouyuan->tag.remove("shensu_invoke2");
            return TriggerStruct(objectName(), xiahouyuan);
        }

        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && room->askForUseCard(xiahouyuan, "@@shensu1", "@shensu1", 1, Card::MethodUse, true, info.skill_position)) {
            if (xiahouyuan->hasFlag("shensu1") && xiahouyuan->tag.contains("shensu_invoke1")) {
                xiahouyuan->skip(Player::Judge);
                xiahouyuan->skip(Player::Draw);
                return info;
            }
        } else if (change.to == Player::Play && room->askForUseCard(xiahouyuan, "@@shensu2", "@shensu2", 2, Card::MethodDiscard, true, info.skill_position)) {
            if (xiahouyuan->hasFlag("shensu2") && xiahouyuan->tag.contains("shensu_invoke2")) {
                xiahouyuan->skip(Player::Play);
                return info;
            }
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        QVariantList target_list;
        if (change.to == Player::Judge) {
            target_list = player->tag["shensu_invoke1"].toList();
            player->tag.remove("shensu_invoke1");
        } else {
            target_list = player->tag["shensu_invoke2"].toList();
            player->tag.remove("shensu_invoke2");
        }

        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setDistanceLimit(false);
        slash->setSkillName("_shensu");
        QList<ServerPlayer *> targets;
        foreach (QVariant x, target_list)
            targets << x.value<ServerPlayer *>();

        room->useCard(CardUseStruct(slash, player, targets));
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->getSubcards().length() + 1;
    }
};

QiaobianCard::QiaobianCard()
{
    mute = true;
}

bool QiaobianCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("qiaobianPhase");
    if (phase == Player::Draw)
        return targets.length() <= 2 && !targets.isEmpty();
    else if (phase == Player::Play)
        return targets.length() == 1;
    return false;
}

bool QiaobianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Player::Phase phase = (Player::Phase)Self->getMark("qiaobianPhase");
    if (phase == Player::Draw)
        return targets.length() < 2 && to_select != Self && Self->canGetCard(to_select, "h");
    else if (phase == Player::Play) {
        if (!targets.isEmpty())
            return false;
        foreach (const Card *card, to_select->getEquips()) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            Q_ASSERT(equip);
            int equip_index = static_cast<int>(equip->location());
            foreach (const Player *p, to_select->getSiblings()) {
                if (!p->getEquip(equip_index))
                    return true;
            }
        }
        foreach (const Card *card, to_select->getJudgingArea()) {
            foreach (const Player *p, to_select->getSiblings()) {
                if (!p->containsTrick(card->objectName()))
                    return true;
            }
        }
    }
    return false;
}

void QiaobianCard::use(Room *room, ServerPlayer *zhanghe, QList<ServerPlayer *> &targets) const
{
    Player::Phase phase = (Player::Phase)zhanghe->getMark("qiaobianPhase");
    if (phase == Player::Draw) {
        if (targets.isEmpty())
            return;

        QList<CardsMoveStruct> moves;
        CardsMoveStruct move1;
        move1.card_ids << room->askForCardChosen(zhanghe, targets[0], "h", "qiaobian", false, Card::MethodGet);
        move1.to = zhanghe;
        move1.to_place = Player::PlaceHand;
        moves.push_back(move1);
        if (targets.length() == 2) {
            CardsMoveStruct move2;
            move2.card_ids << room->askForCardChosen(zhanghe, targets[1], "h", "qiaobian", false, Card::MethodGet);
            move2.to = zhanghe;
            move2.to_place = Player::PlaceHand;
            moves.push_back(move2);
        }
        room->moveCardsAtomic(moves, false);
    } else if (phase == Player::Play) {
        if (targets.isEmpty())
            return;

        ServerPlayer *from = targets.first();
        if (from->getCards("ej").isEmpty())
            return;

        int card_id = room->askForCardChosen(zhanghe, from, "ej", "qiaobian");
        const Card *card = Sanguosha->getCard(card_id);
        Player::Place place = room->getCardPlace(card_id);

        int equip_index = -1;
        if (place == Player::PlaceEquip) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            equip_index = static_cast<int>(equip->location());
        }

        QList<ServerPlayer *> tos;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (equip_index != -1) {
                if (p->getEquip(equip_index) == NULL)
                    tos << p;
            } else {
                if (!zhanghe->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                    tos << p;
            }
        }

        room->setTag("QiaobianTarget", QVariant::fromValue(from));
        QString position = getSkillPosition();
        ServerPlayer *to = room->askForPlayerChosen(zhanghe, tos, "qiaobian", "@qiaobian-to:::" + card->objectName(), false, false, position);
        if (to) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, from->objectName(), to->objectName());

            room->moveCardTo(card, from, to, place,
                CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
                zhanghe->objectName(), "qiaobian", QString()));

            if (place == Player::PlaceDelayedTrick) {
                CardUseStruct use(card, NULL, to);
                QVariant _data = QVariant::fromValue(use);
                room->getThread()->trigger(TargetConfirming, room, to, _data);
                CardUseStruct new_use = _data.value<CardUseStruct>();
                if (new_use.to.isEmpty())
                    card->onNullified(to);

                foreach(ServerPlayer *p, room->getAllPlayers())
                    room->getThread()->trigger(TargetConfirmed, room, p, _data);
            }
        }
        room->removeTag("QiaobianTarget");
    }
}

class QiaobianViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QiaobianViewAsSkill() : ZeroCardViewAsSkill("qiaobian")
    {
        response_pattern = "@@qiaobian";
    }

    virtual const Card *viewAs() const
    {
        QiaobianCard *card = new QiaobianCard;
        card->setSkillName(objectName());
        return card;
    }
};

class Qiaobian : public TriggerSkill
{
public:
    Qiaobian() : TriggerSkill("qiaobian")
    {
        events << EventPhaseChanging;
        view_as_skill = new QiaobianViewAsSkill;
        skill_type = Wizzard;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        room->setPlayerMark(player, "qiaobianPhase", (int)change.to);
        int index = 0;
        switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
			case Player::NotActive: return TriggerStruct();

            case Player::Judge: index = 1; break;
            case Player::Draw: index = 2; break;
            case Player::Play: index = 3; break;
            case Player::Discard: index = 4; break;
            case Player::PhaseNone: Q_ASSERT(false);
        }
        if (TriggerSkill::triggerable(player) && index > 0 && player->canDiscard(player, "h"))
            return TriggerStruct(objectName(), player);
        return TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        /*
                int index = 0;
                switch (change.to) {
                case Player::RoundStart:
                case Player::Start:
                case Player::Finish:
                case Player::NotActive: return false;

                case Player::Judge: index = 1 ;break;
                case Player::Draw: index = 2; break;
                case Player::Play: index = 3; break;
                case Player::Discard: index = 4; break;
                case Player::PhaseNone: Q_ASSERT(false);
                }
                */

        static QStringList phase_strings;
        if (phase_strings.isEmpty())
            phase_strings << "round_start" << "start" << "judge" << "draw"
            << "play" << "discard" << "finish" << "not_active";
        int index = static_cast<int>(change.to);

        QString discard_prompt = QString("#qiaobian:::%1").arg(phase_strings[index]);

        if (room->askForDiscard(zhanghe, objectName(), 1, 1, true, false, discard_prompt, true, info.skill_position)) {
            room->broadcastSkillInvoke("qiaobian", "male", -1, zhanghe, info.skill_position);
            if (!zhanghe->isAlive()) return TriggerStruct();
            if (!zhanghe->isSkipped(change.to)) // warning!
                return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhanghe, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        zhanghe->skip(change.to);
        int index = 0;
        switch (change.to) {
            case Player::RoundStart:
            case Player::Start:
            case Player::Finish:
            case Player::NotActive: return false;

            case Player::Judge: index = 1; break;
            case Player::Draw: index = 2; break;
            case Player::Play: index = 3; break;
            case Player::Discard: index = 4; break;
            case Player::PhaseNone: Q_ASSERT(false);
        }
        if (index == 2 || index == 3) {
            QString use_prompt = QString("@qiaobian-%1").arg(index);
            room->askForUseCard(zhanghe, "@@qiaobian", use_prompt, index, Card::MethodUse, true, info.skill_position);
        }
        return false;
    }
};

class Duanliang : public OneCardViewAsSkill
{
public:
    Duanliang() : OneCardViewAsSkill("duanliang")
    {
        filter_pattern = "BasicCard,EquipCard|black";
        response_or_use = true;
        skill_type = Alter;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            Card *card = Sanguosha->cloneCard("supply_shortage");
            card->deleteLater();
            if (Sanguosha->matchExpPatternType(pattern, card)) return true;
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->setShowSkill(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class DuanliangTargetMod : public TargetModSkill
{
public:
    DuanliangTargetMod() : TargetModSkill("#duanliang-target")
    {
        pattern = "SupplyShortage";
    }

    virtual bool getDistanceLimit(const Player *from, const Player *to, const Card *card) const
    {
        if (!Sanguosha->matchExpPattern(pattern, from, card) || !to)
            return false;

        if (from->hasSkill("duanliang") && from->distanceTo(to, 0, card) == 2)
            return true;
        else
            return false;
    }
};

class Jushou : public PhaseChangeSkill
{
public:
    Jushou() : PhaseChangeSkill("jushou")
    {
        frequency = Frequent;
        skill_type = Defense;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return (PhaseChangeSkill::triggerable(player) && player->getPhase() == Player::Finish) ? TriggerStruct(objectName(), player) : TriggerStruct();
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
    {
        if (player->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, player, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool onPhaseChange(ServerPlayer *caoren, const TriggerStruct &) const
    {
        caoren->drawCards(3, objectName());
        caoren->turnOver();
        return false;
    }
};

QiangxiCard::QiangxiCard()
{
}

bool QiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return Self->inMyAttackRange(to_select, this);
}

void QiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (card_use.card->getSubcards().isEmpty())
        room->loseHp(card_use.from);

    SkillCard::extraCost(room, card_use);
}

void QiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->damage(DamageStruct("qiangxi", effect.from, effect.to));
}

class Qiangxi : public ViewAsSkill
{
public:
    Qiangxi() : ViewAsSkill("qiangxi")
    {
        skill_type = Attack;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) {
            QiangxiCard *card = new QiangxiCard;
            card->setSkillName(objectName());
            card->setShowSkill(objectName());
            return card;
        } else if (cards.length() == 1) {
            QiangxiCard *card = new QiangxiCard;
            card->addSubcards(cards);
            card->setSkillName(objectName());
            card->setShowSkill(objectName());
            return card;
        } else
            return NULL;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return 2 - card->subcardsLength();
    }
};

QuhuCard::QuhuCard()
{
}

bool QuhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && !to_select->isKongcheng();
}

void QuhuCard::extraCost(Room *, const CardUseStruct &card_use) const
{
    ServerPlayer *xunyu = card_use.from;
    PindianStruct *pd = xunyu->pindianSelect(card_use.to.first(), "quhu");
    xunyu->tag["quhu_pd"] = QVariant::fromValue(pd);
}

void QuhuCard::onEffect(const CardEffectStruct &effect) const
{
    PindianStruct *pd = effect.from->tag["quhu_pd"].value<PindianStruct *>();
    effect.from->tag.remove("quhu_pd");
    if (pd != NULL) {
        bool success = effect.from->pindian(pd);
        pd = NULL;

        Room *room = effect.to->getRoom();
        if (success) {
            QList<ServerPlayer *> players = room->getOtherPlayers(effect.to), wolves;
            foreach (ServerPlayer *player, players) {
                if (effect.to->inMyAttackRange(player))
                    wolves << player;
            }

            if (wolves.isEmpty()) {
                LogMessage log;
                log.type = "#QuhuNoWolf";
                log.from = effect.from;
                log.to << effect.to;
                room->sendLog(log);

                return;
            }

            ServerPlayer *wolf = room->askForPlayerChosen(effect.from, wolves, "quhu", QString("@quhu-damage:%1").arg(effect.to->objectName()), false, false, getSkillPosition());

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.to->objectName(), wolf->objectName());
            room->damage(DamageStruct("quhu", effect.to, wolf));
        } else {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.to->objectName(), effect.from->objectName());
            room->damage(DamageStruct("quhu", effect.to, effect.from));
        }
    } else
        Q_ASSERT(false);
}

class Quhu : public ZeroCardViewAsSkill
{
public:
    Quhu() : ZeroCardViewAsSkill("quhu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QuhuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        QuhuCard *card = new QuhuCard;
        card->setSkillName(objectName());
        card->setShowSkill(objectName());
        return card;
    }
};

class Jieming : public MasochismSkill
{
public:
    Jieming() : MasochismSkill("jieming")
    {
        skill_type = Replenish;
    }

    virtual TriggerStruct triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player != NULL && player->isAlive() && player->hasSkill(objectName())) {
            DamageStruct damage = data.value<DamageStruct>();
            TriggerStruct trigger = TriggerStruct(objectName(), player);
            trigger.times = damage.damage;
            return trigger;
        }

        return TriggerStruct();
    }


    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        if (!player->isAlive())
            return TriggerStruct();

        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "jieming-invoke", true, true, info.skill_position);
        if (target != NULL) {
            room->broadcastSkillInvoke(objectName(), "male", (target == player ? 2 : 1), player, info.skill_position);

            QStringList target_list = player->tag["jieming_target"].toStringList();
            target_list.append(target->objectName());
            player->tag["jieming_target"] = target_list;

            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &, const TriggerStruct &) const
    {
        QStringList target_list = xunyu->tag["jieming_target"].toStringList();
        QString target_name = target_list.last();
        target_list.removeLast();
        xunyu->tag["jieming_target"] = target_list;

        ServerPlayer *to = NULL;

        foreach (ServerPlayer *p, xunyu->getRoom()->getPlayers()) {
            if (p->objectName() == target_name) {
                to = p;
                break;
            }
        }

        if (to != NULL) {
            int upper = qMin(5, to->getMaxHp());
            int x = upper - to->getHandcardNum();
            if (x > 0)
                to->drawCards(x);
        }
    }
};

class Xingshang : public TriggerSkill
{
public:
    Xingshang() : TriggerSkill("xingshang")
    {
        events << Death;
        frequency = Frequent;
        skill_type = Replenish;
    }

    QList<TriggerStruct> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<TriggerStruct> triggers;
        if (player->isNude()) return triggers;
        QList<ServerPlayer *> caopis = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *caopi, caopis)
            if (caopi != player)
                triggers << TriggerStruct(objectName(), caopi);

        return triggers;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *caopi, const TriggerStruct &info) const
    {
        if (!player->isNude() && caopi->askForSkillInvoke(objectName(), data, info.skill_position)) {
            room->broadcastSkillInvoke(objectName(), "male", -1, caopi, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *caopi, const TriggerStruct &) const
    {
        if (player->isNude() || caopi == player)
            return false;
        DummyCard dummy(player->handCards());
        dummy.addSubcards(player->getEquips());
        if (dummy.subcardsLength() > 0) {
            CardMoveReason reason(CardMoveReason::S_REASON_RECYCLE, caopi->objectName());
            room->obtainCard(caopi, &dummy, reason, false);
        }
        return false;
    }
};

class Fangzhu : public MasochismSkill
{
public:
    Fangzhu() : MasochismSkill("fangzhu")
    {
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
    {
        ServerPlayer *to = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "fangzhu-invoke", true, true, info.skill_position);
        if (to != NULL) {
            room->broadcastSkillInvoke(objectName(), "male", (to->faceUp() ? 1 : 2), player, info.skill_position);
            //player->tag["fangzhu_invoke"] = QVariant::fromValue(to);
            QStringList target_list = player->tag["fangzhu_target"].toStringList();
            target_list.append(to->objectName());
            player->tag["fangzhu_target"] = target_list;
            return info;
        }
		return TriggerStruct();
    }

    virtual void onDamaged(ServerPlayer *caopi, const DamageStruct &, const TriggerStruct &) const
    {
        //ServerPlayer *to = caopi->tag["fangzhu_invoke"].value<ServerPlayer *>();
        QStringList target_list = caopi->tag["fangzhu_target"].toStringList();
        QString target_name = target_list.last();
        target_list.removeLast();
        caopi->tag["fangzhu_target"] = target_list;
        ServerPlayer *to = NULL;
        foreach (ServerPlayer *p, caopi->getRoom()->getAllPlayers()) {
            if (p->objectName() == target_name) {
                to = p;
                break;
            }
        }

        if (to) {
            if (caopi->isWounded())
                to->drawCards(caopi->getLostHp(), objectName());
            to->turnOver();
        }
    }
};

class Xiaoguo : public TriggerSkill
{
public:
    Xiaoguo() : TriggerSkill("xiaoguo")
    {
        events << EventPhaseStart;
        skill_type = Attack;
    }

    virtual QList<TriggerStruct> triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        QList<TriggerStruct> skill_list;
        if (player != NULL && player->getPhase() == Player::Finish) {
            QList<ServerPlayer *> yuejins = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *yuejin, yuejins) {
                if (yuejin != NULL && player != yuejin && yuejin->canDiscard(yuejin, "h"))
                    skill_list << TriggerStruct(objectName(), yuejin);
            }
        }
        return skill_list;
    }

    virtual TriggerStruct cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (room->askForCard(ask_who, ".Basic", "@xiaoguo", QVariant(), objectName())) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ask_who->objectName(), player->objectName());
            room->broadcastSkillInvoke(objectName(), "male", 1, ask_who, info.skill_position);
            return info;
        }
        return TriggerStruct();
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who, const TriggerStruct &info) const
    {
        if (!room->askForCard(player, ".Equip", "@xiaoguo-discard", QVariant())) {
            room->broadcastSkillInvoke(objectName(), "male", 2, ask_who, info.skill_position);
            room->damage(DamageStruct("xiaoguo", ask_who, player));
        } else {
            room->broadcastSkillInvoke(objectName(), "male", 3, ask_who, info.skill_position);
        }
        return false;
    }
};

void StandardPackage::addWeiGenerals()
{
    General *caocao = new General(this, "caocao", "wei"); // WEI 001
    caocao->addCompanion("dianwei");
    caocao->addCompanion("xuchu");
    caocao->addSkill(new Jianxiong);

    General *simayi = new General(this, "simayi", "wei", 3); // WEI 002
    simayi->addSkill(new Fankui);
    simayi->addSkill(new Guicai);

    General *xiahoudun = new General(this, "xiahoudun", "wei"); // WEI 003
    xiahoudun->addCompanion("xiahouyuan");
    xiahoudun->addSkill(new Ganglie);

    General *zhangliao = new General(this, "zhangliao", "wei"); // WEI 004
    zhangliao->addSkill(new Tuxi);

    General *xuchu = new General(this, "xuchu", "wei"); // WEI 005
    xuchu->addSkill(new Luoyi);

    General *guojia = new General(this, "guojia", "wei", 3); // WEI 006
    guojia->addSkill(new Tiandu);
    guojia->addSkill(new Yiji);

    General *zhenji = new General(this, "zhenji", "wei", 3, false); // WEI 007
    zhenji->addSkill(new Qingguo);
    zhenji->addSkill(new Luoshen);
    zhenji->addSkill(new LuoshenMove);
    insertRelatedSkills("luoshen", "#luoshen-move");

    General *xiahouyuan = new General(this, "xiahouyuan", "wei"); // WEI 008
    xiahouyuan->addSkill(new Shensu);

    General *zhanghe = new General(this, "zhanghe", "wei"); // WEI 009
    zhanghe->addSkill(new Qiaobian);

    General *xuhuang = new General(this, "xuhuang", "wei"); // WEI 010
    xuhuang->addSkill(new Duanliang);
    xuhuang->addSkill(new DuanliangTargetMod);
    insertRelatedSkills("duanliang", "#duanliang-target");

    General *caoren = new General(this, "caoren", "wei"); // WEI 011
    caoren->addSkill(new Jushou);

    General *dianwei = new General(this, "dianwei", "wei"); // WEI 012
    dianwei->addSkill(new Qiangxi);

    General *xunyu = new General(this, "xunyu", "wei", 3); // WEI 013
    xunyu->addSkill(new Quhu);
    xunyu->addSkill(new Jieming);

    General *caopi = new General(this, "caopi", "wei", 3); // WEI 014
    caopi->addCompanion("zhenji");
    caopi->addSkill(new Xingshang);
    caopi->addSkill(new Fangzhu);

    General *yuejin = new General(this, "yuejin", "wei", 4); // WEI 016
    yuejin->addSkill(new Xiaoguo);

    skills << new LuoyiDamage;

    addMetaObject<ShensuCard>();
    addMetaObject<QiaobianCard>();
    addMetaObject<QiangxiCard>();
    addMetaObject<QuhuCard>();
}
