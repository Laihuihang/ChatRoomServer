#ifndef SERVERDEMO_H
#define SERVERDEMO_H

#include <QObject>
#include <QTcpServer>
#include "TextMessage.h"
#include "TextMsgAssembler.h"
#include "textmsghandler.h"
#include <QMap>

class ServerDemo : public QObject
{
    Q_OBJECT
    QTcpServer m_server;
    QMap<QTcpSocket*,TextMsgAssembler*> m_map;
    TextMsgHandler* m_handler;
public:
    ServerDemo(QObject* parent = nullptr);
    bool start(quint16 port);
    void stop();
    void setHandler(TextMsgHandler* handler);
    ~ServerDemo();
protected slots:
    void onNewConnection();

    void onConnected();
    void onDisconnected();
    void onDataReady();
    void onBytesWritten(qint64 bytes);
};

#endif // SERVERDEMO_H
