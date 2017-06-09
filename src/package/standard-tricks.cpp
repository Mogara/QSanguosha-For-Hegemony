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

#include "standard-tricks.h"
#include "standard-package.h"
#include "room.h"
#include "util.h"
#include "engine.h"
#include "skill.h"
#include "json.h"
#include "roomthread.h"

AmazingGrace::AmazingGrace(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("amazing_grace");
    has_preact = true;
}

void AmazingGrace::clearRestCards(Room *room) const
{
    room->clearAG();

    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    if (ag_list.isEmpty()) return;
    DummyCard dummy(VariantList2IntList(ag_list));
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
    room->throwCard(&dummy, reason, NULL);
}

void AmazingGrace::doPreAction(Room *room, const CardUseStruct &) const
{
    QList<int> card_ids = room->getNCards(room->getAllPlayers().length());
    room->fillAG(card_ids);
    room->setTag("AmazingGrace", IntList2VariantList(card_ids));
}

void AmazingGrace::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    try {
        GlobalEffect::use(room, source, targets);
        clearRestCards(room);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            clearRestCards(room);
        throw triggerEvent;
    }
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->setEmotion(effect.from, "amazing_grace");
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    QList<int> card_ids;
    foreach(QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int card_id = room->askForAG(effect.to, card_ids, false, objectName());
    card_ids.removeOne(card_id);

    room->takeAG(effect.to, card_id);
    ag_list.removeOne(card_id);

    room->setTag("AmazingGrace", ag_list);
}

GodSalvation::GodSalvation(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("god_salvation");
}

bool GodSalvation::isCancelable(const CardEffectStruct &effect) const
{
    return effect.to->isWounded() && TrickCard::isCancelable(effect);
}

void GodSalvation::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "god_salvation");
    if (!effect.to->isWounded());
    else {
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }
}

SavageAssault::SavageAssault(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("savage_assault");
}

void SavageAssault::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "savage_assault");
    const Card *slash = room->askForCard(effect.to,
        "slash",
        "savage-assault-slash:" + effect.from->objectName(),
        QVariant::fromValue(effect),
        Card::MethodResponse,
        effect.from->isAlive() ? effect.from : NULL);
    if (slash) {
        if (slash->getSkillName() == "Spear") room->setEmotion(effect.to, "weapon/spear");
        room->setEmotion(effect.to, "killer");
    } else {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

ArcheryAttack::ArcheryAttack(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("archery_attack");
}

void ArcheryAttack::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "archery_attack");
    const Card *jink = room->askForCard(effect.to,
        "jink",
        "archery-attack-jink:" + effect.from->objectName(),
        QVariant::fromValue(effect),
        Card::MethodResponse,
        effect.from->isAlive() ? effect.from : NULL);
    if (jink && jink->getSkillName() != "EightDiagram")
        room->setEmotion(effect.to, "jink");

    if (!jink) {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

Collateral::Collateral(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("collateral");
}

bool Collateral::isAvailable(const Player *player) const
{
    bool canUse = false;
    foreach (const Player *p, player->getAliveSiblings()) {
        if (p->getWeapon()) {
            canUse = true;
            break;
        }
    }
    return canUse && SingleTargetTrick::isAvailable(player);
}

bool Collateral::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool Collateral::targetFilter(const QList<const Player *> &targets,
    const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        // @todo: fix this. We should probably keep the codes here, but change the code in
        // roomscene such that if it is collateral, then targetFilter's result is overriden
        Q_ASSERT(targets.length() <= 2);
        if (targets.length() == 2) return false;
        const Player *slashFrom = targets[0];
        return slashFrom->canSlash(to_select);
    } else {
        if (!to_select->getWeapon() || to_select == Self)
            return false;
        foreach (const Player *p, to_select->getAliveSiblings()) {
            if (to_select->canSlash(p))
                return true;
        }
    }
    return false;
}

void Collateral::onUse(Room *room, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.at(0);
    ServerPlayer *victim = card_use.to.at(1);

    CardUseStruct new_use = card_use;
    new_use.to.removeAt(1);
    killer->tag["collateralVictim"] = QVariant::fromValue(victim);

    SingleTargetTrick::onUse(room, new_use);
}

bool Collateral::doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const
{
    bool useSlash = false;
    if (killer->canSlash(victim, NULL, false))
        useSlash = room->askForUseSlashTo(killer, victim, prompt);
    return useSlash;
}

