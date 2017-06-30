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

#include "standard-basics.h"
#include "standard-package.h"
#include "engine.h"

Slash::Slash(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
    drank = 0;
    has_preact = true;
    extra_target = true;
    distance_limit = true;
}

DamageStruct::Nature Slash::getNature() const
{
    return nature;
}

void Slash::setNature(DamageStruct::Nature nature)
{
    this->nature = nature;
}

bool Slash::IsAvailable(const Player *player, const Card *slash, bool considerSpecificAssignee)
{
    Slash *newslash = new Slash(Card::NoSuit, 0);
    newslash->setFlags("Global_SlashAvailabilityChecker");
    newslash->deleteLater();
#define THIS_SLASH (slash == NULL ? newslash : slash)
    if (player->isCardLimited(THIS_SLASH, Card::MethodUse))
        return false;

    CardUseStruct::CardUseReason reason = Sanguosha->getCurrentCardUseReason();
    if (reason == CardUseStruct::CARD_USE_REASON_PLAY || (reason == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && player->getPhase() == Player::Play)) {
        QList<int> ids;
        if (slash) {
            if (slash->isVirtualCard()) {
                if (slash->subcardsLength() > 0)
                    ids = slash->getSubcards();
            } else {
                ids << slash->getEffectiveId();
            }
        }
        bool has_weapon = player->hasWeapon("Crossbow") && ids.contains(player->getWeapon()->getEffectiveId());
        if ((!has_weapon && player->hasWeapon("Crossbow")) || player->canSlashWithoutCrossbow(THIS_SLASH))
            return true;

        if (considerSpecificAssignee) {
            foreach (const Player *p, player->getAliveSiblings())
                if (Sanguosha->correctCardTarget(TargetModSkill::SpecificAssignee, player, p, THIS_SLASH) && player->canSlash(p, THIS_SLASH))
                    return true;

        }
        return false;
    } else
        return true;

#undef THIS_SLASH
}

bool Slash::IsSpecificAssignee(const Player *player, const Player *from, const Card *slash)
{
    if (from->hasFlag("slashTargetFix") && player->hasFlag("SlashAssignee"))
        return true;
    else if (from->getPhase() == Player::Play && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
        && !Slash::IsAvailable(from, slash, false)) {
        if (Sanguosha->correctCardTarget(TargetModSkill::SpecificAssignee, from, player, slash))
            return true;
    }
    return false;
}

bool Slash::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

QString Slash::getSubtype() const
{
    return "attack_card";
}

void Slash::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;

    if (player->hasFlag("slashTargetFix")) {
        room->setPlayerFlag(player, "-slashTargetFix");
        room->setPlayerFlag(player, "-slashTargetFixToOne");
        foreach (ServerPlayer *target, room->getAlivePlayers())
            if (target->hasFlag("SlashAssignee"))
                room->setPlayerFlag(target, "-SlashAssignee");
    }

    /* actually it's not proper to put the codes here.
       considering the nasty design of the client and the convenience as well,
       I just move them here */
    if (!Sanguosha->getModifiedSkills().contains("Fan") && objectName() == "slash" && player->hasWeapon("Fan") && use.m_isOwnerUse
            && (!isVirtualCard() || subcards.isEmpty())) {
        QVariant data = QVariant::fromValue(use);
        FireSlash *fire_slash = new FireSlash(getSuit(), getNumber());
        fire_slash->addSubcard(this);
        fire_slash->setSkillName("Fan");
        bool can_use = true;
        foreach (ServerPlayer *p, use.to) {
            if (!player->canSlash(p, fire_slash)) {
                can_use = false;
                break;
            }
        }
        if (can_use && room->askForSkillInvoke(player, "Fan", data))
            use.card = fire_slash;
        else
            delete fire_slash;
    }

    if (use.card->isVirtualCard() && player->hasWeapon(use.card->getSkillName())) {
        room->setEmotion(player, "weapon/" + use.card->getSkillName());
        use.card->setFlags(use.card->getSkillName());
    }

    if (use.card->isKindOf("ThunderSlash"))
        room->setEmotion(player, "thunder_slash");
    else if (use.card->isKindOf("FireSlash"))
        room->setEmotion(player, "fire_slash");
    else if (use.card->isRed())
        room->setEmotion(player, "slash_red");
    else if (use.card->isBlack())
        room->setEmotion(player, "slash_black");
    else
        room->setEmotion(player, "killer");

    BasicCard::onUse(room, use);
}

