#include <iostream>

#include "../base/TcpServer.h"

using namespace base;

void onConnectionFunc(void *peerptr)
{
    struct sockaddr_in *peeraddr = static_cast<struct sockaddr_in *>(peerptr);
    std::cout << "有连接建立/断开!" << "[" << inet_ntoa(peeraddr->sin_addr) << ":" << ntohs(peeraddr->sin_port) << "]" << std::endl;
}

// 任务函数，在任务线程中被执行
// 此处做echo server
void taskFunction(const std::shared_ptr<TcpConnection> conn,std::string message)
{
    //
    // 数据处理步骤...此处没有处理
    //

    // 发送处理结果
    std::cout << "taskFunction被执行" << std::endl;
    conn->send(std::move(message)); // 跨线程调用
}

void onMessageFunc(const std::shared_ptr<TcpConnection> conn,
                   Buffer *inputBuffer,struct sockaddr_in peeraddr)
{
    std::string message = inputBuffer->retrieveAllAsString();

    std::cout << "从" << "[" << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port) << "]" \
    << "收到消息：" << message <<std::endl;

    conn->addTaskToPool(std::bind(taskFunction,conn,std::move(message))); // 向任务池投入一个任务，由任务线程执行
}



void onWriteCompleteFunc(struct sockaddr_in peeraddr)
{
    std::cout << "给" << "[" << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port) << "]" \
    << "的消息发送完毕" << std::endl;
}

int main()
{
    TcpServer server(3,3,"1888",onConnectionFunc,onMessageFunc,onWriteCompleteFunc);

    std::cout << "server创建完毕" <<std::endl;
    server.start();

    exit(0);
}