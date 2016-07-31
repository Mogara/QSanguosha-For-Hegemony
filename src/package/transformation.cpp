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
#include "standard-basics.h"
#include "standard-tricks.h"
#include "client.h"
#include "engine.h"
#include "structs.h"
#include "gamerule.h"
#include "settings.h"
#include "json.h"
#include "roomthread.h"

//LengTong
class LieFeng : public TriggerSkill
{
public:
    LieFeng() : TriggerSkill("liefeng")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(lengtong)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == lengtong && move.from_places.contains(Player::PlaceEquip)) {
            QList<ServerPlayer *> other_players = room->getOtherPlayers(lengtong);
            foreach (ServerPlayer *p, other_players) {
                if (lengtong->canDiscard(p, "he"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> other_players = room->getOtherPlayers(lengtong);
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, other_players) {
            if (lengtong->canDiscard(p, "he"))
                targets << p;
        }
        ServerPlayer *to = room->askForPlayerChosen(lengtong, targets, objectName(), "liefeng-invoke", true, true);
        if (to) {
            lengtong->tag["liefeng_target"] = QVariant::fromValue(to);
            room->broadcastSkillInvoke(objectName(), lengtong);
            return true;
        } else lengtong->tag.remove("liefeng_target");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lengtong, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *to = lengtong->tag["liefeng_target"].value<ServerPlayer *>();
        lengtong->tag.remove("liefeng_target");
        if (to && lengtong->canDiscard(to, "he")) {
            int card_id = room->askForCardChosen(lengtong, to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, to, lengtong);
        }
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
        return card_ids.contains(to_select->getId());
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
    room->broadcastSkillInvoke("xuanlue", card_use.from);
    room->doSuperLightbox("lengtong", "xuanlue");
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
    QList<ServerPlayer *> targets;
    QList<int> cards, cards_copy;
    int i = 0;
    QList<CardsMoveStruct> moves;
    do {
        i++;
        targets.clear();
        foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
            foreach (const Card *card, p->getEquips())
            if (!cards.contains(card->getEffectiveId()) && source->canGetCard(p, card->getEffectiveId()))
                targets << p;
        }
        if (!targets.isEmpty()) {
            ServerPlayer *to = room->askForPlayerChosen(source, targets, "xuanlue", "xuanlue-get", true, false);
            if (to) {
                int card_id = room->askForCardChosen(source, to, "e", "xuanlue", false, Card::MethodGet, cards);
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
                CardsMoveStruct move(card_id, source, Player::PlaceHand, reason);
                moves.append(move);
                cards.append(card_id);
            }
        }
    }
    while (cards.size() == i && i < 3);

    if (!cards.isEmpty()) {
        room->moveCardsAtomic(moves, true);
        cards_copy = cards;
        foreach (int id, cards_copy)
            if (room->getCardPlace(id) != Player::PlaceHand || room->getCardOwner(id) != source)
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
        card->setShowSkill(objectName());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@lue") >= 1;
    }
};

TransformationPackage::TransformationPackage()
    : Package("transformation")
{
    General *lengtong = new General(this, "lengtong", "wu"); // Wu
    lengtong->addSkill(new LieFeng);
    lengtong->addSkill(new Xuanlue);
    //lengtong->addSkill(new XuanlueViewAsSkill);

    addMetaObject<XuanlueCard>();
    addMetaObject<XuanlueequipCard>();

    skills << new XuanlueViewAsSkill;
}

ADD_PACKAGE(Transformation)