void Slash::onEffect(const CardEffectStruct &card_effect) const
{
    Room *room = card_effect.from->getRoom();
    if (card_effect.from->getMark("drank") > 0) {
        room->setCardFlag(this, "drank");
        this->drank = card_effect.from->getMark("drank");
        room->setPlayerMark(card_effect.from, "drank", 0);
    }

    SlashEffectStruct effect;
    effect.from = card_effect.from;
    effect.nature = nature;
    effect.slash = this;

    effect.to = card_effect.to;
    effect.drank = this->drank;
    effect.nullified = card_effect.nullified;

    QVariantList jink_list = effect.from->tag["Jink_" + toString()].toList();
    effect.jink_num = jink_list.takeFirst().toInt();
    if (jink_list.isEmpty())
        effect.from->tag.remove("Jink_" + toString());
    else
        effect.from->tag["Jink_" + toString()] = QVariant::fromValue(jink_list);

    room->slashEffect(effect);
}

void Slash::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (!use.card) return;

    ServerPlayer *player = use.from;
    Room *room = player->getRoom();
    QList<const TargetModSkill *> targetModSkills;
    // for Paoxiao, Jili, Crossbow & etc
    if (player->getPhase() == Player::Play && player->hasFlag("Global_MoreSlashInOneTurn") && player->getSlashCount() > 1) {
        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                if (player->getSlashCount() <= Sanguosha->correctCardTarget(TargetModSkill::Residue, player, use.card) + 1
                        && player->usedTimes(use.card->getClassName() + "_" + name) < skill->getResidueNum(player, use.card))
                    targetModSkills << skill;

                foreach (ServerPlayer *p, use.to) {
                    if (skill->checkSpecificAssignee(player, p, use.card)) {
                        targetModSkills << skill;
                        break;
                    }
                }
            }
        }
        bool canSelectCrossbow = player->hasWeapon("Crossbow") && !use.card->getSubcards().contains(player->getWeapon()->getEffectiveId());
        QStringList q;
        foreach (const TargetModSkill *skill, targetModSkills) {
            if (skill->getResidueNum(player, use.card) > 500 && player->hasShownSkill(skill->objectName())) {
                q << skill->objectName();
                canSelectCrossbow = false;
            }
        }
        if (canSelectCrossbow) q << "Crossbow";
        if (q.isEmpty()) {
            foreach (const TargetModSkill *skill, targetModSkills)
                if (!q.contains(skill->objectName()))
                    q << skill->objectName();
        } else {
            foreach (const TargetModSkill *skill, targetModSkills)
                if (skill->getResidueNum(player, use.card) > 500 && !player->hasShownSkill(skill->objectName()))
                    q << skill->objectName();
        }

        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "declare_skill_invoke", m, false);

        if (r.endsWith("Crossbow")) {
            room->setEmotion(player, "weapon/crossbow");
        } else {
            QString skill_name = r.split(":").last();
            QString position = r.split(":").first().split("?").last();
            if (position == player->objectName()) position = QString();

            QString main = Sanguosha->getMainSkill(skill_name)->objectName();
            const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));
            room->addPlayerHistory(player, use.card->getClassName() + "_" + skill_name);

            player->showSkill(main, position);
            room->broadcastSkillInvoke(main, player->isMale() ? "male" : "female", result_skill->getEffectIndex(player, use.card, TargetModSkill::Residue), player, position);
            room->notifySkillInvoked(player, main);
        }
    }

    QList<ServerPlayer *> correct_targets;
    foreach (ServerPlayer *p, use.to)
        if (!distance_limit && !player->inMyAttackRange(p, this))
            correct_targets << p;

    if (!correct_targets.isEmpty()) {
        QList<const TargetModSkill *> showed;
        targetModSkills.clear();
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
                        } else if (!targetModSkills.contains(tarmod))
                            targetModSkills << tarmod;
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

        if (!targetModSkills.isEmpty()) {
            QList<const TargetModSkill *> tarmods_copy;
            while (!correct_targets.isEmpty() && !targetModSkills.isEmpty()) {
                QStringList show;
                tarmods_copy = targetModSkills;
                QList<ServerPlayer *> targets_copy = correct_targets;
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
                targetModSkills.removeOne(result_skill);
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

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int slash_targets = 1 + (extra_target ? Sanguosha->correctCardTarget(TargetModSkill::ExtraMaxTarget, Self, this) : 0);

    bool has_specific_assignee = false;
    foreach (const Player *p, Self->getAliveSiblings()) {
        if (Slash::IsSpecificAssignee(p, Self, this)) {
            has_specific_assignee = true;
            break;
        }
    }

    if (has_specific_assignee) {
        if (targets.isEmpty())
            return Slash::IsSpecificAssignee(to_select, Self, this) && Self->canSlash(to_select, this);
        else {
            if (!extra_target) return false;
            bool canSelect = false;
            foreach (const Player *p, targets) {
                if (Slash::IsSpecificAssignee(p, Self, this)) {
                    canSelect = true;
                    break;
                }
            }
            if (!canSelect) return false;
        }
    }

    if (!Self->canSlash(to_select, this, 0, targets) || targets.length() >= slash_targets) return false;

    return true;
}

bool Slash::extratargetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return extra_target && Self->canSlash(to_select, this, 0, targets);
}