void Collateral::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    room->setEmotion(source, "collateral");
    ServerPlayer *killer = effect.to;
    ServerPlayer *victim = effect.to->tag["collateralVictim"].value<ServerPlayer *>();
    effect.to->tag.remove("collateralVictim");
    if (!victim) return;
    WrappedCard *weapon = killer->getWeapon();

    QString prompt = QString("collateral-slash:%1:%2").arg(victim->objectName()).arg(source->objectName());

    if (victim->isDead()) {
        if (source->isAlive() && killer->isAlive() && killer->getWeapon()) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, killer->objectName());
            room->obtainCard(source, weapon, reason);
        }
    } else if (source->isDead()) {
        if (killer->isAlive())
            doCollateral(room, killer, victim, prompt);
    } else {
        if (killer->isDead()) {
            ; // do nothing
        } else if (!killer->getWeapon()) {
            doCollateral(room, killer, victim, prompt);
        } else {
            if (!doCollateral(room, killer, victim, prompt)) {
                if (killer->getWeapon()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, killer->objectName());
                    room->obtainCard(source, weapon, reason);
                }
            }
        }
    }
}

Nullification::Nullification(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    target_fixed = true;
    setObjectName("nullification");
}

void Nullification::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    // does nothing, just throw it
    QList<int> table_cardids = room->getCardIdsOnTable(this);
    if (!table_cardids.isEmpty()) {
        DummyCard dummy(table_cardids);
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
        room->moveCardTo(&dummy, NULL, Player::DiscardPile, reason);
    }
}

bool Nullification::isAvailable(const Player *) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return Sanguosha->getCurrentCardUsePattern().contains("nullification");

    return false;
}

HegNullification::HegNullification(Suit suit, int number)
    : Nullification(suit, number)
{
    target_fixed = true;
    setObjectName("heg_nullification");
}

ExNihilo::ExNihilo(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("ex_nihilo");
    target_fixed = true;
}

void ExNihilo::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

bool ExNihilo::isAvailable(const Player *player) const
{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void ExNihilo::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "ex_nihilo");
    effect.to->drawCards(2);
}

Duel::Duel(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("duel");
}

bool Duel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;
    if (to_select == Self)
        return false;

    return true;
}

void Duel::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    QList<const Player *> targets_const;
    QList<const TargetModSkill *> targetModSkills, choose_skill;
    foreach (ServerPlayer *p, use.to)
        targets_const << qobject_cast<const Player *>(p);

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
            const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
            choose_skill << skill;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                    targetModSkills << skill;
                    break;
                }
            }
        }
    }

    if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
            && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
        while (!targetModSkills.isEmpty()) {
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills)
                q << skill->objectName();

            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QList<ServerPlayer *> targets_ts, extra_target;

                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;


                int max = result_skill->getExtraTargetNum(player, use.card);
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                        targets_ts << p;
                extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                            "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                player->tag.remove("extra_target_skill");

                use.to << extra_target;
                targetModSkills.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
    }

    while (!choose_skill.isEmpty()) {
        targetModSkills = choose_skill;
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            bool check = false;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                    check = true;

            if (!check) {
                choose_skill.removeOne(skill);
            } else
                q << skill->objectName();
        }
        if (q.isEmpty()) break;
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
        if (r == "cancel") {
            break;
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QVariant data = QVariant::fromValue(use);       //for AI
            player->tag["extra_target_skill"] = data;
            QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
            player->tag.remove("extra_target_skill");
            use.to << targets;
            choose_skill.removeOne(result_skill);
            targets_const.clear();
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
        }
    }
    room->sortByActionOrder(use.to);
    SingleTargetTrick::onUse(room, use);
}

void Duel::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel");
    room->setEmotion(second, "duel");

    forever{
        if (!first->isAlive())
        break;
        if (second->getMark("WushuangTarget") > 0) {
            const Card *slash = room->askForCard(first,
                "slash",
                "@wushuang-slash-1:" + second->objectName(),
                QVariant::fromValue(effect),
                Card::MethodResponse,
                second);
            if (slash == NULL)
                break;

            slash = room->askForCard(first, "slash",
                "@wushuang-slash-2:" + second->objectName(),
                QVariant::fromValue(effect),
                Card::MethodResponse,
                second);
            if (slash == NULL)
                break;
        } else {
            const Card *slash = room->askForCard(first,
                "slash",
                "duel-slash:" + second->objectName(),
                QVariant::fromValue(effect),
                Card::MethodResponse,
                second);
            if (slash == NULL)
                break;
        }

        qSwap(first, second);
    }

    room->setPlayerMark(first, "WushuangTarget", 0);
    room->setPlayerMark(second, "WushuangTarget", 0);

    DamageStruct damage(this, second->isAlive() ? second : NULL, first);
    if (second != effect.from)
        damage.by_user = false;
    room->damage(damage);
}

Snatch::Snatch(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("snatch");
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    if (to_select->isAllNude())
        return false;

    if (to_select == Self)
        return false;

	bool correct_target = Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, to_select, this);

    int distance = Self->distanceTo(to_select, 0, this);
    if (distance == -1 || (!correct_target && distance > 1))
        return false;

    if (!Self->canGetCard(to_select, "hej"))
        return false;

    return true;
}

