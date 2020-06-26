#include "ServerHandler.h"
#include <QDebug>
#include "Definition.h"
ServerHandler::ServerHandler()
{
    /* 构造函数中建立字符串到函数指针的映射 */
    m_handlerMap.insert("CONN", &ServerHandler::CONN_Handler);
    m_handlerMap.insert("DSCN", &ServerHandler::DSCN_Handler);
    m_handlerMap.insert("LGIN", &ServerHandler::LGIN_Handler);
    m_handlerMap.insert("MSGA", &ServerHandler::MSGA_Handler);
    m_handlerMap.insert("MSGP", &ServerHandler::MSGP_Handler);
    m_handlerMap.insert("ADMN", &ServerHandler::ADMN_Handler);
    m_handlerMap.insert("REGI", &ServerHandler::REGI_Handler);
}
void ServerHandler::handle(QTcpSocket& obj, TextMessage& message)
{
    /* 判断消息类型是否在表中 */
    if(m_handlerMap.contains(message.type()))
    {
        MSGHandler handler = m_handlerMap.value(message.type());
        (this->*handler)(obj, message);
    }
}
QString ServerHandler::getOnlineUserid()
{
    QString ret = "";

    for(int i=0; i<m_NodeList.size(); i++)
    {
        Node* n = m_NodeList.at(i);

        if(n->socket != nullptr)
        {
            ret += n->id + '\r';
        }
    }

    return ret;
}
void ServerHandler::sendToAllOnlineUser(TextMessage& message)
{
    for(int i=0; i<m_NodeList.size(); i++)
    {
        Node* n = m_NodeList.at(i);
        if(n->socket)
        {
            n->socket->write(message.serialize());
        }
    }
}
void ServerHandler::CONN_Handler(QTcpSocket& obj, TextMessage&)
{
    for(int i=0; i<m_NodeList.size(); i++)
    {
        Node* n = m_NodeList.at(i);
        if(n->socket == &obj)
        {
            n->socket = nullptr;
            break;
        }
    }
}
void ServerHandler::DSCN_Handler(QTcpSocket& obj, TextMessage&)
{
    Node* n = nullptr;
    for(int i=0; i<m_NodeList.size(); i++)
    {
        n = m_NodeList.at(i);
        /* 在链表中找到登录得Node */
        if(n->socket == &obj)
        {
            TextMessage tm("MSGA"," [ 系统消息 ]\n ========== [ " + n->id + " ] 退出聊天室 ==========\n");
            sendToAllOnlineUser(tm);
            n->socket = nullptr;
            m_NodeList.removeAt(i);
            delete n;
            break;
        }
    }

    TextMessage tm("USER", getOnlineUserid());
    sendToAllOnlineUser(tm);
}
void ServerHandler::LGIN_Handler(QTcpSocket& obj, TextMessage& message)
{
    QString data = message.data();
    /* 找到用户名和密码隔开的字符串 */
    int index = data.indexOf('\r');
    QString id = data.mid(0,index);
    QString pwd = data.mid(index+1);
    QString result = "";
    QString status = "";
    QString level = "";

    auto res = 0;
    res = SearchUser(id, pwd, status, level);
    /* 账号密码验证通过 */
    if(res == SQL_OK)
    {
        qDebug() << "enter in";
        /* 在已经连接的id中找是否有此id */
        index = -1;
        for(int i=0; i<m_NodeList.length(); i++)
        {
            if(id == m_NodeList.at(i)->id)
            {
                index = i;
                break;
            }
        }
        /* 该账号没有连接到服务器 */
        if(index == -1)
        {
            Node* newNode = new Node();
            if(newNode != nullptr)
            {
                newNode->id = id;
                newNode->pwd = pwd;
                newNode->socket = &obj;

                m_NodeList.append(newNode);

                result = "LIOK";
            }
            qDebug() << "LIOK";
            obj.write(TextMessage(result,id + "\r" + status + "\r" + level).serialize());
        }
        /* 次id用户已登录 */
        else
        {
            qDebug() << index;
            result = "LIER";
            obj.write(TextMessage(result,QString::number(LIER_HASLOGIN)).serialize());
        }
    }
    /* 数据库中没有该账号 */
    else if(res == SQL_NO_ACCOUNT)
    {
        qDebug() << "SQL_NO_ACCOUNT";
        result = "LIER";
        obj.write(TextMessage(result,QString::number(LIER_NO_ACCOUNT)).serialize());
    }
    /* 账号密码错误 */
    else if(res == SQL_PWD_ERROR)
    {
        qDebug() << "SQL_PWD_ERROR";
        result = "LIER";
        obj.write(TextMessage(result,QString::number(LIER_PWD_ERROR)).serialize());
    }


    if(result == "LIOK")
    {
        TextMessage user("USER",getOnlineUserid());
        sendToAllOnlineUser(user);

        TextMessage msga("MSGA"," [ 系统消息 ]\n ========== [ " + id + " ] 进入聊天室 ==========\n");

        sendToAllOnlineUser(msga);
    }

}
void ServerHandler::MSGA_Handler(QTcpSocket&, TextMessage& message)
{
    sendToAllOnlineUser(message);
}
void ServerHandler::MSGP_Handler(QTcpSocket&, TextMessage& message)
{
    /* 将数据做分割 */
    QStringList tl = message.data().split("\r", QString::SkipEmptyParts);
    qDebug() << "tl first:" << tl.first() << "tl last:" << tl.last();
    const QByteArray& ba =TextMessage("MSGA",tl.last()).serialize();

    tl.removeLast();
    /* 删除之后tl中的内容就是目标用户id */
    for(int i=0; i<tl.size(); i++)
    {
        qDebug() << tl[i];
        /* 其实遍历太慢。。。可以搞成哈希表 */
        for(int j=0; j<m_NodeList.size(); j++)
        {
            Node* n = m_NodeList.at(j);

            if((tl[i] == n->id) && (n->socket != nullptr))
            {
                n->socket->write(ba);

                break;
            }
        }
    }
}
void ServerHandler::ADMN_Handler(QTcpSocket&, TextMessage& message)
{
    QStringList data = message.data().split("\r");
    QString op = data[0];
    QString id = data[1];

    for (int i = 0; i < m_NodeList.size(); i++)
    {
        Node* n = m_NodeList.at(i);

        if((id == n->id) && (n->socket != nullptr) && (n->level == "user"))
        {
            n->socket->write(TextMessage("CTRL",op).serialize());
            n->status = op;
            break;
        }
    }
}
void ServerHandler::REGI_Handler(QTcpSocket& obj, TextMessage& messgae)
{
    qDebug() << "REGI_Handler enter";
    QStringList data = messgae.data().split("\r");
    QString id = data[0];
    QString pwd = data[1];

    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("User.db");
    /* 打开数据库 */
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
    }
    else
    {
        qDebug() << "Succeed to connect database." ;
    }
    QSqlQuery sql_query;
    /* 插入数据 */
    /* 插入成功 */
    if(sql_query.exec(QString("INSERT INTO User VALUES(\"%1\",\"%2\", \"ok\", \"user\")").arg(id).arg(pwd)))
    {
        obj.write(TextMessage("INSE",QString::number(INSERT_OK)).serialize());
    }
    /* 插入失败 */
    else
    {
        obj.write(TextMessage("INSE",QString::number(INSERT_ERROR)).serialize());
    }
}
int ServerHandler::SearchUser(QString id, QString pwd, QString& status, QString& level)
{
    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("User.db");
    /* 打开数据库 */
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
        return false;
    }
    else
    {
        qDebug() << "Succeed to connect database." ;
    }
    QSqlQuery sql_query;
    /* 创建表格 */
    sql_query.exec("SELECT COUNT(*) FROM sqlite_master where type=\'table\' and name=\'User\'");
    if(sql_query.next())
    {
        if(sql_query.value(0).toInt() == 0)
        {
            qDebug() << "no table";
            if(!sql_query.exec("create table User(id text primary key, pwd text, status text, level text)"))
            {
                qDebug() << "Error: Fail to create table."<< sql_query.lastError();
                database.close();
                return false;
            }
            else
            {
                qDebug() << "Table created!";
            }
        }
    }
    /* 查询数据 */
    QString db_id = "", db_pwd = "";
    sql_query.exec(QString("SELECT id,pwd,status,level FROM User Where id = \"%1\"").arg(id));
    if(sql_query.next())
    {
        db_id = sql_query.value(0).toString();
        db_pwd = sql_query.value(1).toString();
        status = sql_query.value(2).toString();
        level = sql_query.value(3).toString();
    }
    if(db_id == "")
    {
        database.close();
        return SQL_NO_ACCOUNT;
    }
    else if(db_id == id && db_pwd != pwd)
    {
        database.close();
        return SQL_PWD_ERROR;
    }
    else if(db_id == id && db_pwd == pwd)
    {
        database.close();
        return SQL_OK;
    }
}
