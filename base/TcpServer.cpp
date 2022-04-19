#include "TcpServer.h"

using namespace base;

/*
 *  构造函数
 */
TcpServer::TcpServer(
        int taskPoolSize,
        int ioPoolSize,
        std::string port,
        onConnection    onConnectionFunc,
        onMessage       onMessageFunc,
        onWriteComplete onWriteCompleteFunc)
        :
        taskPool_(new ThreadPool(taskPoolSize)),    /*任务处理线程池*/
        ioThreadsNum_(ioPoolSize),            /*IO线程数量*/
        ip_(INADDR_ANY),                      /*监听地址*/
        port_(port),                          /*监听端口*/
        onConnection_(onConnectionFunc),      /*回调*/
        onMessage_(onMessageFunc),            /*回调*/
        onWriteComplete_(onWriteCompleteFunc),/*回调*/
        running_(false),
        idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)), /*空闲fd*/
        epollfd_(epoll_create1(EPOLL_CLOEXEC)), /*epoll实例*/
        nextEventLoop_(0)  /*IO线程轮叫的下一个*/
{
    signal(SIGPIPE, SIG_IGN); // 忽略信号SIGPIPE。当客户端断开tcp连接后，服务器再向其发送两次信息则服务器进程会收到SIGPIPE信号。这个信号的默认处理是结束进程。
    signal(SIGCHLD, SIG_IGN); // 避免僵尸进程

    // 【1】采用IPV4的TCP，流式，非阻塞，不可继承
    listenfd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    // 【2】设置IP PORT复用
    int optval = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof(optval)));
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof(optval)));
    // 【3】绑定IP PORT
    struct sockaddr_in tmpData;
    tmpData.sin_family      = AF_INET;
    tmpData.sin_port        = htons(atoi(port.c_str()));
    tmpData.sin_addr.s_addr = ip_;
    bind(listenfd_,(struct sockaddr *)&tmpData,sizeof(tmpData));

    // 初始化互斥锁
    pthread_mutex_init(&eventLoopsMutex_, nullptr);
    pthread_mutex_init(&cleanMutex_, nullptr);
}

/*
 *  析构函数
 */
TcpServer::~TcpServer()
{
    stop();

    // 关闭文件描述符
    close(listenfd_);
    close(epollfd_);
    close(idlefd_);

    // 销毁互斥锁
    pthread_mutex_destroy(&eventLoopsMutex_);
    pthread_mutex_destroy(&cleanMutex_);
}

/*
 *  启动TcpServer
 *  可重复调用
 */
void TcpServer::start()
{
    if(running_)
        return;

    // 启动任务处理线程池
    taskPool_->startPool();

    // 创建IO线程池
    createEventLoopThreadPool();

    // 【4】设置监听
    listen(listenfd_,SOMAXCONN);
    // 【5】将listenfd_加入监听队列
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = listenfd_;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, listenfd_, &event);

    // 进入监听循环
    acceptNewConnection();
}

/*
 *  停止TcpServer
 *  可重复调用
 */
void TcpServer::stop()
{
    if(!running_)
        return;

    // 清除epoll对listenfd_的监听
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = listenfd_;
    epoll_ctl(epollfd_, EPOLL_CTL_DEL, listenfd_, &event);

    // 停止任务处理线程池
    taskPool_->stopPool();

    // 关闭所有TcpConnection
    for(auto conn:connections_)
        conn.second->handleClose();

    // 关闭IO线程池
    stopEventLoopThreadPool();

    // 清空IO线程对象
    for(int i=0;i < ioThreadsNum_;++i)
        ioThreads_.pop_back();

    running_ = false;
}

/*
 *  创建一个新的TcpConnection对象，保存至connections_
 *  将新的TcpConnection对象用round Robin方式分配给各个IO EventLoop
 */
