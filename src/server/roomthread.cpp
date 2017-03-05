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

#include "roomthread.h"
#include "room.h"
#include "engine.h"
#include "gamerule.h"
#include "scenerule.h"
#include "scenario.h"
#include "ai.h"
#include "settings.h"
#include "standard.h"
#include "json.h"
#include "structs.h"

#include <QTime>

#ifdef QSAN_UI_LIBRARY_AVAILABLE
#pragma message WARN("UI elements detected in server side!!!")
#endif

QString EventTriplet::toString() const
{
    return QString("event[%1], room[%2], target = %3[%4]\n")
        .arg(_m_event)
        .arg(_m_room->getId())
        .arg(_m_target ? _m_target->objectName() : "NULL")
        .arg(_m_target ? _m_target->getGeneralName() : QString());
}

QString HegemonyMode::GetMappedRole(const QString &kingdom)
{
    static QMap<QString, QString> roles;
    if (roles.isEmpty()) {
        roles["wei"] = "lord";
        roles["shu"] = "loyalist";
        roles["wu"] = "rebel";
        roles["qun"] = "renegade";
        roles["god"] = "careerist";
    }
    if (roles[kingdom].isEmpty())
        return kingdom;
    return roles[kingdom];
}

QString HegemonyMode::GetMappedKingdom(const QString &role)
{
    static QMap<QString, QString> kingdoms;
    if (kingdoms.isEmpty()) {
        kingdoms["lord"] = "wei";
        kingdoms["loyalist"] = "shu";
        kingdoms["rebel"] = "wu";
        kingdoms["renegade"] = "qun";
    }
    if (kingdoms[role].isEmpty())
        return role;
    return kingdoms[role];
}

RoomThread::RoomThread(Room *room)
    : room(room)
{
    //Create GameRule inside the thread where RoomThread exists
    game_rule = new GameRule(this);
}

void RoomThread::addPlayerSkills(ServerPlayer *player, bool invoke_game_start)
{
    QVariant void_data;
    bool invoke_verify = false;

    foreach (const TriggerSkill *skill, player->getTriggerSkills()) {

        if (invoke_game_start && skill->getTriggerEvents().contains(GameStart))
            invoke_verify = true;
    }

    //We should make someone trigger a whole GameStart event instead of trigger a skill only.
    if (invoke_verify)
        trigger(GameStart, room, player, void_data);
}

void RoomThread::constructTriggerTable()
{
    foreach (QString skill_name, Sanguosha->getSkillNames()) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill) addTriggerSkill(skill);
    }
    foreach(ServerPlayer *player, room->getPlayers())
        addPlayerSkills(player, true);
}

