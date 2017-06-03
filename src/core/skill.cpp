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

#include "skill.h"
#include "settings.h"
#include "engine.h"
#include "player.h"
#include "room.h"
#include "client.h"
#include "standard.h"
#include "scenario.h"
#include "serverplayer.h"

#include <QFile>
#include <QDir>

Skill::Skill(const QString &name, Frequency frequency)
    : frequency(frequency), limit_mark(QString()), relate_to_place(QString()), attached_lord_skill(false), skill_type(Normal)
{
    static QChar lord_symbol('$');

    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        setObjectName(copy);
        lord_skill = true;
    } else {
        setObjectName(name);
        lord_skill = false;
    }
}

bool Skill::isLordSkill() const
{
    return lord_skill;
}

bool Skill::isAttachedLordSkill() const
{
    return attached_lord_skill;
}

QString Skill::getDescription(bool inToolTip, bool in_game) const
{
    QString desc;
    if (!canPreshow())
        desc.prepend(QString("<font color=gray>(%1)</font><br/>").arg(tr("this skill cannot preshow")));

    QString skill_name = objectName();

    if (objectName().contains("_")) {
        skill_name = objectName().split("_").first();
    }

    QString des_src = Sanguosha->translate(":" + skill_name);
    if ((in_game && Sanguosha->getSkill(skill_name) && Sanguosha->getSkill(skill_name)->isAttachedLordSkill()
            && (!Self->isLord() || !Self->getGeneral()->getRelatedSkillNames().contains(skill_name))) || ServerInfo.SkillModify.contains(skill_name))
        des_src = Sanguosha->translate("&" + skill_name) != "&" + skill_name ? Sanguosha->translate("&" + skill_name) : des_src;
    if (des_src == ":" + skill_name || des_src == "&" + skill_name)
        return desc;

    foreach (const QString &skill_type, Sanguosha->getSkillColorMap().keys()) {
        QString to_replace = Sanguosha->translate(skill_type);
        if (to_replace == skill_type) continue;
        QString color_str = Sanguosha->getSkillColor(skill_type).name();
        if (des_src.contains(to_replace))
            des_src.replace(to_replace, QString("<font color=%1><b>%2</b></font>").arg(color_str)
            .arg(to_replace));
    }

    for (int i = 0; i < 6; i++) {
        Card::Suit suit = (Card::Suit)i;
        QString str = Card::Suit2String(suit);
        QString to_replace = Sanguosha->translate(str);
        bool red = suit == Card::Heart
            || suit == Card::Diamond
            || suit == Card::NoSuitRed;
        if (to_replace == str) continue;
        if (des_src.contains(to_replace)) {
            if (red)
                des_src.replace(to_replace, QString("<font color=#FF0000>%1</font>").arg(Sanguosha->translate(str + "_char")));
            else
                des_src.replace(to_replace, QString("<font color=#000000><span style=background-color:white>%1</span></font>").arg(Sanguosha->translate(str + "_char")));
        }
    }

    desc.append(QString("<font color=%1>%2</font>").arg(inToolTip ? Config.SkillDescriptionInToolTipColor.name() : Config.SkillDescriptionInOverviewColor.name()).arg(des_src));
    return desc;
}

QString Skill::getNotice(int index) const
{
    if (index == -1)
        return Sanguosha->translate("~" + objectName());

    return Sanguosha->translate(QString("~%1%2").arg(objectName()).arg(index));
}

bool Skill::isVisible() const
{
    return !objectName().startsWith("#");
}

int Skill::getEffectIndex(const ServerPlayer *, const Card *) const
{
    return -1;
}

void Skill::initMediaSource()
{
    sources.clear();
    QDir dir;
    dir.setPath("./audio/skill");
    dir.setFilter(QDir::Files | QDir::Hidden);
    dir.setSorting(QDir::Name);
    QStringList names = dir.entryList();
    QStringList newnames = names.filter(objectName() + "_");
    foreach (QString name, newnames) {
        if (QFile::exists("audio/skill/" + name))
            sources << "audio/skill/" + name;
    }

    for (int i = 1;; ++i) {
        QString effect_file = QString("audio/skill/%1%2.ogg").arg(objectName()).arg(QString::number(i));
        if (QFile::exists(effect_file) && !sources.contains(effect_file))
            sources << effect_file;
        else
            break;
    }

    if (sources.isEmpty()) {
        QString effect_file = QString("audio/skill/%1.ogg").arg(objectName());
        if (QFile::exists(effect_file) && !sources.contains(effect_file))
            sources << effect_file;
    }
}

