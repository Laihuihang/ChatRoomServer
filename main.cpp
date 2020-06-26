#include <QCoreApplication>
#include "ServerDemo.h"
#include "ServerHandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ServerHandler handler;
    ServerDemo server;

    server.setHandler(&handler);
    server.start(11112);

    return a.exec();
}