void Snatch::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    QList<const Player *> targets_const;
    QList<const TargetModSkill *> targetModSkills, choose_skill;
    foreach (ServerPlayer *p, use.to)
    targets_const << qobject_cast<const Player *>(p);

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
            const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
            choose_skill << skill;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                    targetModSkills << skill;
                    break;
                }
            }
        }
    }

    if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
            && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
        while (!targetModSkills.isEmpty()) {
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills)
                q << skill->objectName();

            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QList<ServerPlayer *> targets_ts, extra_target;

                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;


                int max = result_skill->getExtraTargetNum(player, use.card);
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                        targets_ts << p;
                extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                            "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                player->tag.remove("extra_target_skill");

                use.to << extra_target;
                targetModSkills.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
    }

    while (!choose_skill.isEmpty()) {
        targetModSkills = choose_skill;
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            bool check = false;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                    check = true;
            }
            if (!check) {
                choose_skill.removeOne(skill);
            } else
                q << skill->objectName();
        }
        if (q.isEmpty()) break;
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
        if (r == "cancel") {
            break;
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QVariant data = QVariant::fromValue(use);       //for AI
            player->tag["extra_target_skill"] = data;
            QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
            player->tag.remove("extra_target_skill");
            use.to << targets;
            choose_skill.removeOne(result_skill);
            targets_const.clear();
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
        }
    }
    room->sortByActionOrder(use.to);

    SingleTargetTrick::onUse(room, use);
}

void Snatch::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;
    if (effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "snatch");
    if (!effect.from->canGetCard(effect.to, "hej"))
        return;

    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName(), false, Card::MethodGet);
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
}

void Snatch::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL) return;

    ServerPlayer *player = use.from;
    Room *room = player->getRoom();
    QList<ServerPlayer *> correct_targets;

    foreach (ServerPlayer *p, use.to)
        if (player->distanceTo(p, 0, use.card) > 1)
            correct_targets << p;

    if (!correct_targets.isEmpty()) {
        QList<const TargetModSkill *> tarmods, showed;
        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                foreach (ServerPlayer *p, correct_targets) {
                    if (tarmod->getDistanceLimit(player, p, use.card)) {
                        const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
                        if (player->hasShownSkill(main_skill) || !player->hasSkill(main_skill)) {
                            correct_targets.removeOne(p);
                            if (!showed.contains(tarmod))
                                showed << tarmod;
                        } else if (!tarmods.contains(tarmod))
                            tarmods << tarmod;
                    }
                }
            }
        }

        if (!showed.isEmpty()) {
            foreach (const TargetModSkill *skill, showed) {
                const Skill *main_skill = Sanguosha->getMainSkill(skill->objectName());
                room->notifySkillInvoked(player, main_skill->objectName());
                if (player->hasSkill(main_skill))
                    room->broadcastSkillInvoke(main_skill->objectName(), player->isMale() ? "male" : "female",
                                               skill->getEffectIndex(player, use.card, TargetModSkill::DistanceLimit));
            }
        }

        if (!tarmods.isEmpty()) {
            QList<const TargetModSkill *> tarmods_copy;
            while (!correct_targets.isEmpty() && !tarmods.isEmpty()) {
                QStringList show;
                QList<ServerPlayer *> targets_copy = correct_targets;
                tarmods_copy = tarmods;
                foreach (const TargetModSkill *tarmod, tarmods_copy)
                    foreach (ServerPlayer *p, targets_copy)
                        if (tarmod->getDistanceLimit(player, p, use.card) && !show.contains(tarmod->objectName()))
                            show << tarmod->objectName();

                if (show.isEmpty()) break;
                SPlayerDataMap m;
                m.insert(player, show);
                QString r = room->askForTriggerOrder(player, "declare_skill_invoke", m, false);

                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();

                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                tarmods.removeOne(result_skill);
                foreach (ServerPlayer *p, targets_copy)
                    if (result_skill->getDistanceLimit(player, p, use.card))
                        correct_targets.removeOne(p);

                player->showSkill(main, position);
                room->notifySkillInvoked(player, main);
                room->broadcastSkillInvoke(main, player->isMale() ? "male" : "female",
                                           result_skill->getEffectIndex(player, use.card, TargetModSkill::DistanceLimit), player, position);
            }
        }
    }
}

Dismantlement::Dismantlement(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("dismantlement");
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    if (to_select->isAllNude())
        return false;

    if (to_select == Self)
        return false;

    if (!Self->canDiscard(to_select, "hej"))
        return false;

    return true;
}

