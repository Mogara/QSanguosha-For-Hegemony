#ifndef _ONE_ON_ONE_H
#define _ONE_ON_ONE_H

#include "scenario.h"

class ServerPlayer;

class OneOnOneScenario : public Scenario
{
    Q_OBJECT

public:
    explicit OneOnOneScenario();

    virtual void assign(QStringList &generals, QStringList &generals2, QStringList &kingdoms, Room *room) const;
    virtual int getPlayerCount() const;
    virtual QString getRoles() const;
    virtual void prepareForStart(Room *room, QList<ServerPlayer *> &room_players) const;
};

#endif
