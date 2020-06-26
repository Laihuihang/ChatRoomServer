#ifndef SERVERHANDLER_H
#define SERVERHANDLER_H

#include <QTcpSocket>
#include "textmsghandler.h"
#include "TextMessage.h"
#include <QList>
#include <QMap>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class ServerHandler : public TextMsgHandler
{
    /* 定义一个指向类成员函数的函数指针 */
    typedef void(ServerHandler::*MSGHandler)(QTcpSocket&, TextMessage&);

    struct Node
    {
        QString id;
        QString pwd;
        /* 账户状态是否封禁 */
        QString status;
        /* 账户级别 是否管理员 */
        QString level;
        QTcpSocket* socket;
    public:
        Node() : id(""), pwd(""), status("ok"), level("user"), socket(nullptr)
        {

        }
    };

    QList<Node*> m_NodeList;
    /* 字符串到函数指针的映射 */
    QMap<QString, MSGHandler> m_handlerMap;

    QString getOnlineUserid();
    void sendToAllOnlineUser(TextMessage&);

    /* 不同协议头对应的处理方式 */
    void CONN_Handler(QTcpSocket&, TextMessage&);
    void DSCN_Handler(QTcpSocket&, TextMessage&);
    void LGIN_Handler(QTcpSocket&, TextMessage&);
    void MSGA_Handler(QTcpSocket&, TextMessage&);
    void MSGP_Handler(QTcpSocket&, TextMessage&);
    void ADMN_Handler(QTcpSocket&, TextMessage&);
    void REGI_Handler(QTcpSocket&, TextMessage&);

    /* 查找数据库中对应的账号密码信息 */
    int SearchUser(QString id, QString pwd, QString& status, QString& level);
public:
    ServerHandler();
    void handle(QTcpSocket& obj, TextMessage& message);
};

#endif // SERVERHANDLER_H