void Skill::playAudioEffect(int index) const
{
    if (!sources.isEmpty()) {
        if (index == -1)
            index = qrand() % sources.length();
        else
            index--;

        // check length
        QString filename;
        if (index >= 0 && index < sources.length())
            filename = sources.at(index);
        else if (index >= sources.length()) {
            while (index >= sources.length())
                index -= sources.length();
            filename = sources.at(index);
        } else
            filename = sources.first();

        Sanguosha->playAudioEffect(filename);
    }
}

Skill::Frequency Skill::getFrequency() const
{
    return frequency;
}

Skill::SkillType Skill::getType() const
{
    return skill_type;
}

QString Skill::getLimitMark() const
{
    return limit_mark;
}

QStringList Skill::getSources(const QString &general, const int skinId) const
{
    if (skinId == 0)
        return sources;

    const QString key = QString("%1_%2")
        .arg(QString::number(skinId))
        .arg(general);

    if (skinSourceHash.contains(key))
        return skinSourceHash[key];

    for (int i = 1;; ++i) {
        QString effectFile = QString("hero-skin/%1/%2/%3%4.ogg")
            .arg(general).arg(QString::number(skinId))
            .arg(objectName()).arg(QString::number(i));
        if (QFile::exists(effectFile))
            skinSourceHash[key] << effectFile;
        else
            break;
    }

    if (skinSourceHash[key].isEmpty()) {
        QString effectFile = QString("hero-skin/%1/%2/%3.ogg")
            .arg(general).arg(QString::number(skinId)).arg(objectName());
        if (QFile::exists(effectFile))
            skinSourceHash[key] << effectFile;
    }

    return skinSourceHash[key].isEmpty() ? sources : skinSourceHash[key];
}

QStringList Skill::getSources() const
{
    return sources;
}

QDialog *Skill::getDialog() const
{
    return NULL;
}

QString Skill::getGuhuoBox() const
{
    if (inherits("TriggerSkill")) {
        const TriggerSkill *triskill = qobject_cast<const TriggerSkill *>(this);
        return triskill->getGuhuoBox();
    } else if (inherits("ViewAsSkill")) {
        const ViewAsSkill *view_as_skill = qobject_cast<const ViewAsSkill *>(this);
        return view_as_skill->getGuhuoBox();
    }
    return "";
}

bool Skill::canPreshow() const
{
    if (inherits("TriggerSkill")) {
        const TriggerSkill *triskill = qobject_cast<const TriggerSkill *>(this);
        return triskill->getViewAsSkill() == NULL;
    }

    return false;
}

bool Skill::relateToPlace(bool head) const
{
    if (head)
        return relate_to_place == "head";
    else
        return relate_to_place == "deputy";
    return false;
}

bool Skill::chooseFilter(const QList<const Card *> &, const Card *) const
{
    return false;
}

ViewAsSkill::ViewAsSkill(const QString &name)
    : Skill(name), response_pattern(QString()), response_or_use(false), expand_pile(QString())
{
}

bool ViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *) const
{
    return viewFilter(selected, to_select);
}

QString ViewAsSkill::getGuhuoBox() const
{
    return guhuo_type;
}

bool ViewAsSkill::isAvailable(const Player *invoker, CardUseStruct::CardUseReason reason, const QString &pattern, const QString &) const
{
    if (!inherits("TransferSkill") && !invoker->hasSkill(objectName()))
        return false;

    if (reason == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && pattern == "nullification")
        return isEnabledAtNullification(invoker);

    switch (reason) {
        case CardUseStruct::CARD_USE_REASON_PLAY: return isEnabledAtPlay(invoker);
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: return isEnabledAtResponse(invoker, pattern);
        default:
            return false;
    }
}

bool ViewAsSkill::isEnabledAtPlay(const Player *) const
{
    return response_pattern.isEmpty();
}

bool ViewAsSkill::isEnabledAtResponse(const Player *, const QString &pattern) const
{
    if (!response_pattern.isEmpty())
        return pattern == response_pattern;
    return false;
}

bool ViewAsSkill::isEnabledAtNullification(const Player *) const
{
    return false;
}

const ViewAsSkill *ViewAsSkill::parseViewAsSkill(const Skill *skill)
{
    if (skill == NULL) return NULL;
    if (skill->inherits("ViewAsSkill")) {
        const ViewAsSkill *view_as_skill = qobject_cast<const ViewAsSkill *>(skill);
        return view_as_skill;
    }
    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL) return view_as_skill;
    }
    if (skill->inherits("DistanceSkill")) {
        const DistanceSkill *trigger_skill = qobject_cast<const DistanceSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL) return view_as_skill;
    }
    return NULL;
}