void TcpServer::createNewTcpConnection(int connfd,struct sockaddr_in peeraddr)
{
    // 每个TcpConnection的name字符串为：Tcp[xxxxxx……]（64位的时间字符串）
    std::string connectionName;

    // 获取当前时间，从1970年至今的us数
    struct timeval time;
    gettimeofday(&time, NULL);
    int64_t seconds = time.tv_sec;
    int64_t microSeconds = seconds * kMicroSecondsPerSecond + time.tv_usec;
    connectionName = "Tcp[" + std::to_string(microSeconds) + "]";

    // 用shared_ptr保存新建的TcpConnection对象，并将之存入connections_
    assert(nextEventLoop_<ioThreadsNum_);
    std::shared_ptr<TcpConnection> newConnection(new TcpConnection(connectionName,
                                                                   connfd,
                                                                   peeraddr,
                                                                   taskPool_,
                                                                   eventLoops_[nextEventLoop_],
                                                                   onConnection_,
                                                                   onMessage_,
                                                                   onWriteComplete_,
                                                                   std::bind(&TcpServer::addClean,this,std::placeholders::_1)));

    // 关闭negal算法
    int optval = 1;
    setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof optval));

    // TcpConnection的shared_ptr保存两份：TcpServer、EventLoop各一份
    connections_[connectionName] = newConnection;

    // 将新的TcpConnection对象用round Robin方式分配给各个IO EventLoop
    eventLoops_[nextEventLoop_]->addConnection(std::move(newConnection));
    nextEventLoop_ = (++nextEventLoop_) >= ioThreadsNum_ ? 0 : nextEventLoop_;
}

/*
 *  清理列表中已经关闭的TcpConnection对象
 */
void TcpServer::cleanTcpConnection()
{
    std::vector<std::string> cleans;
    pthread_mutex_lock(&cleanMutex_);
    cleans.swap(cleans_);
    pthread_mutex_unlock(&cleanMutex_);

    for(const std::string& curClean : cleans)
    {
        connections_.erase(curClean);
    }
}

/*
 *  向cleans_[]新增一项内容
 *  供TcpConnection的handleClose()中跨线程调用
 */
void TcpServer::addClean(std::string name)
{
    pthread_mutex_lock(&cleanMutex_);
    cleans_.push_back(name);
    pthread_mutex_unlock(&cleanMutex_);
}

/*
 *  IO线程入口函数
 */
void* TcpServer::entryIOThread(void *Data)
{
    std::shared_ptr<EventLoop> eventLoop(new EventLoop());
    eventLoops_.push_back(eventLoop);

    eventLoop->loop();

    return nullptr;
}

/*
 *  创建EventLoop固定线程池
 */
void TcpServer::createEventLoopThreadPool()
{
    for(int i=0;i < ioThreadsNum_;++i)
    {
        std::unique_ptr<Thread> newIOThread(new Thread(std::bind(&TcpServer::entryIOThread,this,std::placeholders::_1)));
        newIOThread->startThread();
        ioThreads_.emplace_back(std::move(newIOThread));
    }
}

/*
 *  通知IO线程停止
 */
void TcpServer::stopEventLoopThreadPool()
{
    for(int i=0;i < ioThreadsNum_;++i)
    {
        eventLoops_[i]->stopLoop();  // 马上退出loop()，只要退出loop()就能结束IO线程
        ioThreads_[i]->stopThread(); // detach IO线程，不等待回收资源
    }

    // 删除eventLoops_中保存的引用，临一份引用在IO线程函数entryIOThread()中，退出线程函数后自动析构EventLoop对象
    for(int i=0;i < ioThreadsNum_;++i)
        eventLoops_.pop_back();
}

/*
 *  循环监听listenfd_，接受新连接请求
 */
void TcpServer::acceptNewConnection()
{
    if(!running_)
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

        // 连接请求处理
        for(int i=0;i < numEvent;++i)
        {
            if(events_[i].data.fd == listenfd_)
            {
                // 接受连接请求
                struct sockaddr_in peeraddr; // 对等方ip port，网络字节序
                socklen_t peerlen = sizeof(peeraddr);
                int connfd = accept4(listenfd_,
                                      (struct sockaddr *)&peeraddr,
                                              &peerlen,
                                              SOCK_NONBLOCK | SOCK_CLOEXEC);

                // fd耗尽处理
                if (connfd == -1)
                {
                    if (errno == EMFILE)
                    {
                        close(idlefd_);
                        idlefd_ = accept(listenfd_, NULL, NULL);
                        close(idlefd_);
                        idlefd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
                        continue;
                    }
                }

                // 连接建立时调用回调函数
                onConnection_((void *)&peeraddr);

                // 创建新的TcpConnection对象，保存至connections_，并分配给IO线程
                createNewTcpConnection(connfd,peeraddr);
            }
        }
        // 根据cleanQueue_来清理connections_中的TcpConnection对象
        cleanTcpConnection();
    }

}