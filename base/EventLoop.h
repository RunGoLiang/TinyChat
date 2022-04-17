#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "TcpConnection.h"

namespace base
{
    class EventLoop:noncopyable
    {
    public:
        /// 不可跨线程调用
        explicit EventLoop();
        ~EventLoop();

        void loop();
        void handlePending();
        void addConnectionInLoop(std::shared_ptr<TcpConnection> connection);
        void addClean(int fd);

        /// 可跨线程调用
        void stopLoop();
        void wakeup();
        void addPending(PendingFunc func);
        void addConnection(std::shared_ptr<TcpConnection> connection);

    private:
        bool running_;
        pid_t threadId_; // 所属IO线程的线程ID

        int  wakeupfd_; // 用于跨线程唤醒IO线程
        int  epollfd_;  // 监听用的epoll实例

        int eventsListInitSize_; // 初始化events_的大小，如果不够会成倍扩展
        std::vector<struct epoll_event> events_; // epoll返回发生的事件结构体

        std::shared_ptr<TcpConnection> curConnection_; // 当前正在处理的发生event的Connection

        std::vector<PendingFunc> pendings_; // 待办列表
        pthread_mutex_t pendingsMutex_;     // 待办列表的互斥锁

        std::unordered_map<int,std::shared_ptr<TcpConnection>> connections_; // 监听的TcpConnection列表，K-V -> fd-pointer
    };
}

#endif //EVENTLOOP_H