QString ViewAsSkill::getExpandPile() const
{
    return expand_pile;
}

QList<const Card *> ViewAsSkill::getGuhuoCards(const QList<const Card *> &) const
{
    return QList<const Card *>();
}

QStringList ViewAsSkill::getGuhuoCards(const QString &type) const
{
    QStringList all;
    QList<const Card *> cards = Sanguosha->getCards();
    if (type.contains("b")) {
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !all.contains(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                all << card->objectName();
            }
        }
    }

    if (type.contains("t")) {
        foreach (const Card *card, cards) {
            if (card->isNDTrick() && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                if (all.contains(card->objectName()))
                    continue;

                if (card->inherits("SingleTargetTrick"))
                    all << card->objectName();
                else
                    all << card->objectName();

            }
        }
    }

    if (type.contains("d")) {
        foreach (const Card *card, cards) {
            if (!card->isNDTrick() && card->isKindOf("TrickCard") && !all.contains(card->objectName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                all << card->objectName();
            }
        }
    }

    return all;
}

ZeroCardViewAsSkill::ZeroCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
{
}

const Card *ZeroCardViewAsSkill::viewAs(const QList<const Card *> &cards) const
{
    if (cards.isEmpty())
        return viewAs();
    else
        return NULL;
}

bool ZeroCardViewAsSkill::viewFilter(const QList<const Card *> &, const Card *) const
{
    return false;
}

QString ZeroCardViewAsSkill::getExpandPile() const
{
    return expand_pile;
}

OneCardViewAsSkill::OneCardViewAsSkill(const QString &name)
    : ViewAsSkill(name), filter_pattern(QString())
{
}

bool OneCardViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select) const
{
    return selected.isEmpty() && !to_select->hasFlag("using") && viewFilter(to_select);
}

bool OneCardViewAsSkill::viewFilter(const Card *to_select) const
{
    if (!inherits("FilterSkill") && !filter_pattern.isEmpty()) {
        QString pat = filter_pattern;
        if (pat.endsWith("!")) {
            if (Self->isJilei(to_select)) return false;
            pat.chop(1);
        } else if (response_or_use && pat.contains("hand")) {
            QStringList handlist;
            handlist.append("hand");
            foreach (const QString &pile, Self->getPileNames()) {
                if (pile.startsWith("&") || pile == "wooden_ox")
                    handlist.append(pile);
            }
            pat.replace("hand", handlist.join(","));
        }
        ExpPattern pattern(pat);
        return pattern.match(Self, to_select);
    }
    return false;
}

const Card *OneCardViewAsSkill::viewAs(const QList<const Card *> &cards) const
{
    if (cards.length() != 1)
        return NULL;
    else
        return viewAs(cards.first());
}

QString OneCardViewAsSkill::getExpandPile() const
{
    return expand_pile;
}

FilterSkill::FilterSkill(const QString &name)
    : OneCardViewAsSkill(name)
{
    frequency = Compulsory;
}

bool FilterSkill::isEnabledAtPlay(const Player *) const
{
    return false;
}

TriggerSkill::TriggerSkill(const QString &name)
    : Skill(name), view_as_skill(NULL), global(false), current_priority(0.0)
{
    priority.clear();
    guhuo_type = "";
}

const ViewAsSkill *TriggerSkill::getViewAsSkill() const
{
    return view_as_skill;
}

QList<TriggerEvent> TriggerSkill::getTriggerEvents() const
{
    return events;
}

int TriggerSkill::getPriority() const
{
    return 3;
}

double TriggerSkill::getDynamicPriority(TriggerEvent e) const
{
    if (priority.keys().contains(e))
        return priority.key(e);
    else
        return this->getPriority();
}

void TriggerSkill::insertPriority(TriggerEvent e, double value)
{
    priority.insert(e, value);
}

void TriggerSkill::record(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
{

}

QString TriggerSkill::getGuhuoBox() const
{
    return guhuo_type;
}

bool TriggerSkill::triggerable(const ServerPlayer *target) const
{
    return target != NULL && target->isAlive() && target->hasSkill(objectName());
}

QList<TriggerStruct> TriggerSkill::triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
{
    QList<TriggerStruct> skill_lists;
    if (objectName() == "game_rule") return skill_lists;

    TriggerStruct skill_list = triggerable(triggerEvent, room, player, data, NULL);
    if (!skill_list.skill_name.isEmpty()) {
        for (int i = 0; i < skill_list.times; i++) {
            TriggerStruct skill = skill_list;
            skill.times = 1;
            skill_lists << skill;
        }
    }

    return skill_lists;
}

TriggerStruct TriggerSkill::triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &, ServerPlayer *) const
{
    if (triggerable(target))
        return TriggerStruct(objectName(), target);
    return TriggerStruct();
}

