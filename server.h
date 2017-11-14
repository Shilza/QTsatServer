#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <thread>
#include <chrono>
#include <memory>
#include <QtSql/QSql>
#include <QtSql/QSqlQuery>
#include <QVector>
#include <QDateTime>
#include <QUdpSocket>
#include <QStringList>
#include <unordered_map>
#include <QCryptographicHash>
#include "def.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = 0);
    void start();

private:
    class Session;
    QUdpSocket *socket, *systemSocket;
    QVector<std::shared_ptr<Session>> sessions;
    QVector<quint32> answers;
    QString check(QByteArray sessionKey);
    std::unordered_map<std::string, std::string> registrationQueue;
    std::unordered_map<std::string, std::string> recoveryQueue;
    bool findInAnswers(int i);

signals:
    void isReceived(QByteArray message);
    void handshakeReceived(QStringList list, QHostAddress ip, quint16 port);
    void registrationReceived(QStringList list, QHostAddress ip, quint16 port);
    void recoveryReceived(QStringList list, QHostAddress ip, quint16 port);
    void existNicknameReceived(QString nickname, QHostAddress ip, quint16 port);
    void existEmailReceived(QString email, QHostAddress ip, quint16 port);
    void registrationCodeReceived(QStringList list, QHostAddress ip, quint16 port);
    void recoveryCodeReceived(QStringList list, QHostAddress ip, quint16 port);
    void recoveryNewPassReceived(QStringList list, QHostAddress ip, quint16 port);
    void systemReceived(QByteArray index);

public slots:
    void sendReceived(QByteArray message);
    void read();
    void handshake(QStringList list, QHostAddress peer, quint16 port);
    void registration(QStringList list, QHostAddress ip, quint16 port);
    void registrationCode(QStringList list, QHostAddress ip, quint16 port);
    void recovery(QStringList list, QHostAddress ip, quint16 port);
    void recoveryCode(QStringList list, QHostAddress ip, quint16 port);
    void recoveryNewPass(QStringList list, QHostAddress ip, quint16 port);
    void checkingNickname(QString nickname, QHostAddress peer, quint16 port);
    void checkingEmail(QString email, QHostAddress peer, quint16 port);
    void answersChecker(QByteArray index);
    void systemReading();
};

#endif // UDPSERVER_H