void Dismantlement::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    QList<const Player *> targets_const;
    QList<const TargetModSkill *> targetModSkills, choose_skill;
    foreach (ServerPlayer *p, use.to)
        targets_const << qobject_cast<const Player *>(p);

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
            const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
            choose_skill << skill;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                    targetModSkills << skill;
                    break;
                }
            }
        }
    }

    if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
            && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
        while (!targetModSkills.isEmpty()) {
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills)
                q << skill->objectName();

            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QList<ServerPlayer *> targets_ts, extra_target;

                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;


                int max = result_skill->getExtraTargetNum(player, use.card);
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                        targets_ts << p;
                extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                            "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                player->tag.remove("extra_target_skill");

                use.to << extra_target;
                targetModSkills.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
    }

    while (!choose_skill.isEmpty()) {
        targetModSkills = choose_skill;
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            bool check = false;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                    check = true;
            }
            if (!check) {
                choose_skill.removeOne(skill);
            } else
                q << skill->objectName();
        }
        if (q.isEmpty()) break;
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
        if (r == "cancel") {
            break;
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QVariant data = QVariant::fromValue(use);       //for AI
            player->tag["extra_target_skill"] = data;
            QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
            player->tag.remove("extra_target_skill");
            use.to << targets;
            choose_skill.removeOne(result_skill);
            targets_const.clear();
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
        }
    }
    room->sortByActionOrder(use.to);
    SingleTargetTrick::onUse(room, use);
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;

    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "dismantlement");
    if (!effect.from->canDiscard(effect.to, "hej"))
        return;

    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName(), false, Card::MethodDiscard);
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : effect.to, effect.from);
}
/*
QStringList Dismantlement::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}
*/
IronChain::IronChain(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("iron_chain");
    can_recast = true;
}

QString IronChain::getSubtype() const
{
    return "damage_spread";
}

bool IronChain::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    return true;
}

bool IronChain::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast;
    QList<int> sub, hand_cards;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (const Card *card, Self->getHandcards())
        hand_cards << card->getEffectiveId();
    foreach (int id, sub) {
        if (!hand_cards.contains(id)) {
            rec = false;
            break;
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() > total_num)
        return false;
    return rec || targets.length() > 0;
}

void IronChain::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, use.from->objectName());
        reason.m_skillName = getSkillName();
        room->moveCardTo(this, NULL, Player::PlaceTable, reason, true);
        use.from->broadcastSkillInvoke("@recast");

        if (!card_use.card->getSkillName().isNull() && card_use.card->getSkillName(true) == card_use.card->getSkillName(false)
            && card_use.m_isOwnerUse && card_use.from->hasSkill(card_use.card->getSkillName()))
            room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = use.from;
        log.card_str = use.card->toString();
        room->sendLog(log);

        QString skill_name = use.card->showSkill();
        if (!skill_name.isNull() && use.from->ownSkill(skill_name) && !use.from->hasShownSkill(skill_name))
            use.from->showGeneral(use.from->inHeadSkills(skill_name));

        QList<int> table_cardids = room->getCardIdsOnTable(this);
        if (!table_cardids.isEmpty()) {
            DummyCard dummy(table_cardids);
            room->moveCardTo(&dummy, use.from, NULL, Player::DiscardPile, reason, true);
        }

        use.from->drawCards(1);
    } else {
        ServerPlayer *player = use.from;
        QList<const Player *> targets_const;
        QList<const TargetModSkill *> targetModSkills, choose_skill;
        foreach (ServerPlayer *p, use.to)
            targets_const << qobject_cast<const Player *>(p);

        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                choose_skill << skill;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                        targetModSkills << skill;
                        break;
                    }
                }
            }
        }

        if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
                && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
            while (!targetModSkills.isEmpty()) {
                QStringList q;
                foreach (const TargetModSkill *skill, targetModSkills)
                    q << skill->objectName();

                SPlayerDataMap m;
                m.insert(player, q);
                QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
                if (r == "cancel") {
                    break;
                } else {
                    QString skill_name = r.split(":").last();
                    QString position = r.split(":").first().split("?").last();
                    if (position == player->objectName()) position = QString();
                    const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                    QList<ServerPlayer *> targets_ts, extra_target;

                    QVariant data = QVariant::fromValue(use);       //for AI
                    player->tag["extra_target_skill"] = data;


                    int max = result_skill->getExtraTargetNum(player, use.card);
                    QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                    foreach (ServerPlayer *p, room->getAlivePlayers())
                        if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                            targets_ts << p;
                    extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                                "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                    player->tag.remove("extra_target_skill");

                    use.to << extra_target;
                    targetModSkills.removeOne(result_skill);
                    targets_const.clear();
                    foreach (ServerPlayer *p, use.to)
                        targets_const << qobject_cast<const Player *>(p);
                }
            }
        }

        while (!choose_skill.isEmpty()) {
            targetModSkills = choose_skill;
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills) {
                bool check = false;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                        check = true;
                }
                if (!check) {
                    choose_skill.removeOne(skill);
                } else
                    q << skill->objectName();
            }
            if (q.isEmpty()) break;
            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;
                QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
                player->tag.remove("extra_target_skill");
                use.to << targets;
                choose_skill.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
        room->sortByActionOrder(use.to);
        TrickCard::onUse(room, use);
    }
}

