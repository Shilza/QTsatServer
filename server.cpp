#include "server.h"
using std::shared_ptr;

class Server::Session{
public:
    unsigned int time;
    QByteArray sessionKey;
    QString nickname;
    QHostAddress IP;

    Session(QString nickname, QHostAddress IP){
        this->nickname = nickname;
        this->IP = IP;
        time=QDateTime::currentDateTime().toTime_t();
        sessionKey = QCryptographicHash::hash(nickname.toUtf8() + QByteArray::number(time), QCryptographicHash::Md5).toHex();
    }
};


Server::Server(QObject *parent) :
    QObject(parent)
{
    sessions.clear();

    socket = new QUdpSocket(this);
    systemSocket = new QUdpSocket(this);

    socket->bind(QHostAddress::Any, 49001);
    systemSocket->bind(QHostAddress::Any, 49003);

    connect(socket, SIGNAL(readyRead()), this, SLOT(read()));
    connect(systemSocket, SIGNAL(readyRead()), this, SLOT(systemReading()));
    connect(this, SIGNAL(isReceived(QByteArray)), this, SLOT(sendReceived(QByteArray)));
    connect(this, SIGNAL(handshakeReceived(QStringList, QHostAddress, quint16)), this, SLOT(handshake(QStringList,QHostAddress,quint16)));
    connect(this, SIGNAL(registrationReceived(QStringList,QHostAddress,quint16)), this, SLOT(registration(QStringList,QHostAddress,quint16)));
    connect(this, SIGNAL(registrationCodeReceived(QStringList,QHostAddress,quint16)), this, SLOT(registrationCode(QStringList,QHostAddress,quint16)));
    connect(this, SIGNAL(recoveryReceived(QStringList,QHostAddress,quint16)), this,SLOT(recovery(QStringList,QHostAddress,quint16)));
    connect(this, SIGNAL(recoveryCodeReceived(QStringList,QHostAddress,quint16)), this, SLOT(recoveryCode(QStringList,QHostAddress,quint16)));
    connect(this, SIGNAL(recoveryNewPassReceived(QString,QHostAddress,quint16)), this, SLOT(recoveryNewPass(QString,QHostAddress,quint16)));
    connect(this, SIGNAL(existNicknameReceived(QString,QHostAddress,quint16)), this, SLOT(checkingNickname(QString,QHostAddress,quint16)));
    connect(this, SIGNAL(existEmailReceived(QString,QHostAddress,quint16)), this, SLOT(checkingEmail(QString,QHostAddress,quint16)));
    connect(this, SIGNAL(systemReceived(QByteArray)), this, SLOT(answersChecker(QByteArray)));

}

