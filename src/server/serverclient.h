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

#ifndef _SERVER_CLIENT_H
#define _SERVER_CLIENT_H

class Room;
class Recorder;

#include "structs.h"
#include "player.h"
#include "socket.h"
#include "serverplayer.h"
#include "protocol.h"
#include "namespace.h"

#include <QSemaphore>
#include <QDateTime>


class ServerClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ip READ getIp)

public:
    explicit ServerClient(Room *room, const QString &name);
    ~ServerClient();

    void setSocket(ClientSocket *socket);
    inline ClientSocket *getSocket()
    {
        return socket;
    }
    void unicast(const QSanProtocol::AbstractPacket *packet);
    void notify(QSanProtocol::CommandType type, const QVariant &arg = QVariant());
    void kick();
    void startRecord();
    void saveRecord(const QString &filename);
    QString reportHeader() const;
    void unicast(const QByteArray &message);
    Room *getRoom() const;

    QString getIp() const;
    bool isOwner() const;
    void setOwner(bool owner);

    void setPlayer(ServerPlayer *player);
    inline QList<ServerPlayer *> getPlayers()
    {
        return m_players;
    }

    inline void setState(QString state)
    {
        this->state = state;
    }
    inline QString getState() const
    {
        return state;
    }
    inline bool isOnline() const
    {
        return state == "online";
    }
    inline bool isOffline() const
    {
        return getState() == "offline";
    }

    void startNetworkDelayTest();
    qint64 endNetworkDelayTest();

    //Synchronization helpers
    enum SemaphoreType
    {
        SEMA_MUTEX, // used to protect mutex access to member variables
        SEMA_COMMAND_INTERACTIVE // used to wait for response from client
    };
    inline QSemaphore *getSemaphore(SemaphoreType type)
    {
        return semas[type];
    }
    inline void acquireLock(SemaphoreType type)
    {
        semas[type]->acquire();
    }
    inline bool tryAcquireLock(SemaphoreType type, int timeout = 0)
    {
        return semas[type]->tryAcquire(1, timeout);
    }
    inline void releaseLock(SemaphoreType type)
    {
        semas[type]->release();
    }
    inline void drainLock(SemaphoreType type)
    {
        while (semas[type]->tryAcquire()) {
        }
    }
    inline void drainAllLocks()
    {
        for (int i = 0; i < S_NUM_SEMAPHORES; i++) {
            drainLock((SemaphoreType)i);
        }
    }
    inline const QVariant &getClientReply() const
    {
        return _m_clientResponse;
    }

    void setClientReply(const QVariant &val);
    unsigned int m_expectedReplySerial; // Suggest the acceptable serial number of an expected response.
    bool m_isClientResponseReady; //Suggest whether a valid player's reponse has been received.
    bool m_isWaitingReply; // Suggest if the server player is waiting for client's response.
    QVariant m_cheatArgs; // Store the cheat code received from client.
    QSanProtocol::CommandType m_expectedReplyCommand; // Store the command to be sent to the client.
    QVariant m_commandArgs; // Store the command args to be sent to the client.


    void notifyPreshow();


    bool event_received;

    void changeToLord();

protected:
    //Synchronization helpers
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;

private:
    ClientSocket *socket;
    Room *room;
    QList<ServerPlayer *> m_players;
    Recorder *recorder;
    int _m_phases_index;
    QDateTime test_time;
    QVariant _m_clientResponse;
    QString state;
    bool owner;

private slots:
    void getMessage(QByteArray request);
    void sendMessage(const QByteArray &message);

signals:
    void disconnected();
    void request_got(const QByteArray &request);
    void message_ready(const QByteArray &msg);

    void roomPacketReceived(const QSanProtocol::Packet &packet);
    void lobbyPacketReceived(const QSanProtocol::Packet &packet);
    void invalidPacketReceived(const QByteArray &message);
};

#endif