void IronChain::onEffect(const CardEffectStruct &effect) const
{
    if (!effect.to->canBeChainedBy(effect.from))
        return;
    effect.to->setChained(!effect.to->isChained());

    Room *room = effect.to->getRoom();

    room->broadcastProperty(effect.to, "chained");
    room->setEmotion(effect.to, "chain");
    room->getThread()->trigger(ChainStateChanged, room, effect.to);
}
/*
QStringList IronChain::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 3) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 2;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}
*/
AwaitExhausted::AwaitExhausted(Card::Suit suit, int number) : TrickCard(suit, number)
{
    setObjectName("await_exhausted");
    target_fixed = true;
}

QString AwaitExhausted::getSubtype() const
{
    return "await_exhausted";
}

bool AwaitExhausted::isAvailable(const Player *player) const
{
    bool canUse = false;
    if (!player->isProhibited(player, this))
        canUse = true;
    if (!canUse) {
        QList<const Player *> players = player->getAliveSiblings();
        foreach (const Player *p, players) {
            if (player->isProhibited(p, this))
                continue;
            if (player->isFriendWith(p)) {
                canUse = true;
                break;
            }
        }
    }

    return canUse && TrickCard::isAvailable(player);
}

void AwaitExhausted::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct new_use = card_use;
    if (!card_use.from->isProhibited(card_use.from, this))
        new_use.to << new_use.from;
    foreach (ServerPlayer *p, room->getOtherPlayers(new_use.from)) {
        if (p->isFriendWith(new_use.from)) {
            const ProhibitSkill *skill = room->isProhibited(card_use.from, p, this);
            if (skill) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = p;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName(), p);
            } else {
                new_use.to << p;
            }
        }
    }

    TrickCard::onUse(room, new_use);
}

void AwaitExhausted::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));

        QVariantList players;
        for (int i = targets.indexOf(target); i < targets.length(); i++) {
            if (!nullified_list.contains(targets.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(targets.at(i)));
        }
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }

    room->removeTag("targets" + this->toString());

    foreach (ServerPlayer *target, targets) {
        if (target->hasFlag("AwaitExhaustedEffected")) {
            room->setPlayerFlag(target, "-AwaitExhaustedEffected");
            room->askForDiscard(target, objectName(), 2, 2, false, true);
        }
    }

    QList<int> table_cardids = room->getCardIdsOnTable(this);
    if (!table_cardids.isEmpty()) {
        DummyCard dummy(table_cardids);
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        room->moveCardTo(&dummy, source, NULL, Player::DiscardPile, reason, true);
    }
}

void AwaitExhausted::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(2);
    effect.to->getRoom()->setPlayerFlag(effect.to, "AwaitExhaustedEffected");
}

KnownBoth::KnownBoth(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number)
{
    setObjectName("known_both");
    can_recast = true;
}

bool KnownBoth::isAvailable(const Player *player) const
{
    bool can_use = false;
    foreach (const Player *p, player->getSiblings()) {
        if (player->isProhibited(p, this))
            continue;
        if (p->isKongcheng() && p->hasShownAllGenerals())
            continue;
        can_use = true;
        break;
    }
    bool can_rec = can_recast;
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    if (sub.isEmpty() || sub.contains(-1))
        can_rec = false;
    return (can_use && !player->isCardLimited(this, Card::MethodUse))
        || (can_rec && !player->isCardLimited(this, Card::MethodRecast));
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng() || !to_select->hasShownAllGenerals();
}

bool KnownBoth::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast
        && !Self->isCardLimited(this, Card::MethodRecast);
    QList<int> sub, hand_cards;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (const Card *card, Self->getHandcards())
        hand_cards << card->getEffectiveId();
    foreach (int id, sub) {
        if (!hand_cards.contains(id)) {
            rec = false;
            break;
        }
    }

    if (Self->isCardLimited(this, Card::MethodUse))
        return rec && targets.length() == 0;
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() > total_num)
        return false;
    return targets.length() > 0 || rec;
}