NatureSlash::NatureSlash(Suit suit, int number, DamageStruct::Nature nature)
    : Slash(suit, number)
{
    this->nature = nature;
}

bool NatureSlash::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("slash"))
        return true;
    else
        return Slash::match(pattern);
}

ThunderSlash::ThunderSlash(Suit suit, int number, bool is_transferable)
    : NatureSlash(suit, number, DamageStruct::Thunder)
{
    setObjectName("thunder_slash");
    transferable = is_transferable;
}

FireSlash::FireSlash(Suit suit, int number)
    : NatureSlash(suit, number, DamageStruct::Fire)
{
    setObjectName("fire_slash");
}

Jink::Jink(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("jink");
    target_fixed = true;
}

QString Jink::getSubtype() const
{
    return "defense_card";
}

bool Jink::isAvailable(const Player *player) const
{
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return Sanguosha->getCurrentCardUsePattern() == "jink" && !player->isProhibited(player, this);

    return false;
}

Peach::Peach(Suit suit, int number, bool is_transferable) : BasicCard(suit, number)
{
    setObjectName("peach");
    target_fixed = true;
    transferable = is_transferable;
}

QString Peach::getSubtype() const
{
    return "recover_card";
}

void Peach::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    BasicCard::onUse(room, use);
}

void Peach::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "peach");

    // recover hp
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    room->recover(effect.to, recover);
}

bool Peach::isAvailable(const Player *player) const
{
    if (!BasicCard::isAvailable(player)) return false;

    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && player->hasFlag("askForSinglePeach")) {
        Player *dying = Sanguosha->getCurrentAskforPeachPlayer();
        return !player->isProhibited(dying, this);
    }

    return !player->isProhibited(player, this) && player->isWounded() && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE;
}

Analeptic::Analeptic(Card::Suit suit, int number, bool is_transferable)
    : BasicCard(suit, number)
{
    setObjectName("analeptic");
    target_fixed = true;
    transferable = is_transferable;
}

QString Analeptic::getSubtype() const
{
    return "buff_card";
}

bool Analeptic::IsAvailable(const Player *player, const Card *analeptic)
{
    Analeptic *newanal = new Analeptic(Card::NoSuit, 0);
    newanal->deleteLater();
#define THIS_ANAL (analeptic == NULL ? newanal : analeptic)
    if (player->isCardLimited(THIS_ANAL, Card::MethodUse) || player->isProhibited(player, THIS_ANAL))
        return false;

    if (player->hasFlag("Global_Dying")) return true;

    if (player->usedTimes("Analeptic") <= Sanguosha->correctCardTarget(TargetModSkill::Residue, player, THIS_ANAL))
        return true;

    if (Sanguosha->correctCardTarget(TargetModSkill::SpecificAssignee, player, player, THIS_ANAL))
        return true;

    return false;
#undef THIS_ANAL
}

bool Analeptic::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

void Analeptic::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    BasicCard::onUse(room, use);
}

void Analeptic::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "analeptic");

    if (effect.to->hasFlag("Global_Dying") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        // recover hp
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    } else {
        room->addPlayerMark(effect.to, "drank");
    }
}

