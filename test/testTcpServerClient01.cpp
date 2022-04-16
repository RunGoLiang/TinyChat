#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

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

    sleep(3);

    close(coonnfd);

    exit(0);
}