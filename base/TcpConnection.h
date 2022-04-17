#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "noncopyable.h"
#include "Types.h"

namespace base
{
    class TcpConnection:noncopyable
    {
    public:
        /// 可跨线程调用

        /// 不可跨线程调用
        explicit TcpConnection(std::string connectionName,
                               int connfd,
                               struct sockaddr_in peeraddr,
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

    private:
        pid_t threadId_; // 所属IO线程的线程ID

        std::string        name_;     // 每个TcpConnection对象的唯一标识符，格式为：Tcp[xxxxxx……]（[]中为64位的时间字符串，以us为单位）
        int                socketfd_; // 连接对应的socket文件描述符
        struct sockaddr_in peeraddr_;

        onConnection       onConnection_;     // 连接建立、断开回调
        onMessage          onMessage_;        // 收到消息回调，去往TcpServer更上层的回调
        onWriteComplete    onWriteComplete_;  // 发送完毕回调
        onCleanTcpSever    onCleanTcpServer_; // Tcp连接关闭时，清理TcpServer::connections_的回调
        onCleanEventLoop   onCleanEventLoop_; // Tcp连接关闭时，清理EventLoop::connections_的回调，并取消监听
    };


}

#endif //TCPCONNECTION_H