TriggerStruct TriggerSkill::cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *, const TriggerStruct &info) const
{
    return info;
}

bool TriggerSkill::effect(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *, const TriggerStruct &) const
{
    return false;
}

TriggerSkill::~TriggerSkill()
{
    if (view_as_skill && !view_as_skill->inherits("LuaViewAsSkill"))
        delete view_as_skill;
    events.clear();
}

ScenarioRule::ScenarioRule(Scenario *scenario)
    :TriggerSkill(scenario->objectName())
{
    setParent(scenario);
}

int ScenarioRule::getPriority() const
{
    return 0;
}

bool ScenarioRule::triggerable(const ServerPlayer *) const
{
    return true;
}

MasochismSkill::MasochismSkill(const QString &name)
    : TriggerSkill(name)
{
    events << Damaged;
}

TriggerStruct MasochismSkill::cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who, const TriggerStruct &info) const
{
    return TriggerSkill::cost(triggerEvent, room, player, data, ask_who, info);
}

bool MasochismSkill::effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
{
    DamageStruct damage = data.value<DamageStruct>();
    onDamaged(player, damage, info);

    return false;
}

PhaseChangeSkill::PhaseChangeSkill(const QString &name)
    : TriggerSkill(name)
{
    events << EventPhaseStart;
}

bool PhaseChangeSkill::effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
{
    return onPhaseChange(player, info);
}

DrawCardsSkill::DrawCardsSkill(const QString &name)
    : TriggerSkill(name)
{
    events << DrawNCards;
}

bool DrawCardsSkill::effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
{
    int n = data.toInt();
    data = getDrawNum(player, n);
    return false;
}

GameStartSkill::GameStartSkill(const QString &name)
    : TriggerSkill(name)
{
    events << GameStart;
}

bool GameStartSkill::effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &info) const
{
    onGameStart(player, info);
    return false;
}

BattleArraySkill::BattleArraySkill(const QString &name, const HegemonyMode::ArrayType type)
    : TriggerSkill(name), array_type(type)
{
    if (!inherits("LuaBattleArraySkill")) //extremely dirty hack!!!
        view_as_skill = new ArraySummonSkill(objectName());
}

bool BattleArraySkill::triggerable(const ServerPlayer *player) const
{
    return TriggerSkill::triggerable(player) && player->aliveCount() >= 4;
}

void BattleArraySkill::summonFriends(ServerPlayer *player) const
{
    player->summonFriends(array_type);
}

ArraySummonSkill::ArraySummonSkill(const QString &name)
    : ZeroCardViewAsSkill(name)
{

}

const Card *ArraySummonSkill::viewAs() const
{
    QString name = objectName();
    name[0] = name[0].toUpper();
    name += "Summon";
    Card *card = Sanguosha->cloneSkillCard(name);
    card->setShowSkill(objectName());
    return card;
}

using namespace HegemonyMode;
bool ArraySummonSkill::isEnabledAtPlay(const Player *player) const
{
    if (player->getAliveSiblings().length() < 3) return false;
    if (player->hasFlag("Global_SummonFailed")) return false;
    if (!player->canShowGeneral(player->inHeadSkills(objectName()) ? "h" : "d")) return false;
    if (!player->hasShownOneGeneral() && player->willbeRole() == "careerist") return false;
    const BattleArraySkill *skill = qobject_cast<const BattleArraySkill *>(Sanguosha->getTriggerSkill(objectName()));
    if (skill) {
        ArrayType type = skill->getArrayType();
        switch (type) {
            case Siege: {
                if (player->willBeFriendWith(player->getNextAlive())
                    && player->willBeFriendWith(player->getLastAlive()))
                    return false;
                if (!player->willBeFriendWith(player->getNextAlive())) {
                    if (!player->getNextAlive(2)->hasShownOneGeneral() && player->getNextAlive()->hasShownOneGeneral())
                        return true;
                }
                if (!player->willBeFriendWith(player->getLastAlive()))
                    return !player->getLastAlive(2)->hasShownOneGeneral() && player->getLastAlive()->hasShownOneGeneral();
                break;
            }
            case Formation: {
                int n = player->aliveCount(false);
                int asked = n;
                for (int i = 1; i < n; ++i) {
                    Player *target = player->getNextAlive(i);
                    if (player->isFriendWith(target))
                        continue;
                    else if (!target->hasShownOneGeneral())
                        return true;
                    else {
                        asked = i;
                        break;
                    }
                }
                n -= asked;
                for (int i = 1; i < n; ++i) {
                    Player *target = player->getLastAlive(i);
                    if (player->isFriendWith(target))
                        continue;
                    else return !target->hasShownOneGeneral();
                }
                break;
            }
        }
    }
    return false;
}