void KnownBoth::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, use.from->objectName());
        reason.m_skillName = getSkillName();
        room->moveCardTo(this, NULL, Player::PlaceTable, reason);
        use.from->broadcastSkillInvoke("@recast");
        if (!card_use.card->getSkillName().isNull() && card_use.card->getSkillName(true) == card_use.card->getSkillName(false)
            && card_use.m_isOwnerUse && card_use.from->hasSkill(card_use.card->getSkillName()))
            room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = use.from;
        log.card_str = use.card->toString();
        room->sendLog(log);

        QString skill_name = use.card->showSkill();
        if (!skill_name.isNull() && use.from->ownSkill(skill_name) && !use.from->hasShownSkill(skill_name))
            use.from->showGeneral(use.from->inHeadSkills(skill_name));

        QList<int> table_cardids = room->getCardIdsOnTable(this);
        if (!table_cardids.isEmpty()) {
            DummyCard dummy(table_cardids);
            room->moveCardTo(&dummy, use.from, NULL, Player::DiscardPile, reason, true);
        }

        use.from->drawCards(1);
    } else {
        ServerPlayer *player = use.from;
        QList<const Player *> targets_const;
        QList<const TargetModSkill *> targetModSkills, choose_skill;
        foreach (ServerPlayer *p, use.to)
            targets_const << qobject_cast<const Player *>(p);

        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                choose_skill << skill;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                        targetModSkills << skill;
                        break;
                    }
                }
            }
        }

        if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
                && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
            while (!targetModSkills.isEmpty()) {
                QStringList q;
                foreach (const TargetModSkill *skill, targetModSkills)
                    q << skill->objectName();

                SPlayerDataMap m;
                m.insert(player, q);
                QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
                if (r == "cancel") {
                    break;
                } else {
                    QString skill_name = r.split(":").last();
                    QString position = r.split(":").first().split("?").last();
                    if (position == player->objectName()) position = QString();
                    const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                    QList<ServerPlayer *> targets_ts, extra_target;

                    QVariant data = QVariant::fromValue(use);       //for AI
                    player->tag["extra_target_skill"] = data;


                    int max = result_skill->getExtraTargetNum(player, use.card);
                    QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                    foreach (ServerPlayer *p, room->getAlivePlayers())
                        if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                            targets_ts << p;
                    extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                                "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                    player->tag.remove("extra_target_skill");

                    use.to << extra_target;
                    targetModSkills.removeOne(result_skill);
                    targets_const.clear();
                    foreach (ServerPlayer *p, use.to)
                        targets_const << qobject_cast<const Player *>(p);
                }
            }
        }

        while (!choose_skill.isEmpty()) {
            targetModSkills = choose_skill;
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills) {
                bool check = false;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                        check = true;
                }
                if (!check) {
                    choose_skill.removeOne(skill);
                } else
                    q << skill->objectName();
            }
            if (q.isEmpty()) break;
            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;
                QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
                player->tag.remove("extra_target_skill");
                use.to << targets;
                choose_skill.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
        room->sortByActionOrder(use.to);
        SingleTargetTrick::onUse(room, use);
    }
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const
{
    QStringList choices;
    if (!effect.to->isKongcheng())
        choices << "handcards";
    if (!effect.to->hasShownGeneral1())
        choices << "head_general";
    if (effect.to->getGeneral2() && !effect.to->hasShownGeneral2())
        choices << "deputy_general";

    Room *room = effect.from->getRoom();

    effect.to->setFlags("KnownBothTarget");// For AI
    QString choice = room->askForChoice(effect.from, objectName(),
        choices.join("+"), QVariant::fromValue(effect.to));
    effect.to->setFlags("-KnownBothTarget");

    if (choice == "handcards")
        room->showAllCards(effect.to, effect.from);
    else {
        QStringList list = room->getTag(effect.to->objectName()).toStringList();
        list.removeAt(choice == "head_general" ? 1 : 0);
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = name;
            log.arg2 = choice;
            room->doNotify(effect.from->getClient(), QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
        }
        JsonArray arg;
        arg << objectName();
        arg << JsonUtils::toJsonArray(list);
        room->doNotify(effect.from->getClient(), QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
    }
}
/*
QStringList KnownBoth::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}
*/
BefriendAttacking::BefriendAttacking(Card::Suit suit, int number) : SingleTargetTrick(suit, number)
{
    setObjectName("befriend_attacking");
}

bool BefriendAttacking::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    return to_select->hasShownOneGeneral() && !Self->isFriendWith(to_select);
}

void BefriendAttacking::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    QList<const Player *> targets_const;
    QList<const TargetModSkill *> targetModSkills, choose_skill;
    foreach (ServerPlayer *p, use.to)
        targets_const << qobject_cast<const Player *>(p);

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
            const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
            choose_skill << skill;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                    targetModSkills << skill;
                    break;
                }
            }
        }
    }

    if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
            && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
        while (!targetModSkills.isEmpty()) {
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills)
                q << skill->objectName();

            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QList<ServerPlayer *> targets_ts, extra_target;

                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;


                int max = result_skill->getExtraTargetNum(player, use.card);
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                        targets_ts << p;
                extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                            "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                player->tag.remove("extra_target_skill");

                use.to << extra_target;
                targetModSkills.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
    }

    while (!choose_skill.isEmpty()) {
        targetModSkills = choose_skill;
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            bool check = false;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                    check = true;
            }
            if (!check) {
                choose_skill.removeOne(skill);
            } else
                q << skill->objectName();
        }
        if (q.isEmpty()) break;
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
        if (r == "cancel") {
            break;
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QVariant data = QVariant::fromValue(use);       //for AI
            player->tag["extra_target_skill"] = data;
            QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
            player->tag.remove("extra_target_skill");
            use.to << targets;
            choose_skill.removeOne(result_skill);
            targets_const.clear();
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
        }
    }
    room->sortByActionOrder(use.to);
    SingleTargetTrick::onUse(room, use);
}

void BefriendAttacking::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(1);
    effect.from->drawCards(3);
}

