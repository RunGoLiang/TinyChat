#include "EventLoop.h"

using namespace base;

/*
 *  构造函数
 */
EventLoop::EventLoop():
        epollfd_(epoll_create1(EPOLL_CLOEXEC)), /*epoll实例*/
        curConnection_(nullptr),
        threadId_(static_cast<pid_t>(::syscall(SYS_gettid))) /*EventLoop对象是在所属的线程被创建的，因此构造时就读取即可*/
{
    // 注册wakeupfd，用于唤醒
    wakeupfd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = wakeupfd_;
    epoll_ctl(epollfd_,EPOLL_CTL_ADD, wakeupfd_, &event);

    // 初始化互斥锁
    pthread_mutex_init(&pendingsMutex_, nullptr);
}

/// FIXME:和TcpServer谁负责关闭所有Tcp连接？关闭epoll。
/*
 *  析构函数
 */
EventLoop::~EventLoop()
{
    // 销毁互斥锁
    pthread_mutex_destroy(&pendingsMutex_);

    // 注销监听，关闭wakeup
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = wakeupfd_;
    epoll_ctl(epollfd_,EPOLL_CTL_DEL, wakeupfd_, &event);
    close(wakeupfd_);
}

/*
 *  EventLoop主循环
 */
void EventLoop::loop()
{
    if(running_)
        return;

    running_ = true;

    const int eventsListInitSize_ = 16; // 初始化events_的大小，如果不够会成倍扩展
    std::vector<struct epoll_event> events_(eventsListInitSize_); // epoll返回发生的事件结构体

    while(running_)
    {
        int numEvent = epoll_wait(epollfd_,&(*events_.begin()), events_.size(),-1);

        // 异常情况处理
        if(numEvent == -1 && errno == EINTR)
            continue;
        if(numEvent == 0)
            continue;
        if(numEvent == events_.size())
            events_.resize(events_.size() * 2); // 成倍地扩展大小

        // event处理
        for(int i=0;i < numEvent;++i)
        {
            // 被wakeup
            if(events_[i].data.fd == wakeupfd_)
            {
                uint64_t one = 1;
                ssize_t n = ::read(wakeupfd_, &one, sizeof(one)); // 读了就扔掉
                if (n != sizeof one)
                {
                    /// FIXME:读取出错
                }
                continue;
            }

            curConnection_   = connections_[events_[i].data.fd];
            uint32_t revents = events_[i].events;

            // POLLHUP只有在output时才会产生，因此如果只关注了in事件时代表发生error
            if ((revents & EPOLLHUP) && !(revents & EPOLLIN))
            {
                curConnection_->handleClose();
            }

            // 发生错误
            if (revents & (EPOLLERR))
            {
                curConnection_->handleError();
            }

            // 可读事件
            // POLLRDHUP：对等方关闭连接、半连接，也认为是一件可读事件
            if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
            {
                curConnection_->handleRead();
            }

            // 可写事件
            if (revents & EPOLLOUT)
            {
                curConnection_->handleWrite();
            }
        }

        // 处理完event后处理pending
        handlePending();

        curConnection_.reset();
    }
}

/*
 *  处理待办任务
 *  先把pendings_内容取出来，再执行待办，缩小临界区
 */
void EventLoop::handlePending()
{
    std::vector<PendingFunc> pendings;
    pthread_mutex_lock(&pendingsMutex_);
    pendings.swap(pendings_);
    pthread_mutex_unlock(&pendingsMutex_);

    for(const PendingFunc& curFunc : pendings)
        curFunc();
}

/*
 *  添加TcpConnection对象到列表，并注册监听其fd
 */
void EventLoop::addConnectionInLoop(std::shared_ptr<TcpConnection> connection)
{
    // 告知TcpConnection其所属的线程ID
    connection->setTid(threadId_);

    // 设置清除回调
    connection->setonCleanEventLoop(std::bind(&EventLoop::addClean,this,std::placeholders::_1));

    // 添加connection到列表
    int fd = connection->getFd();
    connections_[fd] = connection;

    // 注册监听
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epollfd_,EPOLL_CTL_ADD, fd, &event);
}

/*
 *  清除TcpConnection对象，并取消监听其fd
 */
void EventLoop::addClean(int fd)
{
    // 取消监听其fd
    struct epoll_event event;
    event.data.fd = fd;
    event.events  = EPOLLIN;
    epoll_ctl(epollfd_,EPOLL_CTL_DEL,fd,&event);

    // 清除TcpConnection对象
    connections_.erase(fd);
}

/****************************************************************************************************************/

/*
 *  通知loop停止
 *  可重复调用
 */
void EventLoop::stopLoop()
{
    if(!running_)
        return;

    running_ = false;
}

/*
 *  从epoll中唤醒EventLoop
 *  可以往IO线程监听的wakeupfd_中写入触发read event来结束poll()阻塞。
 */
void EventLoop::wakeup()
{
    // 随便写点什么都行
    uint64_t tmp = 1;
    ssize_t n = ::write(wakeupfd_, &tmp, sizeof tmp);
    if (n != sizeof tmp)
    {
        /// FIXME:发送出错
    }
}

/*
 *  添加待办
 *  跨线程调用安全
 *  如果想跨线程管理EventLoop对象，比如注册一个连接，不能直接跨线程调用addPending，因为这需要this指针
 *  正确的做法是：通过一个EventLoop调用成员函数（类似addConnection）来间接调用addPending，因为只有成员函数才有this指针
 *  因此EventLoop需要提供一些API来跨线程间接调用addPending
 */
void EventLoop::addPending(PendingFunc func)
{
    pthread_mutex_lock(&pendingsMutex_);
    pendings_.push_back(func);
    pthread_mutex_unlock(&pendingsMutex_);
}

/*
 *  添加TcpConnection对象，并注册监听其fd
 */
void EventLoop::addConnection(std::shared_ptr<TcpConnection> connection)
{
    addPending(std::bind(&EventLoop::addConnectionInLoop,this,std::move(connection)));
    wakeup(); // 添加一个连接比较紧急，需要唤醒IO线程
}