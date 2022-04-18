#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "ThreadPool.h"
#include "EventLoop.h"

namespace base
{
    /*
     * Tcp服务器类
     * 利用epoll监听listenfd的连接请求，建立新的连接；
     * 利用name-pointer的K-V对保存TcpConnection列表；
     * 利用unique_ptr保存IO线程对象，shared_ptr保存EventLoop对象；
     * 在创建的IO子线程内部创建EventLoop对象，因此向eventLoops_中存放时是跨线程操作，需要加锁；
     * 清理TcpConnection对象是在IO线程中，跨线程调用TcpServer::addClean()，向cleans_添加要清除的连接name，
     * 等下次处理完连接请求后会执行TcpServer::cleanTcpConnection()来清除对象。但并不一定马上析构对象；
     * */
    class TcpServer : noncopyable
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
        int  epollfd_;  // 监听用的epoll实例
        int  idlefd_;   // 占一个位置，以防fd耗尽

        std::shared_ptr<ThreadPool> taskPool_; // 任务处理线程池

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
