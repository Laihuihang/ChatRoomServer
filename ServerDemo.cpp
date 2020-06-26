#include "ServerDemo.h"
#include <QHostAddress>
#include <QDebug>
#include <QTcpSocket>
#include <QObjectList>

ServerDemo::ServerDemo(QObject* parent) : QObject (parent) , m_handler(nullptr)
{
    connect(&m_server,SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}
bool ServerDemo::start(quint16 port)
{
    bool ret = true;
    if(!m_server.isListening())
    {
        ret = m_server.listen(QHostAddress("192.168.0.123"),port);
    }
    return ret;
}
void ServerDemo::stop()
{
    if(m_server.isListening())
    {
        m_server.close();
    }
}
void ServerDemo::setHandler(TextMsgHandler* handler)
{
    m_handler = handler;
}
void ServerDemo::onNewConnection()
{
    QTcpSocket* tcp = m_server.nextPendingConnection();
    /* 客户端连接过来了动态得创建一个装配类 */
    TextMsgAssembler* assembler = new TextMsgAssembler();
    m_map.insert(tcp,assembler);
    connect(tcp, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(tcp, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(tcp, SIGNAL(readyRead()), this, SLOT(onDataReady()));
    connect(tcp, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));

    if(m_handler != nullptr)
    {
        TextMessage msg("CONN",tcp->peerAddress().toString() + ":" + QString::number(tcp->peerPort()));
        m_handler->handle(*tcp,msg);
    }
}


void ServerDemo::onConnected()
{
    /* 通过sender()函数获取到现在连接上的对象 */
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    if(tcp != nullptr)
    {
        qDebug() << "onConnected()";
        qDebug() << "local address" << tcp->localAddress();
        qDebug() << "local port" << tcp->localPort();
    }
}
void ServerDemo::onDisconnected()
{
    /* 获取是哪一个客户端断开了 */
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    if(tcp != nullptr)
    {
        /* 剔除tcp对应的value */
        delete m_map.take(tcp);

        if(m_handler != nullptr)
        {
            TextMessage msg("DSCN","");
            m_handler->handle(*tcp,msg);
        }
    }
}
/* 读取数据 */
void ServerDemo::onDataReady()
{
    QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(sender());
    char buf[256] = {0};
    int len = 0;

    if(tcp != nullptr)
    {
        /* 取出tcp对应的装配类对象 */
        TextMsgAssembler* m_assembler = m_map.value(tcp);
        while((len = tcp->read(buf,sizeof(buf))) > 0)
        {
            if(m_assembler != nullptr)
            {
                QSharedPointer<TextMessage> ptm = nullptr;

                m_assembler->prepare(buf, len);

                while((ptm = m_assembler->assemble()) != nullptr)
                {
                    if(m_handler != nullptr)
                    {
                        m_handler->handle(*tcp, *ptm);
                    }
                }
            }
        }
    }
}
void ServerDemo::onBytesWritten(qint64 bytes)
{
    qDebug() << "onBytesWritten" << bytes;
}
ServerDemo::~ServerDemo()
{
    /* 关闭所有TCPSocket对象 */
    const QObjectList& list = m_server.children();
    for (int i=0; i<list.size(); i++)
    {
        QTcpSocket* tcp = dynamic_cast<QTcpSocket*>(list[i]);
        if(tcp != nullptr)
        {
            tcp->close();
        }
    }

    /* 析构所有的装配类对象 */
    const QList<TextMsgAssembler*> al = m_map.values();
    for(int i=0; i<al.length(); i++)
    {
        delete al[i];
    }
}