bool BefriendAttacking::isAvailable(const Player *player) const
{
    return player->hasShownOneGeneral() && TrickCard::isAvailable(player);
}
/*
QStringList BefriendAttacking::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}
*/
FireAttack::FireAttack(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("fire_attack");
}

bool FireAttack::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    if (to_select->isKongcheng())
        return false;

    if (to_select == Self)
        return !Self->isLastHandCard(this, true);
    else
        return true;
}

void FireAttack::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    QList<const Player *> targets_const;
    QList<const TargetModSkill *> targetModSkills, choose_skill;
    foreach (ServerPlayer *p, use.to)
        targets_const << qobject_cast<const Player *>(p);

    foreach (QString name, Sanguosha->getSkillNames()) {
        if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
            const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
            choose_skill << skill;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->getExtraTargetNum(player, use.card) > 0 && use.card->targetFilter(targets_const, p, player)) {
                    targetModSkills << skill;
                    break;
                }
            }
        }
    }

    if (card_use.m_reason != CardUseStruct::CARD_USE_REASON_PLAY
            && card_use.m_reason != CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {              //the first step for choosing skill not contains "extra/more" in description
        while (!targetModSkills.isEmpty()) {
            QStringList q;
            foreach (const TargetModSkill *skill, targetModSkills)
                q << skill->objectName();

            SPlayerDataMap m;
            m.insert(player, q);
            QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
            if (r == "cancel") {
                break;
            } else {
                QString skill_name = r.split(":").last();
                QString position = r.split(":").first().split("?").last();
                if (position == player->objectName()) position = QString();
                const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
                QList<ServerPlayer *> targets_ts, extra_target;

                QVariant data = QVariant::fromValue(use);       //for AI
                player->tag["extra_target_skill"] = data;


                int max = result_skill->getExtraTargetNum(player, use.card);
                QString main = Sanguosha->getMainSkill(skill_name)->objectName();
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, player))
                        targets_ts << p;
                extra_target = room->askForPlayersChosen(player, targets_ts, main, 0, max,
                            "@extra_targets:" + result_skill->objectName() + ":" + use.card->getLogName() + ":" + QString::number(max), false, position);

                player->tag.remove("extra_target_skill");

                use.to << extra_target;
                targetModSkills.removeOne(result_skill);
                targets_const.clear();
                foreach (ServerPlayer *p, use.to)
                    targets_const << qobject_cast<const Player *>(p);
            }
        }
    }

    while (!choose_skill.isEmpty()) {
        targetModSkills = choose_skill;
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            bool check = false;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && skill->checkExtraTargets(player, p, use.card, targets_const) && use.card->extratargetFilter(targets_const, p, player))
                    check = true;
            }
            if (!check) {
                choose_skill.removeOne(skill);
            } else
                q << skill->objectName();
        }
        if (q.isEmpty()) break;
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "extra_target_skill", m, true);
        if (r == "cancel") {
            break;
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QVariant data = QVariant::fromValue(use);       //for AI
            player->tag["extra_target_skill"] = data;
            QList<ServerPlayer *> targets = room->askForExtraTargets(player, use.to, use.card, skill_name, "@extra_targets1:" + use.card->getLogName(), true, position);
            player->tag.remove("extra_target_skill");
            use.to << targets;
            choose_skill.removeOne(result_skill);
            targets_const.clear();
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
        }
    }
    room->sortByActionOrder(use.to);
    SingleTargetTrick::onUse(room, use);
}

void FireAttack::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.to->isKongcheng())
        return;

    const Card *card = room->askForCardShow(effect.to, effect.from, objectName());
    room->setEmotion(effect.from, "fire_attack");
    room->showCard(effect.to, card->getEffectiveId());

    QString suit_str = card->getSuitString();
    QString pattern = QString(".%1").arg(suit_str.at(0).toUpper());
    QString prompt = QString("@fire-attack:%1::%2").arg(effect.to->objectName()).arg(suit_str);
    if (effect.from->isAlive()) {
        const Card *card_to_throw = room->askForCard(effect.from, pattern, prompt);
        if (card_to_throw)
            room->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
        else
            effect.from->setFlags("FireAttackFailed_" + effect.to->objectName()); // For AI
    }

    if (card->isVirtualCard())
        delete card;
}
/*
QStringList FireAttack::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}
*/
Indulgence::Indulgence(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("indulgence");

    judge.pattern = ".|heart";
    judge.good = true;
    judge.reason = objectName();
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->containsTrick(objectName()) && to_select != Self;
}

void Indulgence::takeEffect(ServerPlayer *target) const
{
    target->clearHistory();
#ifndef QT_NO_DEBUG
    if (!target->getAI() && target->askForSkillInvoke("userdefine:cancelIndulgence")) return;
#endif
    target->getRoom()->setEmotion(target, "indulgence");
    target->skip(Player::Play);
}

SupplyShortage::SupplyShortage(Card::Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("supply_shortage");

    judge.pattern = ".|club";
    judge.good = true;
    judge.reason = objectName();
}