int MaxCardsSkill::getExtra(const ServerPlayer *, MaxCardsType::MaxCardsCount) const
{
    return 0;
}

int MaxCardsSkill::getFixed(const ServerPlayer *, MaxCardsType::MaxCardsCount) const
{
    return -1;
}

ProhibitSkill::ProhibitSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

DistanceSkill::DistanceSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
    view_as_skill = new ShowDistanceSkill(objectName());
}

const ViewAsSkill *DistanceSkill::getViewAsSkill() const
{
    return view_as_skill;
}

int DistanceSkill::getCorrect(const Player *, const Player *) const
{
    return 0;
}

int DistanceSkill::getFixed(const Player *, const Player *) const
{
    return 0;
}

ShowDistanceSkill::ShowDistanceSkill(const QString &name)
    : ZeroCardViewAsSkill(name)
{
}

const Card *ShowDistanceSkill::viewAs() const
{
    SkillCard *card = Sanguosha->cloneSkillCard("ShowMashu");
    card->setUserString(objectName());
    return card;
}

bool ShowDistanceSkill::isEnabledAtPlay(const Player *player) const
{
    const DistanceSkill *skill = qobject_cast<const DistanceSkill *>(Sanguosha->getSkill(objectName()));
    if (skill && player->hasSkill(objectName())) {
        if (!player->hasShownSkill(skill->objectName())) return true;
    }
    return false;
}

MaxCardsSkill::MaxCardsSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

TargetModSkill::TargetModSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
    pattern = "Slash";
}

QString TargetModSkill::getPattern() const
{
    return pattern;
}

int TargetModSkill::getResidueNum(const Player *, const Card *) const
{
    return 0;
}

int TargetModSkill::getExtraTargetNum(const Player *, const Card *) const
{
    return 0;
}

bool TargetModSkill::getDistanceLimit(const Player *, const Player *, const Card *) const
{
    return false;
}

bool TargetModSkill::checkSpecificAssignee(const Player *, const Player *, const Card *) const
{
    return false;
}

bool TargetModSkill::checkExtraTargets(const Player *, const Player *, const Card *, const QList<const Player *> &, const QList<const Player *> &) const
{
    return false;
}

int TargetModSkill::getEffectIndex(const ServerPlayer *, const Card *, const TargetModSkill::ModType) const
{
    return -1;
}

SlashNoDistanceLimitSkill::SlashNoDistanceLimitSkill(const QString &skill_name)
    : TargetModSkill(QString("#%1-slash-ndl").arg(skill_name)), name(skill_name)
{
}

int SlashNoDistanceLimitSkill::getDistanceLimit(const Player *from, const Card *card) const
{
    if (from->hasSkill(name) && card->getSkillName() == name)
        return 1000;
    else
        return 0;
}

AttackRangeSkill::AttackRangeSkill(const QString &name) : Skill(name, Skill::Compulsory)
{

}

int AttackRangeSkill::getExtra(const Player *, bool) const
{
    return 0;
}

int AttackRangeSkill::getFixed(const Player *, bool) const
{
    return -1;
}

FakeMoveSkill::FakeMoveSkill(const QString &name)
    : TriggerSkill(QString("#%1-fake-move").arg(name)), name(name)
{
    events << BeforeCardsMove << CardsMoveOneTime;
}

int FakeMoveSkill::getPriority() const
{
    return 10;
}

TriggerStruct FakeMoveSkill::triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &, ServerPlayer *) const
{
	return (target != NULL) ? TriggerStruct(objectName(), target) : TriggerStruct();
}

bool FakeMoveSkill::effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *, const TriggerStruct &) const
{
    QString flag = QString("%1_InTempMoving").arg(name);

    foreach(ServerPlayer *p, room->getAllPlayers())
        if (p->hasFlag(flag)) return true;

    return false;
}

