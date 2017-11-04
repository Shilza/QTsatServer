#include <QtSql/QSqlDatabase>
#include <QCoreApplication>
#include "server.h"
#include "def.h"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

    db.setHostName("localhost");
    db.setDatabaseName("Tsat");
    db.setUserName(DATABASE_USER);
    db.setPassword(DATABASE_PASSWORD);
    if(!db.open())
        return 1;

    Server server;
    server.start();
    return application.exec();
}
