#include <iostream>

#include "../base/TcpServer.h"

using namespace base;

void onConnectionFunc(void *peerptr)
{
    struct sockaddr_in *peeraddr = static_cast<struct sockaddr_in *>(peerptr);
    std::cout << "有连接建立/断开!" << "[" << inet_ntoa(peeraddr->sin_addr) << ":" << ntohs(peeraddr->sin_port) << "]" << std::endl;
}

void onMessageFunc(std::string&& message,struct sockaddr_in peeraddr)
{
    std::cout << "从" << "[" << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port) << "]" << "收到消息：" << message <<std::endl;
}

void onWriteCompleteFunc(void *tmp)
{
    std::cout << "消息发送完毕" << std::endl;
}

int main()
{
    TcpServer server(3,3,"1888",onConnectionFunc,onMessageFunc,onWriteCompleteFunc);

    std::cout << "server创建完毕" <<std::endl;
    server.start();
    std::cout << "server启动完成" <<std::endl;
    exit(0);
}