DetachEffectSkill::DetachEffectSkill(const QString &skillname, const QString &pilename)
    : TriggerSkill(QString("#%1-clear").arg(skillname)), name(skillname), pile_name(pilename)
{
    events << EventLoseSkill;
    frequency = Compulsory;
}

TriggerStruct DetachEffectSkill::triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *) const
{
    TriggerStruct trigger;
    if (target && data.value<InfoStruct>().info == name) {
        trigger = TriggerStruct(objectName(), target);
        trigger.skill_position = data.value<InfoStruct>().head ? "head" : "deputy";
    }

    return trigger;
}

TriggerStruct DetachEffectSkill::cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &info) const
{
    if (player->hasSkill(name) && player->isAlive()) {
        QString position = data.value<InfoStruct>().head ? "deputy" : "head";
        if (!player->hasShownSkill(name) && player->askForSkillInvoke(name, QVariant(), position)) {
            player->showSkill(name, position);
            return TriggerStruct();
        } else if (player->hasShownSkill(name))
            return TriggerStruct();
    }

    return info;
}

bool DetachEffectSkill::effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *, const TriggerStruct &) const
{
    if (!pile_name.isEmpty())
        player->clearOnePrivatePile(pile_name);

    onSkillDetached(room, player, data);
    return false;
}

void DetachEffectSkill::onSkillDetached(Room *, ServerPlayer *, QVariant &) const
{
}

WeaponSkill::WeaponSkill(const QString &name)
    : TriggerSkill(name)
{
}

int WeaponSkill::getPriority() const
{
    return 2;
}

bool WeaponSkill::triggerable(const ServerPlayer *target) const
{
    if (target == NULL) return false;
    if (target->getMark("Equips_Nullified_to_Yourself") > 0) return false;
    return target->hasWeapon(objectName());
}

ArmorSkill::ArmorSkill(const QString &name)
    : TriggerSkill(name)
{
}

int ArmorSkill::getPriority() const
{
    return 2;
}

bool ArmorSkill::triggerable(const ServerPlayer *target) const
{
    if (target == NULL)
        return false;
    return target->hasArmorEffect(objectName());
}

TriggerStruct ArmorSkill::cost(Room *room, QVariant &data, const TriggerStruct &info) const
{
    ServerPlayer *target = room->findPlayer(info.invoker, true);
    if (target->hasArmorEffect(objectName(), false) &&
        (getFrequency() == Skill::Compulsory || target->askForSkillInvoke(objectName(), data)))
        return info;

    QStringList all;
    bool show = false;
    bool protect = true;
    foreach(const ViewHasSkill *vhskill, Sanguosha->ViewHas(target, objectName(), "armor")) {
        const Skill *mskill = Sanguosha->getMainSkill(vhskill->objectName());
        if (getFrequency() == Skill::Compulsory && target->hasShownSkill(mskill)) show = true;
        if (target->hasShownSkill(mskill)) protect = false;
        all << mskill->objectName();
    }
    if (protect) target->setFlags("Global_askForSkillCost");

    TriggerStruct skill;
    skill.invoker = info.invoker;
    skill.result_target = info.result_target;

    if (!all.isEmpty()) {
        SPlayerDataMap map;
        map[target] = all;
        QString result = room->askForTriggerOrder(target, "armorskill", map, !show, data);
        if (result == "cancel")
            return TriggerStruct();
        else {
            skill.skill_name = result.split(":").last();
            skill.skill_position = result.contains("?") ? result.split(":").first().split("?").last() : QString();
            const TriggerSkill *result_skill = Sanguosha->getTriggerSkill(skill.skill_name);
            if (result_skill) {
                if (show || all.length() > 1 || target->askForSkillInvoke(result_skill->objectName(), data, skill.skill_position)) {
                    return skill;
                }
            } else if (show || all.length() > 1 || target->askForSkillInvoke(result_skill->objectName(), data, skill.skill_position))
                return info;
        }
    }
    return TriggerStruct();
}

TreasureSkill::TreasureSkill(const QString &name)
    : TriggerSkill(name)
{
}

int TreasureSkill::getPriority() const
{
    return 2;
}

bool TreasureSkill::triggerable(const ServerPlayer *target) const
{
    if (target == NULL)
        return false;
    return target->hasTreasure(objectName());
}

FixCardSkill::FixCardSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

ViewHasSkill::ViewHasSkill(const QString &name)
    : Skill(name, Skill::Compulsory), global(false), viewhas_skills(QStringList()), viewhas_armors(QStringList())
{
}
