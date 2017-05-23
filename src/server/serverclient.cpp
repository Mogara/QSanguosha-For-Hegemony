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

#include "serverclient.h"
#include "engine.h"
#include "standard.h"
#include "ai.h"
#include "settings.h"
#include "recorder.h"
#include "lua-wrapper.h"
#include "json.h"
#include "gamerule.h"
#include "roomthread.h"

using namespace QSanProtocol;

const int ServerClient::S_NUM_SEMAPHORES = 6;

ServerClient::ServerClient(Room *room, const QString &name)
    : QObject(room), m_isClientResponseReady(false), m_isWaitingReply(false),
    event_received(false), socket(NULL), room(room), recorder(NULL),
    _m_phases_index(0), state("online")
{
    setObjectName(name);

    semas = new QSemaphore *[S_NUM_SEMAPHORES];
    for (int i = 0; i < S_NUM_SEMAPHORES; i++)
        semas[i] = new QSemaphore(0);
}

ServerClient::~ServerClient()
{
    for (int i = 0; i < S_NUM_SEMAPHORES; i++)
        delete semas[i];

    delete[] semas;
}

Room *ServerClient::getRoom() const
{
    return room;
}

void ServerClient::setSocket(ClientSocket *socket)
{
    if (socket) {
        connect(socket, &ClientSocket::disconnected, this, &ServerClient::disconnected);
        connect(socket, &ClientSocket::message_got, this, &ServerClient::getMessage);
        connect(this, &ServerClient::message_ready, this, &ServerClient::sendMessage);
    } else {
        if (this->socket) {
            this->disconnect(this->socket);
            this->socket->disconnect(this);
            //this->socket->disconnectFromHost();
            this->socket->deleteLater();
        }

        disconnect(this, &ServerClient::message_ready, this, &ServerClient::sendMessage);
    }

    this->socket = socket;
}

void ServerClient::kick()
{
    foreach (ServerPlayer *p, m_players)
        room->notifyProperty(this, p, "flags", "is_kicked");

    if (socket != NULL)
        socket->disconnectFromHost();
    setSocket(NULL);
}

void ServerClient::getMessage(QByteArray request)
{
    if (request.endsWith('\n'))
        request.chop(1);

    emit request_got(request);

    Packet packet;
    if (packet.parse(request)) {
        switch (packet.getPacketDestination()) {
        case S_DEST_ROOM:
            emit roomPacketReceived(packet);
            break;
            //unused destination. Lobby hasn't been implemented.
        case S_DEST_LOBBY:
            emit lobbyPacketReceived(packet);
            break;
        default:
            emit invalidPacketReceived(request);
        }
    } else {
        emit invalidPacketReceived(request);
    }
}

void ServerClient::unicast(const QByteArray &message)
{
    emit message_ready(message);

    if (recorder)
        recorder->recordLine(message);
}

void ServerClient::startNetworkDelayTest()
{
    test_time = QDateTime::currentDateTime();
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_NETWORK_DELAY_TEST);
    unicast(&packet);
}

qint64 ServerClient::endNetworkDelayTest()
{
    return test_time.msecsTo(QDateTime::currentDateTime());
}

void ServerClient::startRecord()
{
    recorder = new Recorder(this);
}

void ServerClient::saveRecord(const QString &filename)
{
    if (recorder)
        recorder->save(filename);
}

void ServerClient::sendMessage(const QByteArray &message)
{
    if (socket) {
#ifndef QT_NO_DEBUG
        printf("%s", qPrintable(objectName()));
#endif
        socket->send(message);
    }
}

void ServerClient::unicast(const AbstractPacket *packet)
{
    unicast(packet->toJson());
}

void ServerClient::notify(CommandType type, const QVariant &arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, type);
    packet.setMessageBody(arg);
    unicast(packet.toJson());
}

void ServerClient::setClientReply(const QVariant &val)
{
    _m_clientResponse = val;
}

QString ServerClient::reportHeader() const
{
    QString name = objectName();
    return QString("%1 ").arg(name.isEmpty() ? tr("Anonymous") : name);
}

QString ServerClient::getIp() const
{
    if (socket)
        return socket->peerAddress();
    else
        return QString();
}

bool ServerClient::isOwner() const
{
    return owner;
}

void ServerClient::setOwner(bool owner)
{
    if (this->owner != owner) {
        this->owner = owner;
        room->doNotify(this, S_COMMAND_OWNER_CHANGE, owner);
    }
}

void ServerClient::setPlayer(ServerPlayer *player)
{
    if (!m_players.contains(player))
        m_players << player;

    player->setClient(this);
}
