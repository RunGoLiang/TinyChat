#include "TcpConnection.h"

using namespace base;

/*
 *  构造函数
 */
TcpConnection::TcpConnection(
        std::string connectionName,
        int connfd,
        struct sockaddr_in peeraddr,
        onConnection onConnectionFunc,
        onMessage onMessageFunc,
        onWriteComplete onWriteCompleteFunc,
        onCleanTcpSever onCleanTcpSeverFunc)
        :
        name_(connectionName),
        socketfd_(connfd),
        peeraddr_(peeraddr),
        onConnection_(onConnectionFunc),
        onMessage_(onMessageFunc),
        onWriteComplete_(onWriteCompleteFunc),
        onCleanTcpServer_(onCleanTcpSeverFunc)
{

}

/*
 *  析构函数
 */
TcpConnection::~TcpConnection()
{

}

/*
 *  可读事件回调
 */
void TcpConnection::handleRead()
{


    char buf[1024] = {0};
    int recvBytes = read(socketfd_, buf, sizeof(buf));

    if(recvBytes > 0) // 收到数据
    {
        std::string recvMessage = buf;
        onMessage_(std::move(recvMessage),peeraddr_);
    }
    else if(recvBytes == 0) // 对等方关闭连接
    {
        handleClose();
    }
    else // 发生错误
    {
        handleError();
    }
}

/*
 *  可写事件回调
 */
void TcpConnection::handleWrite()
{

}

/*
 *  关闭当前连接
 */
void TcpConnection::handleClose()
{
    onConnection_((void *)&peeraddr_); // 用户设定的连接建立、断开的回调

    // 清理TcpServer，调用TcpServer::addClean()
    onCleanTcpServer_(name_);

    // 清理EventLoop，取消监听，调用EventLoop::addClean()
    onCleanEventLoop_(socketfd_);

}

/*
 *  epoll时出现问题回调
 */
void TcpConnection::handleError()
{

}