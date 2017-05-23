#include "1v1.h"
#include "skill.h"
#include "engine.h"
#include "room.h"
#include "json.h"
#include "banpair.h"

class OneOnOneRule : public ScenarioRule
{
public:
    OneOnOneRule(Scenario *scenario)
        : ScenarioRule(scenario)
    {
        events << GameStart << GameOverJudge;
        priority.insert(GameOverJudge, 5);
        priority.insert(GameStart, -4);
    }

    virtual bool effect(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *, const TriggerStruct &) const
    {
        if (event == GameStart) {
            QStringList selected = room->getTag("1v1_selected").toStringList();
            if (player != NULL) {
                if (selected.contains(player->getActualGeneral1Name()) || selected.contains(player->getActualGeneral1Name().remove("lord_")))
                    player->showGeneral(true);
                if (selected.contains(player->getActualGeneral2Name()))
                    player->showGeneral(false);
            }
        } else {
            if (player && player->isLord()) {
                QStringList winner;
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    if (!p->isFriendWith(player))
                        winner << p->objectName();
                room->gameOver(winner.join("+"));
            }
        }

        return false;
    }
};

OneOnOneScenario::OneOnOneScenario()
    : Scenario("one_on_one")
{
    rule = new OneOnOneRule(this);
    random_seat = false;
}

void OneOnOneScenario::prepareForStart(Room *room, QList<ServerPlayer *> &room_players) const
{
    QList<ServerPlayer *> players;
    foreach (ServerClient *p, room->getClients()) {
        players << p->getPlayers();
    }
    qShuffle(players);
    foreach (ServerPlayer *p, room_players)
        if (!players.contains(p))
            players << p;
    room_players = players;

    if (players.first()->getClient()) {
        room->addMappingPlayer(players.first()->getClient(), players.at(3));
        room->addMappingPlayer(players.first()->getClient(), players.at(4));
    }
    if (players.at(1)->getClient()) {
        room->addMappingPlayer(players.at(1)->getClient(), players.at(2));
        room->addMappingPlayer(players.at(1)->getClient(), players.at(5));
    }
}

void OneOnOneScenario::assign(QStringList &generals, QStringList &generals2, QStringList &kingdoms, Room *room) const
{
    QStringList all_kingdom;
    all_kingdom << "wei" << "shu" << "qun" << "wu";
    qShuffle(all_kingdom);

    QStringList wei_generals, shu_generals;
    foreach (const QString &general, Sanguosha->getLimitedGeneralNames()) {
        if (general.startsWith("lord_")) continue;
        if  (BanPair::isBanned(general)) continue;
        QString kingdom = Sanguosha->getGeneral(general)->getKingdom();
        if (kingdom == all_kingdom[0])
            wei_generals << general;
        else if (kingdom == all_kingdom[1])
            shu_generals << general;
    }
    Q_ASSERT(wei_generals.length() >= 10 && shu_generals.length() >= 10);

    QList<ServerPlayer *> players;
    players << room->getPlayers().first() << room->getPlayers().at(1) ;
    QStringList result = room->doBanPick(players, QStringList() << wei_generals.join("+") << shu_generals.join("+"));
    QStringList bans = result.first().split("+");
    QStringList selected = result.last().split("+");

    QHash<QString, QStringList> kingdom_map;
    kingdom_map[all_kingdom[0]] = wei_generals;
    kingdom_map[all_kingdom[1]] = shu_generals;

    QString last = all_kingdom[0];
    int count = 1;
    foreach (ServerPlayer *player, room->getAllPlayers()) {
        QStringList selects;
        QString king;
        if (count > 0) {
            king = last;
            count = 0;
        } else {
            king = (last == all_kingdom[0] ? all_kingdom[1] : all_kingdom[0]);
            count++;
        }
        last = king;
        kingdoms << king;

        foreach (QString name, selected)
            if (kingdom_map[king].contains(name))
                selects << name;

        for (int i = selects.length(); i < 7; i++) {
            qShuffle(kingdom_map[king]);
            foreach (QString name, kingdom_map[king]) {
                if (!selects.contains(name) && !bans.contains(name)) {
                    selects << name;
                    break;
                }
            }
        }

        JsonArray args;
        args << player->objectName();
        args << "1v1:::" + QString::number(player->getSeat());
        args << JsonUtils::toJsonArray(selects);
        args << false;
        args << false;
        args << true;

        room->tryPause();
        room->notifyMoveFocus(player, QSanProtocol::S_COMMAND_CHOOSE_GENERAL);
        QStringList result;
        room->doRequest(player, QSanProtocol::S_COMMAND_CHOOSE_GENERAL, args, true);

        if (player->getClient()) {
            const QVariant &generalName = player->getClient()->getClientReply();
            if (player->getClient()->m_isClientResponseReady && JsonUtils::isString(generalName))
                result = generalName.toString().split("+");
        }
        if (result.isEmpty()) {
            player->clearSelected();
            foreach (QString name, selects)
                player->addToSelected(name);

            result = room->chooseDefaultGenerals(player, true);
        }

        kingdom_map[king].removeAll(result.first());
        kingdom_map[king].removeAll(result.last());
        generals << result.first();
        generals2 << result.last();

        if (selected.contains(result.first()))
            room->broadcastProperty(player, "general", result.first());
        if (selected.contains(result.last()))
            room->broadcastProperty(player, "general2", result.last());
    }
    room->setTag("1v1_selected", selected);
}

int OneOnOneScenario::getPlayerCount() const
{
    return 6;
}

QString OneOnOneScenario::getRoles() const
{
    return "ZNNN";
}

