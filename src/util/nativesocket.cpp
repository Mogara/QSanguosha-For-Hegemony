#include "nativesocket.h"
#include "settings.h"
#include "clientplayer.h"

#include <QTcpSocket>
#include <QRegExp>
#include <QStringList>
#include <QUdpSocket>

NativeServerSocket::NativeServerSocket() {
    server = new QTcpServer(this);
    daemon = NULL;
    connect(server, SIGNAL(newConnection()), this, SLOT(processNewConnection()));
}

bool NativeServerSocket::listen() {
    return server->listen(QHostAddress::Any, Config.ServerPort);
}

void NativeServerSocket::daemonize() {
    daemon = new QUdpSocket(this);
    daemon->bind(Config.ServerPort, QUdpSocket::ShareAddress);
    connect(daemon, SIGNAL(readyRead()), this, SLOT(processNewDatagram()));
}

void NativeServerSocket::processNewDatagram() {
    while (daemon->hasPendingDatagrams()) {
        QHostAddress from;
        char ask_str[256];

        daemon->readDatagram(ask_str, sizeof(ask_str), &from);

        QByteArray data = Config.ServerName.toUtf8();
        daemon->writeDatagram(data, from, Config.DetectorPort);
        daemon->flush();
    }
}

void NativeServerSocket::processNewConnection() {
    QTcpSocket *socket = server->nextPendingConnection();
    NativeClientSocket *connection = new NativeClientSocket(socket);
    emit new_connection(connection);
}

// ---------------------------------

NativeClientSocket::NativeClientSocket()
    : socket(new QTcpSocket(this))
{
    init();
}

NativeClientSocket::NativeClientSocket(QTcpSocket *socket)
    : socket(socket)
{
    socket->setParent(this);
    init();
}

void NativeClientSocket::init() {
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(getMessage()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(raiseError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
}

void NativeClientSocket::connectToHost() {
    QString address = "127.0.0.1";
    ushort port = 9527u;

    if (Config.HostAddress.contains(QChar(':'))) {
        QStringList texts = Config.HostAddress.split(QChar(':'));
        address = texts.value(0);
        port = texts.value(1).toUShort();
    } else {
        address = Config.HostAddress;
        if (address == "127.0.0.1")
            port = Config.value("ServerPort", "9527").toString().toUShort();
    }

    socket->connectToHost(address, port);
}

void NativeClientSocket::getMessage() {
    while (socket->canReadLine()) {
        buffer_t msg;
        socket->readLine(msg, sizeof(msg));
#ifndef QT_NO_DEBUG
        printf("recv: %s", msg);
#endif
        emit message_got(msg);
    }
}

void NativeClientSocket::disconnectFromHost() {
    socket->disconnectFromHost();
}

void NativeClientSocket::send(const QString &message) {
    socket->write(message.toUtf8());
    socket->write("\n");
#ifndef QT_NO_DEBUG
    printf(": %s\n", message.toUtf8().constData());
#endif
    socket->flush();
}

bool NativeClientSocket::isConnected() const{
    return socket->state() == QTcpSocket::ConnectedState;
}

QString NativeClientSocket::peerName() const{
    QString peer_name = socket->peerName();
    if (peer_name.isEmpty())
        peer_name = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

    return peer_name;
}

QString NativeClientSocket::peerAddress() const{
    return socket->peerAddress().toString();
}

void NativeClientSocket::raiseError(QAbstractSocket::SocketError socket_error) {
    // translate error message
    QString reason;
    switch (socket_error) {
    case QAbstractSocket::ConnectionRefusedError:
        reason = tr("Connection was refused or timeout"); break;
    case QAbstractSocket::RemoteHostClosedError:{
        if (Self && Self->hasFlag("is_kicked"))
            reason = tr("You are kicked from server");
        else
            reason = tr("Remote host close this connection");

        break;
    }
    case QAbstractSocket::HostNotFoundError:
        reason = tr("Host not found"); break;
    case QAbstractSocket::SocketAccessError:
        reason = tr("Socket access error"); break;
    case QAbstractSocket::NetworkError:
        return; // this error is ignored ...
    default: reason = tr("Unknow error"); break;
    }

    emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(socket_error).arg(reason));
}

