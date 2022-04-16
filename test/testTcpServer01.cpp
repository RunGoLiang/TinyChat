#include <iostream>

#include "../base/TcpServer.h"

using namespace base;

void *onConnectionFunc(void *peerptr)
{
    struct sockaddr_in *peeraddr = static_cast<struct sockaddr_in *>(peerptr);
    std::cout << "新的连接到来!" << "[" << inet_ntoa(peeraddr->sin_addr) << ":" << ntohs(peeraddr->sin_port) << "]" << std::endl;
}

void *onMessageFunc(void *tmp)
{
    std::cout << "新的消息到达" << std::endl;
}

void *onWriteCompleteFunc(void *tmp)
{
    std::cout << "消息发送完毕" << std::endl;
}

int main()
{
    TcpServer server("1888",onConnectionFunc,onMessageFunc,onWriteCompleteFunc);

    std::cout << "server创建完毕" <<std::endl;
    server.start();

    exit(0);
}