void RoomThread::actionNormal(GameRule *game_rule)
{
    try {
        forever{
            trigger(TurnStart, room, room->getCurrent());
            if (room->isFinished()) break;
            ServerPlayer *regular_next = qobject_cast<ServerPlayer *>(room->getCurrent()->getNextAlive(1, false));
            while (!room->getTag("ExtraTurnList").isNull()) {
                QStringList extraTurnList = room->getTag("ExtraTurnList").toStringList();
                if (!extraTurnList.isEmpty()) {
                    QString extraTurnPlayer = extraTurnList.takeFirst();
                    room->setTag("ExtraTurnList", QVariant::fromValue(extraTurnList));
                    ServerPlayer *next = room->findPlayer(extraTurnPlayer);
                    room->setCurrent(next);
                    trigger(TurnStart, room, next);
                    if (room->isFinished()) break;
                } else
                    room->removeTag("ExtraTurnList");
            }
            if (room->isFinished()) break;
            room->setCurrent(regular_next);
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenNormal(GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, player);
        ServerPlayer *next = qobject_cast<ServerPlayer *>(player->getNextAlive(1, false));
        if (player->getPhase() != Player::NotActive) {
            QVariant _variant;
            game_rule->effect(EventPhaseEnd, room, player, _variant, player, TriggerStruct("game_rule"));
            player->changePhase(player->getPhase(), Player::NotActive);
        }

        room->setCurrent(next);
        actionNormal(game_rule);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::run()
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    Sanguosha->registerRoom(room);

    addTriggerSkill(game_rule);
    foreach(const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
        addTriggerSkill(triggerSkill);

    if (room->getScenario() != NULL) {
        const ScenarioRule *rule = room->getScenario()->getRule();
        if (rule) addTriggerSkill(rule);
    }

    QString winner = game_rule->getWinner(room->getPlayers().first());
    if (!winner.isNull()) {
        try {
            room->gameOver(winner);
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == GameFinished) {
                terminate();
                Sanguosha->unregisterRoom();
                return;
            } else
                Q_ASSERT(false);
        }
    }

    // start game
    try {
        trigger(GameStart, room, NULL);
        constructTriggerTable();
        // delay(3000);
        actionNormal(game_rule);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == GameFinished) {
            Sanguosha->unregisterRoom();
            return;
        } else
            Q_ASSERT(false);
    }
}

// static bool compareByPriority(const TriggerSkill *a, const TriggerSkill *b)
// {
//     return a->getCurrentPriority() > b->getCurrentPriority();
// }

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data)
{
    // push it to event stack
    EventTriplet triplet(triggerEvent, room, target);
    event_stack.push_back(triplet);

    bool broken = false;
    QList<const TriggerSkill *> will_trigger;
    QSet<const TriggerSkill *> triggerable_tested;
    QList<const TriggerSkill *> rules; // we can't get a GameRule with Engine::getTriggerSkill() :(
    QMap<ServerPlayer *, QVariantList> trigger_who;

    try {
        QList<const TriggerSkill *> triggered;
        QList<const TriggerSkill *> &skills = skill_table[triggerEvent];
        //         foreach (const TriggerSkill *skill,skills){
        //             skill->setCurrentPriority(skill->getDynamicPriority(triggerEvent));
        //         }

        qStableSort(skills.begin(), skills.end(), [triggerEvent](const TriggerSkill *a, const TriggerSkill *b) {return a->getDynamicPriority(triggerEvent) > b->getDynamicPriority(triggerEvent); });

        do {
            trigger_who.clear();
            foreach (const TriggerSkill *skill, skills) {
                if (!triggered.contains(skill)) {
                    if (skill->objectName() == "game_rule" || (room->getScenario()
                        && room->getScenario()->objectName() == skill->objectName())) {
                        room->tryPause();

                        if (will_trigger.isEmpty()
                            || skill->getDynamicPriority(triggerEvent) == will_trigger.last()->getDynamicPriority(triggerEvent)) {
                            will_trigger.append(skill);
                            trigger_who[NULL].append(TriggerStruct(skill->objectName()).toVariant());       // Don't assign game rule to some player.
                            rules.append(skill);
                        } else if (skill->getDynamicPriority(triggerEvent) != will_trigger.last()->getDynamicPriority(triggerEvent))
                            break;
                        triggered.prepend(skill);
                    } else {
                        room->tryPause();
                        if (will_trigger.isEmpty()
                            || skill->getDynamicPriority(triggerEvent) == will_trigger.last()->getDynamicPriority(triggerEvent)) {

                            skill->record(triggerEvent, room, target, data);        //to record something for next.
                            QList<TriggerStruct> triggerSkillList = skill->triggerable(triggerEvent, room, target, data);

                            foreach (const TriggerStruct &skill, triggerSkillList) {
                                TriggerStruct tskill = skill;
                                if (tskill.times != 1) tskill.times = 1;            //makesure times == 1
                                const TriggerSkill *trskill = Sanguosha->getTriggerSkill(tskill.skill_name);
                                ServerPlayer *p = room->findPlayer(tskill.invoker, true);
                                if (trskill) {
                                    will_trigger.append(trskill);
                                    trigger_who[p].append(tskill.toVariant());
                                }
                            }
                        } else if (skill->getDynamicPriority(triggerEvent) != will_trigger.last()->getDynamicPriority(triggerEvent))
                            break;

                        triggered.prepend(skill);
                    }
                }
                triggerable_tested << skill;
            }
            if (!will_trigger.isEmpty()) {
                will_trigger.clear();
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    if (!trigger_who.contains(p)) continue;
                    QVariantList already_triggered;
                    forever {
                        QList<TriggerStruct> who_skills = VariantList2TriggerStructList(trigger_who.value(p));
                        if (who_skills.isEmpty()) break;
                        TriggerStruct name;
                        foreach (const TriggerStruct &skill, who_skills) {
                            const TriggerSkill *tskill = Sanguosha->getTriggerSkill(skill.skill_name);
                            if (tskill && tskill->isGlobal() && tskill->getFrequency() == Skill::Compulsory) {
                                TriggerStruct name_copy = skill;
                                if (!skill.targets.isEmpty()) name_copy.result_target = skill.targets.first();
                                name = name_copy;       // a new trick to deal with all "record-skill" or "compulsory-global",
                                                        // they should always be triggered first.
                                break;
                            }
                        }

                        if (name.skill_name.isEmpty()) {
                            if (p && !p->hasShownAllGenerals())
                                p->setFlags("Global_askForSkillCost");           // TriggerOrder need protect
                            if (who_skills.length() == 1 && (p == NULL || who_skills.first().skill_name.contains("global-fake-move"))) {
                                TriggerStruct name_copy = who_skills.first();
                                if (!who_skills.first().targets.isEmpty()) name_copy.result_target = who_skills.first().targets.first();
                                name = name_copy;
                            } else if (p != NULL) {
                                QString reason = "GameRule:TriggerOrder";
                                foreach (const TriggerStruct &skill, who_skills) {
                                    if (skill.skill_name.contains("GameRule_AskForGeneralShow")) {
                                        reason = "GameRule:TurnStart";
                                        break;
                                    }
                                }
                                name = room->askForSkillTrigger(p, reason, who_skills, data);
                            } else
                                name = who_skills.last();
                            if (p && p->hasFlag("Global_askForSkillCost"))
                                p->setFlags("-Global_askForSkillCost");
                        }

                        if (name.skill_name.isEmpty()) break;

                        const TriggerSkill *result_skill = Sanguosha->getTriggerSkill(name.skill_name);
                        ServerPlayer *skill_target = room->findPlayer(name.result_target, true);
                        if (!skill_target) skill_target = target;
                        if (name.result_target.isEmpty() && target)
                            name.result_target = target->objectName();

                        Q_ASSERT(result_skill);

                        if (!name.targets.isEmpty()) {                         //merge the same
                            bool deplicated = false;

                            foreach (QVariant already_skill, already_triggered) {
                                TriggerStruct already_s;
                                already_s.tryParse(already_skill);
                                if (already_s.skill_name == name.skill_name && already_s.invoker == name.invoker && !already_s.targets.isEmpty()) {
                                    ServerPlayer *triggered_target = room->findPlayer(already_s.result_target, true);
                                    ServerPlayer *target = room->findPlayer(name.result_target, true);
                                    QList<ServerPlayer *> all = room->getAllPlayers(true);
                                    if (all.indexOf(target) > all.indexOf(triggered_target)) {
                                        already_triggered.replace(already_triggered.indexOf(already_skill), name.toVariant());
                                        deplicated = true;
                                        break;
                                    }
                                }
                            }
                            if (!deplicated)
                                already_triggered.append(name.toVariant());
                        } else
                            already_triggered.append(name.toVariant());

                        //----------------------------------------------- TriggerSkill::cost
                        bool do_effect = false;

                        if (p && !p->hasShownSkill(result_skill))
                            p->setFlags("Global_askForSkillCost");           // SkillCost need protect

                        TriggerStruct cost_str = result_skill->cost(triggerEvent, room, skill_target, data, p, name);
                        result_skill = Sanguosha->getTriggerSkill(cost_str.skill_name);

                        if (result_skill) {
                            do_effect = true;
                            if (p) {
                                bool compulsory_shown = false;
                                if (result_skill->getFrequency() == Skill::Compulsory && p->hasShownSkill(result_skill))
                                    compulsory_shown = true;
                                bool show = p->showSkill(result_skill->objectName(), cost_str.skill_position);
                                if (!compulsory_shown && show) p->tag["JustShownSkill"] = result_skill->objectName();
                            }
                        }

                        if (p && p->hasFlag("Global_askForSkillCost"))                      // for next time
                            p->setFlags("-Global_askForSkillCost");

                        //----------------------------------------------- TriggerSkill::effect
                        if (do_effect) {
                            ServerPlayer *invoker = room->findPlayer(cost_str.invoker, true);       //it may point to another skill, such as EightDiagram to bazhen
                            skill_target = room->findPlayer(cost_str.result_target, true);
                            broken = result_skill->effect(triggerEvent, room, skill_target, data, invoker, cost_str);
                            if (broken) {
                                p->tag.remove("JustShownSkill");
                                break;
                            }
                        }
                        //-----------------------------------------------
                        p->tag.remove("JustShownSkill");

                        trigger_who.clear();
                        foreach (const TriggerSkill *skill, triggered) {
                            if (skill->objectName() == "game_rule" || (room->getScenario()
                                && room->getScenario()->objectName() == skill->objectName())) {
                                room->tryPause();
                                continue; // dont assign them to some person.
                            } else {
                                room->tryPause();
                                if (skill->getDynamicPriority(triggerEvent) == triggered.first()->getDynamicPriority(triggerEvent)) {
                                    QList<TriggerStruct> triggerSkillList = skill->triggerable(triggerEvent, room, target, data);
                                    foreach (ServerPlayer *p, room->getPlayers()) {
                                        foreach (const TriggerStruct &skill, triggerSkillList) {
                                            if (p->objectName() == skill.invoker) {
                                                TriggerStruct tskill = skill;
                                                if (tskill.times != 1) tskill.times = 1;            //makesure times == 1
                                                const TriggerSkill *trskill = Sanguosha->getTriggerSkill(tskill.skill_name);
                                                if (trskill)
                                                    trigger_who[p].append(tskill.toVariant());
                                            }
                                        }
                                    }
                                } else
                                    break;
                            }
                        }
                        foreach (QVariant already_skill, already_triggered) {
                            TriggerStruct already_s;
                            already_s.tryParse(already_skill);
                            QVariantList who_skills_q = trigger_who.value(p);

                            foreach (QVariant skill, who_skills_q) {
                                TriggerStruct re_skill;
                                if (re_skill.tryParse(skill) && already_s.invoker == re_skill.invoker &&
                                        already_s.skill_name == re_skill.skill_name && already_s.skill_owner == re_skill.skill_owner) {
                                    if (!already_s.targets.isEmpty() && !re_skill.targets.isEmpty()) {
                                        QStringList targets;
                                        ServerPlayer *already_target = room->findPlayer(already_s.result_target, true);
                                        ServerPlayer *target = room->findPlayer(re_skill.targets.first(), true);
                                        QList<ServerPlayer *> all = room->getAllPlayers(true);
                                        if (all.indexOf(already_target) < all.indexOf(target))
                                            continue;

                                        for (int i = 1 + all.indexOf(already_target); i < room->getAllPlayers(true).length(); i++) {
                                            QString target = room->getAllPlayers(true).at(i)->objectName();
                                            if (re_skill.targets.contains(target))
                                                targets << target;
                                        }

                                        if (!targets.isEmpty()) {
                                            TriggerStruct re_skill2 = already_s;
                                            re_skill2.targets = targets;
                                            re_skill2.result_target = QString();
                                            trigger_who[p].replace(trigger_who[p].indexOf(skill), re_skill2.toVariant());
                                            break;
                                        }
                                    }

                                    trigger_who[p].removeOne(skill);
                                    break;
                                }
                            }
                        }
/*
                        if (has_compulsory) {
                            has_compulsory = false;
                            foreach (QVariant s, trigger_who[p]) {
                                TriggerStruct skill;
                                skill.tryParse(s);
                                const TriggerSkill *trskill = Sanguosha->getTriggerSkill(skill.skill_name);
                                if (trskill && (p->hasShownSkill(trskill) || trskill->isGlobal())
                                    && (trskill->getFrequency() == Skill::Compulsory
                                    //|| trskill->getFrequency() == Skill::NotCompulsory //for Paoxia, Anjian, etc.
                                    || trskill->getFrequency() == Skill::Wake)) {
                                    has_compulsory = true;
                                    break;
                                }
                            }
                        }
*/
                    }

                    if (broken) break;
                }
                // @todo_Slob: for drawing cards when game starts -- stupid design of triggering no player!
                if (!broken) {
                    if (!trigger_who[NULL].isEmpty()) {
                        foreach (QVariant q, trigger_who[NULL]) {
                            TriggerStruct s;
                            s.tryParse(q);
                            const TriggerSkill *skill = NULL;
                            foreach (const TriggerSkill *rule, rules) { // because we cannot get a GameRule with Engine::getTriggerSkill()
                                if (rule->objectName() == s.skill_name) {
                                    skill = rule;
                                    break;
                                }
                            }

                            Q_ASSERT(skill != NULL);
                            TriggerStruct skill_cost = skill->cost(triggerEvent, room, target, data, NULL, s);
                            if (!skill_cost.skill_name.isEmpty()) {
                                broken = skill->effect(triggerEvent, room, target, data, NULL, skill_cost);
                                if (broken)
                                    break;
                            }
                        }
                    }
                }
            }
            if (broken)
                break;
        } while (skills.length() != triggerable_tested.size());

        if (target) {
            foreach(AI *ai, room->ais)
                ai->filterEvent(triggerEvent, target, data);
        }

        // pop event stack
        event_stack.pop_back();
    }
    catch (TriggerEvent throwed_event) {
        if (target) {
            foreach(AI *ai, room->ais)
                ai->filterEvent(triggerEvent, target, data);
        }

        // pop event stack
        event_stack.pop_back();

        throw throwed_event;
    }

    room->tryPause();
    return broken;
}

