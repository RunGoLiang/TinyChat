#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <string>
#include <string.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>

#include <functional>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <signal.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>

#include "Thread.h"
#include "noncopyable.h"

namespace base
{
    using onConnection    = std::function<void*(void*)>; // 连接建立、断开回调函数
    using onMessage       = std::function<void*(void*)>; // 消息到来回调函数
    using onWriteComplete = std::function<void*(void*)>; // 消息发送完毕回调函数

    /*
     * Tcp服务器类
     * 利用epoll监听listenfd的连接请求，建立新的连接
     * 利用name-pointer的K-V对保存TcpConnection列表
     * */
    class TcpServer:noncopyable
    {
    public:
        /// 不可跨线程调用
        explicit TcpServer(
                std::string port,
                onConnection    onConnectionFunc,
                onMessage       onMessageFunc,
                onWriteComplete onWriteCompleteFunc);
        ~TcpServer();

        void start();
        void stop();

        void createNewTcpConnection();
        void cleanTcpConnection();

        void createEventLoopThreadPool();
        void stopEventLoopThreadPool();

    private:
        void acceptNewConnection();

    private:
        in_addr_t ip_;
        std::string port_;

        bool running_;
        int  listenfd_; // 监听socket
        int  epollfd_;  // 监听用的epoll实例
        int  idlefd_;   // 占一个位置，以防fd耗尽

        onConnection    onConnection_;
        onMessage       onMessage_;
        onWriteComplete onWriteComplete_;

        int eventsListInitSize_; // 初始化events_的大小，如果不够会成倍扩展
        std::vector<struct epoll_event> events_; // epoll返回发生的事件结构体

//        std::vector<std::unique_ptr<EventLoop>> loops_; // EventLoop列表

//        std::unordered_map<std::string,std::shared_ptr<TcpConnection>> connections_; // TcpConnection列表，name-pointer的K-V对
//        std::queue<std::string> cleanQueue_; // 需要clean的TcpConnection的name
//        pthread_cond_t  cleanCond_;    // clean队列的条件变量
//        pthread_mutex_t cleanMutex_;   // clean队列的互斥锁

    };
}

#endif //_TCPSERVER_H