void Analeptic::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return;

    ServerPlayer *player = use.from;
    Room *room = player->getRoom();
    if (player->getPhase() != Player::NotActive && player->usedTimes(use.card->getClassName()) > 1) {
        QStringList q;
        foreach (QString name, Sanguosha->getSkillNames()) {
            if (Sanguosha->getSkill(name)->inherits("TargetModSkill")) {
                const TargetModSkill *skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(name));
                if (player->usedTimes("Analeptic") <= Sanguosha->correctCardTarget(TargetModSkill::Residue, player, use.card) + 1
                        && player->usedTimes(use.card->getClassName() + "_" + name) < skill->getResidueNum(player, this))
                    q << skill->objectName();

                if (skill->checkSpecificAssignee(player, player, use.card))
                    q << skill->objectName();
            }
        }
        SPlayerDataMap m;
        m.insert(player, q);
        QString r = room->askForTriggerOrder(player, "declare_skill_invoke", m, false);

        QString skill_name = r.split(":").last();
        QString position = r.split(":").first().split("?").last();
        if (position == player->objectName()) position = QString();

        QString main = Sanguosha->getMainSkill(skill_name)->objectName();
        const TargetModSkill *result_skill = qobject_cast<const TargetModSkill *>(Sanguosha->getSkill(skill_name));

        room->addPlayerHistory(player, use.card->getClassName() + "_" + skill_name);
        player->showSkill(main, position);
        room->broadcastSkillInvoke(main, player->isMale() ? "male" : "female", result_skill->getEffectIndex(player, use.card, TargetModSkill::Residue), player, position);
        room->notifySkillInvoked(player, main);
    }
}

QList<Card *> StandardCardPackage::basicCards()
{
    QList<Card *> cards;


    cards
        << new Slash(Card::Spade, 5)
        << new Slash(Card::Spade, 7)
        << new Slash(Card::Spade, 8)
        << new Slash(Card::Spade, 8)
        << new Slash(Card::Spade, 9)
        << new Slash(Card::Spade, 10)
        << new Slash(Card::Spade, 11)

        << new Slash(Card::Club, 2)
        << new Slash(Card::Club, 3)
        << new Slash(Card::Club, 4)
        << new Slash(Card::Club, 5)
        << new Slash(Card::Club, 8)
        << new Slash(Card::Club, 9)
        << new Slash(Card::Club, 10)
        << new Slash(Card::Club, 11)
        << new Slash(Card::Club, 11)

        << new Slash(Card::Heart, 10)
        << new Slash(Card::Heart, 12)

        << new Slash(Card::Diamond, 10)
        << new Slash(Card::Diamond, 11)
        << new Slash(Card::Diamond, 12)

        << new FireSlash(Card::Heart, 4)

        << new FireSlash(Card::Diamond, 4)
        << new FireSlash(Card::Diamond, 5)

        << new ThunderSlash(Card::Spade, 6)
        << new ThunderSlash(Card::Spade, 7)

        << new ThunderSlash(Card::Club, 6)
        << new ThunderSlash(Card::Club, 7)
        << new ThunderSlash(Card::Club, 8)

        << new Jink(Card::Heart, 2)
        << new Jink(Card::Heart, 11)
        << new Jink(Card::Heart, 13)

        << new Jink(Card::Diamond, 2)
        << new Jink(Card::Diamond, 3)
        << new Jink(Card::Diamond, 6)
        << new Jink(Card::Diamond, 7)
        << new Jink(Card::Diamond, 7)
        << new Jink(Card::Diamond, 8)
        << new Jink(Card::Diamond, 8)
        << new Jink(Card::Diamond, 9)
        << new Jink(Card::Diamond, 10)
        << new Jink(Card::Diamond, 11)
        << new Jink(Card::Diamond, 13)

        << new Peach(Card::Heart, 4)
        << new Peach(Card::Heart, 6)
        << new Peach(Card::Heart, 7)
        << new Peach(Card::Heart, 8)
        << new Peach(Card::Heart, 9)
        << new Peach(Card::Heart, 10)
        << new Peach(Card::Heart, 12)

        << new Peach(Card::Diamond, 2)

        << new Analeptic(Card::Spade, 9)

        << new Analeptic(Card::Club, 9)

        << new Analeptic(Card::Diamond, 9);

    return cards;
}
