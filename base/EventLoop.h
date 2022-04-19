#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "TcpConnection.h"

namespace base
{
    /*
     *  事件循环，one loop per thread
     *  EventLoop对象的生命周期也是由shared_ptr管理。正常运作时保留2+n份指向其对象的引用计数：
     *  TcpServer的eventLoops_和IO线程函数中各有一份，属于这个EventLoop的TcpConnection对象中有n份。
     *  TcpServer在创建IO线程时，先创建一个Thread对象，然后在线程函数TcpServer::entryIOThread()中创建EventLoop对象。
     *  线程函数中创建对象时用shared_ptr保存一份引用，然后马上往TcpServer的eventLoops_也保存一份引用。
     *  构造TcpConnection时，由TcpServer传递一个shared_ptr给TcpConnection对象。
     *
     *  要停止IO线程前，先断开所有Tcp连接，保存的n份引用都销毁，
     *  然后在TcpServer::stopEventLoopThreadPool()中将TcpServer的eventLoops_中的引用清除（引用剩余1），并命令IO线程停止；
     *  IO线程结束时，先退出loop()，然后就退出线程函数，此时线程函数中的引用销毁（引用剩余0），此时析构EventLoop对象。
     * */
    class EventLoop : noncopyable
    {
    public:
        /// 不可跨线程调用
        explicit EventLoop();
        ~EventLoop();

        void loop();
        void handlePending();
        void addConnectionInLoop(std::shared_ptr<TcpConnection> connection);
        void addClean(int fd);

        void enableEpollOut(int fd);
        void disableEpollOut(int fd);

        /// 可跨线程调用
        void stopLoop();
        void wakeup();
        void addPending(PendingFunc func);
        void addConnection(std::shared_ptr<TcpConnection> connection);



    private:
        bool running_;
        pid_t threadId_; // 所属IO线程的线程ID

        int wakeupfd_; // 用于跨线程唤醒IO线程
        int epollfd_;  // 监听用的epoll实例

        std::shared_ptr<TcpConnection> curConnection_; // 当前正在处理的发生event的Connection

        std::vector<PendingFunc> pendings_; // 待办列表
        pthread_mutex_t pendingsMutex_;     // 待办列表的互斥锁

        std::unordered_map<int,std::shared_ptr<TcpConnection>> connections_; // 监听的TcpConnection列表，K-V -> fd-pointer
    };
}

#endif //EVENTLOOP_H
