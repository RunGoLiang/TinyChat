#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "noncopyable.h"
#include "Buffer.h"
#include "ThreadPool.h"

namespace base
{
    class EventLoop;

    /*
     *  保存一个Tcp连接
     *  生命周期由shared_ptr控制，正常运作时保留2份指向其对象的引用计数：TcpServer和EventLoop的connections_各有一份。
     *
     *  断开连接时，首先在loop()中curConnection_临时保存一份引用（引用剩余3），
     *  然后调用TcpConnection::handleRead()，再跳到TcpConnection::handleClose()；
     *  在handleClose()中，先调用TcpServer::addClean()告知server从其connections_中清理掉一份引用（引用剩余2）；
     *  然后调用EventLoop::addClean()，告知eventloop从其connections_中清理掉一份引用，并取消监听（引用剩余1）；
     *  最后回到loop()中，curConnection_改变指向，保存的引用消失（引用剩余0）。
     *  由于“server清理”和“curConnection_改变指向”位于不同线程，因此是两者中后发生者析构了TcpConnection对象。
     *
     *  还有可能任务列表中还有task函数bind了TcpConnection的shared_ptr，这会导致无法马上销毁TcpConnection对象
     * */
    class TcpConnection : noncopyable,
                            public std::enable_shared_from_this<TcpConnection>
    {
    public:
        /// 不可跨线程调用
        explicit TcpConnection(std::string connectionName,
                               int connfd,
                               struct sockaddr_in peeraddr,
                               std::shared_ptr<ThreadPool> taskPool,
                               std::shared_ptr<EventLoop> eventLoop,
                               onConnection    onConnectionFunc,
                               onMessage       onMessageFunc,
                               onWriteComplete onWriteCompleteFunc,
                               onCleanTcpSever onCleanTcpSeverFunc);
        ~TcpConnection();

        std::string getName(){ return name_; }
        int getFd(){ return socketfd_; }

        void handleRead();
        void handleWrite();
        void handleClose();
        void handleError();

        void setTid(pid_t tid){ threadId_ = tid; }
        void setonCleanEventLoop(onCleanEventLoop func){ onCleanEventLoop_ = func; }

        void addTaskToPool(Task func){ taskPool_->addTask(func); }

        void sendInLoop(std::string message);

        /// 可跨线程调用
        void send(std::string message);

    private:
        bool connected_;

        pid_t threadId_; // 所属IO线程的线程ID

        std::string        name_;     // 每个TcpConnection对象的唯一标识符，格式为：Tcp[xxxxxx……]（[]中为64位的时间字符串，以us为单位）
        int                socketfd_; // 连接对应的socket文件描述符
        struct sockaddr_in peeraddr_;

        Buffer inputBuffer_,outputBuffer_;      // 应用层缓冲区，内部数据以网络字节序存放
        std::shared_ptr<ThreadPool> taskPool_;  // TcpServer拥有的任务处理线程池
        std::shared_ptr<EventLoop>  eventLoop_; // 所属的EventLoop对象

        onConnection       onConnection_;     // 连接建立、断开回调
        onMessage          onMessage_;        // 收到消息回调，去往TcpServer更上层的回调
        onWriteComplete    onWriteComplete_;  // 发送完毕回调
        onCleanTcpSever    onCleanTcpServer_; // Tcp连接关闭时，清理TcpServer::connections_的回调
        onCleanEventLoop   onCleanEventLoop_; // Tcp连接关闭时，清理EventLoop::connections_的回调，并取消监听
    };
}

#endif //TCPCONNECTION_H