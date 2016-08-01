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
#ifndef _TRANSFORMATION_H
#define _TRANSFORMATION_H

#include "package.h"
#include "card.h"
#include "wrappedcard.h"
#include "skill.h"
#include "standard.h"

class XuanlueequipCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XuanlueequipCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class XuanlueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XuanlueCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const;
private:
    bool canPutEquipment(Room *room, QList<int> &cards) const;
};

class DiaoduequipCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DiaoduequipCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DiaoduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DiaoduCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class QiceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiceCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
};

class SanyaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SanyaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TransformationPackage : public Package
{
    Q_OBJECT

public:
    TransformationPackage();
};

#endif

