#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "ThreadPool.h"
#include "EventLoop.h"

namespace base
{
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
                int taskPoolSize,
                int ioPoolSize,
                std::string port,
                onConnection    onConnectionFunc,
                onMessage       onMessageFunc,
                onWriteComplete onWriteCompleteFunc);
        ~TcpServer();

        void start();
        void stop();

        void createNewTcpConnection(int connfd,struct sockaddr_in peeraddr);
        void cleanTcpConnection();

        void* entryIOThread(void *Data);

        void createEventLoopThreadPool();
        void stopEventLoopThreadPool();

        /// 可跨线程调用
        void addClean(std::string name);

    private:
        /// 不可跨线程调用
        void acceptNewConnection();

    private:
        in_addr_t       ip_;       // 监听的ip，默认为 0.0.0.0，即监听所有源ip
        std::string     port_;     // 监听port

        bool running_;

        int  listenfd_; // 监听socket
        int  epollfd_;   // 监听用的epoll实例
        int  idlefd_;   // 占一个位置，以防fd耗尽

        int eventsListInitSize_; // 初始化events_的大小，如果不够会成倍扩展
        std::vector<struct epoll_event> events_; // epoll返回发生的事件结构体

        ThreadPool taskPool_; // 任务处理线程池

        int nextEventLoop_;
        int ioThreadsNum_; // IO线程数量
        std::vector<std::unique_ptr<Thread>>    ioThreads_;  // IO线程对象列表
        std::vector<std::shared_ptr<EventLoop>> eventLoops_; // EventLoop对象列表
        pthread_mutex_t eventLoopsMutex_;   // EventLoop对象列表的互斥锁

        std::unordered_map<std::string,std::shared_ptr<TcpConnection>> connections_; // TcpConnection列表，name-pointer的K-V对
        std::vector<std::string> cleans_; // 需要clean的TcpConnection的name
        pthread_mutex_t cleanMutex_;   // clean队列的互斥锁

        onConnection    onConnection_;
        onMessage       onMessage_;
        onWriteComplete onWriteComplete_;
    };
}

#endif //_TCPSERVER_H
