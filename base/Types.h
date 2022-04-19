#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>

#include <functional>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

////////////////////
/// 测试用头文件//////
//#include <iostream>
////////////////////
////////////////////

namespace base
{
    class Buffer;
    class TcpConnection;

    using onConnection     = std::function<void(void*)>;                            // 连接建立、断开回调函数
    using onMessage        = std::function<void(const std::shared_ptr<TcpConnection>,
                                                Buffer *,
                                                struct sockaddr_in)>;               // 消息到来回调函数
    using onWriteComplete  = std::function<void(struct sockaddr_in)>;               // 消息发送完毕回调函数
    using onCleanTcpSever  = std::function<void(std::string)>;                      // Tcp连接关闭时，清理TcpServer::connections_的回调
    using onCleanEventLoop = std::function<void(int)>;                              // Tcp连接关闭时，清理EventLoop::connections_的回调，并取消监听

    using PendingFunc = std::function<void()>;       // IO线程待办函数
    using ThreadFunc  = std::function<void*(void*)>; // 工作线程主函数
    using Task        = std::function<void()>;       // 任务函数

    const int kMicroSecondsPerSecond = 1000 * 1000;
}

#endif //TYPES_H