bool SupplyShortage::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select->containsTrick(objectName()) || to_select == Self)
        return false;

	bool correct_target = Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, to_select, this);

    int distance = Self->distanceTo(to_select, 0, this);
    if (distance == -1 || (!correct_target && distance > 1))
        return false;

    return true;
}

void SupplyShortage::takeEffect(ServerPlayer *target) const
{
#ifndef QT_NO_DEBUG
    if (!target->getAI() && target->askForSkillInvoke("userdefine:cancelSupplyShortage")) return;
#endif
    target->getRoom()->setEmotion(target, "supplyshortage");
    target->skip(Player::Draw);
}

void SupplyShortage::checkTargetModSkillShow(const CardUseStruct &use) const     //forced to use this function instead of onUse cause delaytrick will wrapped later
{                                                                                       //and turn back after show general in onUse
    if (use.card == NULL) return;

    ServerPlayer *player = use.from;
    Room *room = player->getRoom();

    if (player->distanceTo(use.to.first(), 0, use.card) > 1) {
        QStringList tarmods;
        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                if (tarmod->getDistanceLimit(player, use.to.first(), use.card)) {
                    const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
                    if (player->hasShownSkill(main_skill) || !player->hasSkill(main_skill)) {
                        room->notifySkillInvoked(player, main_skill->objectName());
                        if (player->hasSkill(main_skill))
                            room->broadcastSkillInvoke(main_skill->objectName(), player->isMale() ? "male" : "female",
                                                       tarmod->getEffectIndex(player, use.card, TargetModSkill::DistanceLimit));
                        return;
                    } else if (!tarmods.contains(tarmod->objectName()))
                        tarmods << tarmod->objectName();
                }
            }
        }
        if (!tarmods.isEmpty()) {
            SPlayerDataMap m;
            m.insert(player, tarmods);
            QString r = room->askForTriggerOrder(player, "declare_skill_invoke", m, false);
            QString skill_name = r.split(":").last();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();
            QString main = Sanguosha->getMainSkill(skill_name)->objectName();
            player->showSkill(main, position);
            room->notifySkillInvoked(player, main);
            room->broadcastSkillInvoke(main, player->isMale() ? "male" : "female",
                                       result_skill->getEffectIndex(player, use.card, TargetModSkill::DistanceLimit), player, position);
        }
    }
}

Disaster::Disaster(Card::Suit suit, int number)
    : DelayedTrick(suit, number, true)
{
    target_fixed = true;
}

void Disaster::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    DelayedTrick::onUse(room, use);
}

bool Disaster::isAvailable(const Player *player) const
{
    if (player->containsTrick(objectName()))
        return false;

    return !player->isProhibited(player, this) && DelayedTrick::isAvailable(player);
}

Lightning::Lightning(Suit suit, int number) :Disaster(suit, number)
{
    setObjectName("lightning");

    judge.pattern = ".|spade|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Lightning::takeEffect(ServerPlayer *target) const
{
#ifndef QT_NO_DEBUG
    if (!target->getAI() && target->askForSkillInvoke("userdefine:cancelLightning")) return;
#endif
    target->getRoom()->setEmotion(target, "lightning");
    target->getRoom()->damage(DamageStruct(this, NULL, target, 3, DamageStruct::Thunder));
}

QList<Card *> StandardCardPackage::trickCards()
{
    QList<Card *> cards;

    cards
        << new AmazingGrace
        << new GodSalvation
        << new SavageAssault(Card::Spade, 13)
        << new SavageAssault(Card::Club, 7)
        << new ArcheryAttack
        << new Duel(Card::Spade, 1)
        << new Duel(Card::Club, 1)
        << new ExNihilo(Card::Heart, 7)
        << new ExNihilo(Card::Heart, 8)
        << new Snatch(Card::Spade, 3)
        << new Snatch(Card::Spade, 4)
        << new Snatch(Card::Diamond, 3)
        << new Dismantlement(Card::Spade, 3)
        << new Dismantlement(Card::Spade, 4)
        << new Dismantlement(Card::Heart, 12)
        << new IronChain(Card::Spade, 12)
        << new IronChain(Card::Club, 12)
        << new IronChain(Card::Club, 13)
        << new FireAttack(Card::Heart, 2)
        << new FireAttack(Card::Heart, 3)
        << new Collateral
        << new Nullification
        << new HegNullification(Card::Club, 13)
        << new HegNullification(Card::Diamond, 12)
        << new AwaitExhausted(Card::Heart, 11)
        << new AwaitExhausted(Card::Diamond, 4)
        << new KnownBoth(Card::Club, 3)
        << new KnownBoth(Card::Club, 4)
        << new BefriendAttacking
        << new Indulgence(Card::Club, 6)
        << new Indulgence(Card::Heart, 6)
        << new SupplyShortage(Card::Spade, 10)
        << new SupplyShortage(Card::Club, 10)
        << new Lightning;

    return cards;
}
