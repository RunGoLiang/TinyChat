#include "TcpServer.h"

using namespace base;

/*
 *  构造函数
 */
TcpServer::TcpServer(
        std::string port,
        onConnection    onConnectionFunc,
        onMessage       onMessageFunc,
        onWriteComplete onWriteCompleteFunc)
        :
        ip_(INADDR_ANY),                      /*监听地址*/
        port_(port),                          /*监听端口*/
        onConnection_(onConnectionFunc),      /*回调*/
        onMessage_(onMessageFunc),            /*回调*/
        onWriteComplete_(onWriteCompleteFunc),/*回调*/
        running_(false),
        idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)), /*空闲fd*/
        epollfd_(epoll_create1(EPOLL_CLOEXEC)), /*epoll实例*/
        eventsListInitSize_(16),                     /*事件保存列表初始化大小*/
        events_(eventsListInitSize_)              /*epoll事件列表*/
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
}

/*
 *  析构函数
 */
TcpServer::~TcpServer()
{
    stop();

    close(listenfd_);
    close(epollfd_);
    close(idlefd_);
}

/*
 *  启动TcpServer
 *  可重复调用
 */
void TcpServer::start()
{
    if(running_)
        return;

    /// FIXME:创建IO线程池
//    createEventLoopThreadPool();

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

    /// FIXME:清除epoll对listenfd_的监听
    /// FIXME:关闭IO线程池
    /// FIXME:关闭所有TcpConnection
//    stopEventLoopThreadPool();

    running_ = false;
}

/*
 *  创建一个新的TcpConnection对象
 */
void TcpServer::createNewTcpConnection()
{

}

/*
 *  清理列表中已经关闭的TcpConnection对象
 */
void TcpServer::cleanTcpConnection()
{

}

/*
 *  创建EventLoop固定线程池
 */
void TcpServer::createEventLoopThreadPool()
{

}

/*
 *  停止EventLoop固定线程池
 */
void TcpServer::stopEventLoopThreadPool()
{

}

/*
 *  循环监听listenfd_，接受新连接请求
 */
void TcpServer::acceptNewConnection()
{
    if(!running_)
        running_ = true;

    while(running_)
    {
        int numEvent = epoll_wait(epollfd_,&(*events_.begin()), events_.size(),-1);

        if(numEvent == -1 && errno == EINTR)
            continue;
        if(numEvent == 0)
            continue;
        if(numEvent == events_.size())
            events_.resize(events_.size() * 2); // 成倍地扩展大小

        // 接受连接，创建新的TcpConnection对象，保存至connections_
        for(int i=0;i < numEvent;++i)
        {
            if(events_[i].data.fd == listenfd_)
            {
                struct sockaddr_in peeraddr; // 对等方ip port，网络字节序
                socklen_t peerlen = sizeof(peeraddr);
                int connfd = accept4(listenfd_,
                                      (struct sockaddr *)&peeraddr,
                                              &peerlen,
                                              SOCK_NONBLOCK | SOCK_CLOEXEC);

                onConnection_((void *)&peeraddr); // 连接建立时调用回调函数

                close(connfd);
//                createNewTcpConnection();
            }
        }

        // 将新的TcpConnection对象用round Robin方式分配给各个IO EventLoop


        // 根据cleanQueue_来清理connections_中的TcpConnection对象
//        cleanTcpConnection();
    }

}