const QList<EventTriplet> *RoomThread::getEventStack() const
{
    return &event_stack;
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target)
{
    QVariant data;
    return trigger(triggerEvent, room, target, data);
}

void RoomThread::addTriggerSkill(const TriggerSkill *skill)
{
    if (skill == NULL || skillSet.contains(skill->objectName()))
        return;

    skillSet << skill->objectName();

    QList<TriggerEvent> events = skill->getTriggerEvents();
    foreach (const TriggerEvent &triggerEvent, events) {
        QList<const TriggerSkill *> &table = skill_table[triggerEvent];
        table << skill;
        qStableSort(table.begin(), table.end(), [triggerEvent](const TriggerSkill *a, const TriggerSkill *b) {return a->getDynamicPriority(triggerEvent) > b->getDynamicPriority(triggerEvent); });
    }

    if (skill->isVisible()) {
        foreach (const Skill *skill, Sanguosha->getRelatedSkills(skill->objectName())) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill)
                addTriggerSkill(trigger_skill);
        }
    }
}

void RoomThread::delay(long secs)
{
    if (secs == -1) secs = Config.AIDelay;
    Q_ASSERT(secs >= 0);
    if (room->property("to_test").toString().isEmpty() && Config.AIDelay > 0)
        msleep(secs);
}