void Server::start(){
    std::thread sessionsCheckerThread([&](){
        while(true){
            answers.clear();
            unsigned int time=QDateTime::currentDateTime().toTime_t();
            for(int i=0; i<sessions.size(); i++)
                if(time > sessions[i].get()->time+10){
                    systemSocket->writeDatagram(QByteArray::number(i), sessions[i].get()->IP, 49002);
                }
            std::this_thread::sleep_for(std::chrono::seconds(2));

            for(int i=0; i<sessions.size(); i++)
                if(!findInAnswers(i) && time > sessions[i].get()->time+10)
                    sessions.erase(sessions.begin()+i);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
    sessionsCheckerThread.detach();
}

QString Server::check(QByteArray sessionKey){
    for(int i=0; i<sessions.size(); i++)
        if(sessions[i].get()->sessionKey == sessionKey)
            return sessions[i].get()->nickname;
    return "";
}

bool Server::findInAnswers(int i){
    for(int j=0; j<answers.size(); j++)
        if(answers[j] == i){
            answers.erase(answers.begin()+j);
            return true;
        }
    return false;
}

void Server::sendReceived(QByteArray message){
    QStringList list = QString(message).split('|');
    QString nickname = check((list.at(0)).toUtf8());
    list.pop_front();

    if(nickname != ""){
        QString finalMessage;
        while(list.size()){
            finalMessage += '|' + list.front();
            list.pop_front();
        }

        for(int i=0; i<sessions.size(); i++)
            socket->writeDatagram(nickname.toUtf8() + finalMessage.toUtf8(), sessions[i].get()->IP, 49000);

        finalMessage.remove(0,1);
        QSqlQuery query;
        query.prepare("INSERT INTO messages (Sender, Text, Time) VALUES (:sender, :text, :time)");
        query.bindValue(":sender", nickname);
        query.bindValue(":text", finalMessage);
        query.bindValue(":time", QDateTime::currentDateTime().toTime_t());
        query.exec();
    }
}

void Server::read()
{
    QByteArray message;
    message.resize(socket->pendingDatagramSize());
    socket->readDatagram(message.data(), message.size());
    emit isReceived(message);
}

void Server::handshake(QStringList list, QHostAddress peer, quint16 port){
    for(int i=0; i<sessions.size(); i++)
        if(list.at(1) == sessions.at(i).get()->nickname){
            systemSocket->writeDatagram(sessions[i].get()->sessionKey, peer, port);
            return;
        }

    QSqlQuery query;
    query.prepare("SELECT ID FROM users WHERE Nickname=? AND Password=?");
    query.bindValue(0, list.at(1));
    query.bindValue(1, list.at(2));
    query.exec();

    QString id="";
    while (query.next())
        id = query.value(0).toString();

    if(id != ""){
        sessions.push_back(std::make_shared<Session>(Session(list.at(1), peer)));
        systemSocket->writeDatagram(sessions[sessions.size()-1].get()->sessionKey, peer, port);
    }
    else
        systemSocket->writeDatagram(QByteArray().append(ERROR_AUTH), peer, port);
}

void Server::registration(QStringList list, QHostAddress ip, quint16 port)
{
    QSqlQuery queryExist;
    queryExist.prepare("SELECT ID FROM users WHERE Email=? OR Nickname=?");
    queryExist.bindValue(0, list.at(1));
    queryExist.bindValue(1, list.at(2));
    queryExist.exec();

    QString id="";
    while (queryExist.next())
        id = queryExist.value(0).toString();

    if(id==""){
        QSqlQuery query;
        query.prepare("INSERT INTO users (Email, Nickname, Password, Date) VALUES (:email, :nickname, :password, :date)");
        query.bindValue(":email", list.at(1));
        query.bindValue(":nickname", list.at(2));
        query.bindValue(":password", list.at(3));
        query.bindValue(":date", QDateTime::currentDateTime().toTime_t());
        query.exec();
    }
}

void Server::registrationCode(QStringList list, QHostAddress ip, quint16 port){

}

void Server::recovery(QStringList list, QHostAddress ip, quint16 port)
{
    QSqlQuery query;
    query.prepare("SELECT Email FROM users WHERE Email=? OR Nickname=?");
    query.bindValue(0, list.at(1));
    query.bindValue(1, list.at(1));
    query.exec();

    QString email="";
    while (query.next())
        email = query.value(0).toString();

    if(email!=""){
        //DO
        systemSocket->writeDatagram(QByteArray().append(RECOVERY_FOUND), ip, port);
    }
    else{
        systemSocket->writeDatagram(QByteArray().append(RECOVERY_NOT_FOUND), ip, port);
    }
}

void Server::recoveryCode(QStringList list, QHostAddress ip, quint16 port){

}

void Server::recoveryNewPass(QString pass, QHostAddress ip, quint16 port){

}

void Server::checkingNickname(QString nickname, QHostAddress peer, quint16 port){
    QSqlQuery query;
    query.prepare("SELECT ID FROM users WHERE Nickname=?");
    query.bindValue(0, nickname);

    query.exec();

    QString id="";
    while (query.next())
        id = query.value(0).toString();

    if(id != "")
        systemSocket->writeDatagram(QByteArray().append(NICKNAME_EXIST), peer, port);
    else
        systemSocket->writeDatagram(QByteArray().append(NICKNAME_NOT_EXIST), peer, port);
}

void Server::checkingEmail(QString email, QHostAddress peer, quint16 port)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM users WHERE Email=?");
    query.bindValue(0, email);

    query.exec();

    QString strEmail="";
    while (query.next())
        strEmail = query.value(0).toString();

    if(strEmail != "")
        systemSocket->writeDatagram(QByteArray().append(EMAIL_EXIST), peer, port);
    else
        systemSocket->writeDatagram(QByteArray().append(EMAIL_NOT_EXIST), peer, port);
}

void Server::answersChecker(QByteArray index){
    answers.push_back(index.toShort());
}

void Server::systemReading(){
    QByteArray buffer;
    quint16 port;
    QHostAddress peer;

    buffer.resize(systemSocket->pendingDatagramSize());
    systemSocket->readDatagram(buffer.data(), buffer.size(), &peer, &port);

    QStringList list = QString(buffer).split('|');

    if(list.at(0)==HANDSHAKE)
        emit handshakeReceived(list, peer, port);
    else if(list.at(0)==REGISTRATION)
        emit registrationReceived(list, peer, port);
    else if(list.at(0)==RECOVERY)
        emit recoveryReceived(list, peer, port);
    else if(list.at(0)==DOES_EXIST_NICKNAME)
        emit existNicknameReceived(list.at(1), peer, port);
    else if(list.at(0)==DOES_EXIST_EMAIL)
        emit existEmailReceived(list.at(1), peer, port);
    else if(list.at(0)==REGISTRATION_CODE)
        emit registrationCodeReceived(list, peer, port);
    else if(list.at(0)==RECOVERY_CODE)
        emit recoveryCodeReceived(list, peer, port);
    else if(list.at(0)==RECOVERY_NEW_PASS)
        emit recoveryNewPassReceived(list.at(1), peer, port);
    else
        emit systemReceived(buffer);
}
