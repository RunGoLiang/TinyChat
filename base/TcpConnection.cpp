#include "TcpConnection.h"
#include "EventLoop.h"

using namespace base;

/*
 *  构造函数
 */
TcpConnection::TcpConnection(
        std::string connectionName,
        int connfd,
        struct sockaddr_in peeraddr,
        std::shared_ptr<ThreadPool> taskPool,
        std::shared_ptr<EventLoop> eventLoop,
        onConnection onConnectionFunc,
        onMessage onMessageFunc,
        onWriteComplete onWriteCompleteFunc,
        onCleanTcpSever onCleanTcpSeverFunc)
        :
        name_(connectionName),
        connected_(true),
        socketfd_(connfd),
        peeraddr_(peeraddr),
        taskPool_(taskPool),
        eventLoop_(eventLoop),
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
    handleClose();
}

/*
 *  可读事件回调
 */
void TcpConnection::handleRead()
{
    int savedErrno = 0;
    ssize_t recvBytes = inputBuffer_.readFd(socketfd_,&savedErrno);

    if(recvBytes > 0) // 收到数据
    {
        onMessage_(shared_from_this(),&inputBuffer_,peeraddr_);
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
 *  主要用于一次发送不完时，需要监听可写事件，然后回调handleWrite()把暂存在output buffer中的数据发出去
 */
void TcpConnection::handleWrite()
{
    if(outputBuffer_.readableBytes() > 0)
    {
        size_t len = write(socketfd_,outputBuffer_.peek(),outputBuffer_.readableBytes());
        size_t remain = outputBuffer_.readableBytes() - len; // 发送完还剩下多少字节
        // 没发完
        if(remain > 0)
        {
            outputBuffer_.retrieve(len);
        }
        // 发完了，取消关注可写事件
        else if(remain == 0)
        {
            outputBuffer_.retrieveAll();
            eventLoop_->disableEpollOut(socketfd_);
            onWriteComplete_(peeraddr_);
        }
        // 出错了
        else // remain < 0
        {
            handleError();
        }
    }
}

/*
 *  关闭当前连接
 *  可用于主动断开连接
 *  干3件事：
 *  1.调用用户设定的onConnection()
 *  2.清理TcpServer
 *  3.清理EventLoop，取消监听
 */
void TcpConnection::handleClose()
{
    if(!connected_)
        return;

    // 标记已经断开，禁止再向对等方发送数据
    connected_ = false;

    // 用户设定的连接建立、断开的回调
    onConnection_((void *)&peeraddr_);

    // 清理TcpServer，调用TcpServer::addClean()
    onCleanTcpServer_(name_);

    // 取消监听可写事件
    eventLoop_->disableEpollOut(socketfd_);

    // 清理EventLoop，取消监听，调用EventLoop::addClean()
    onCleanEventLoop_(socketfd_);

    // 关闭socket
    close(socketfd_);
}

/*
 *  epoll时出现问题回调
 *  关闭连接
 */
void TcpConnection::handleError()
{
    handleClose();
}

/*
 * 发送消息给对等方
 * 任务线程中调用的，需要转调用
 */
void TcpConnection::send(std::string message)
{
    // 如果已经断开连接，就不再向对等方发送数据
    if(!connected_)
        return;

    // shared_from_this()是为了防止一旦待办被执行前TcpConnection对象就被销毁，this就成了野指针，将会出现段错误
    eventLoop_->addPending(std::bind(&TcpConnection::sendInLoop,shared_from_this(),std::move(message)));
    eventLoop_->wakeup();
}

/*
 * 发送消息给对等方
 * IO线程中调用
 */
void TcpConnection::sendInLoop(std::string message)
{
    // 如果是待办未执行前连接已经关闭，就不再发送数据给对等方
    if(!connected_)
        return;

    size_t total = message.size();

    // 如果output中有数据，就不发送，把message附加在后面
    if(outputBuffer_.readableBytes() > 0)
    {
        outputBuffer_.append(message.c_str(),total);
        return;
    }

    // 前面没有未发完的数据，可以直接发送
    size_t len = write(socketfd_,message.c_str(),total);
    if(len >= 0)
    {
        size_t remain = total - len; // 剩余没发送的字节数

        // 这次发送完了，回调onWriteComplete_
        if(remain == 0)
        {
            onWriteComplete_(peeraddr_);
        }
        // 这次没发完
        else if(remain > 0)
        {
            outputBuffer_.append(message.c_str()+len,remain);
            eventLoop_->enableEpollOut(socketfd_); // 关注可写事件
        }
        // 出错了
        else // remain < 0
        {
            handleError();
        }
    }
    else // len < 0
    {
        handleError();
    }
}