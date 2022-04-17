#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

int main()
{
    // 【1】socket
    int coonnfd = socket(AF_INET,SOCK_STREAM,0);

    // 【2】connect
    struct sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port   = htons(1888);
    inet_pton(AF_INET,"127.0.0.1",&peerAddr.sin_addr.s_addr);
    connect(coonnfd,(struct sockaddr *)&peerAddr,sizeof(peerAddr));

    std::cout << "连接请求已发送" <<std::endl;

    sleep(5);

    char buf[] = "hello!";
    write(coonnfd,buf,sizeof(buf));
    std::cout << "消息已发送" <<std::endl;

    sleep(5);

    close(coonnfd);
    std::cout << "连接已关闭" <<std::endl;

    exit(0);
}