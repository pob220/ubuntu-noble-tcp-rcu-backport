#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#ifndef IPPROTO_MPTCP
#define IPPROTO_MPTCP 262
#endif
static int stream(int protocol) {
    int listener=socket(AF_INET,SOCK_STREAM,protocol),client=-1,child=-1,result=1;
    struct sockaddr_in addr={.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
    socklen_t n=sizeof(addr);char byte=0;
    if(listener<0)goto done;
    if(bind(listener,(struct sockaddr*)&addr,sizeof(addr)) || listen(listener,4) || getsockname(listener,(struct sockaddr*)&addr,&n))goto done;
    client=socket(AF_INET,SOCK_STREAM,protocol);if(client<0)goto done;
    if(connect(client,(struct sockaddr*)&addr,sizeof(addr)))goto done;
    child=accept(listener,NULL,NULL);if(child<0)goto done;
    if(send(client,"x",1,0)!=1 || recv(child,&byte,1,MSG_WAITALL)!=1 || byte!='x')goto done;
    if(send(child,"y",1,0)!=1 || recv(client,&byte,1,MSG_WAITALL)!=1 || byte!='y')goto done;
    result=0;
done:
    if(result)perror("stream control");
    if(child>=0)close(child);
    if(client>=0)close(client);
    if(listener>=0)close(listener);
    printf("EDGE_STREAM protocol=%d result=%d\n",protocol,result);return result;
}
int main(void) {
    int fd=socket(AF_INET,SOCK_DGRAM,0),failed=0;
    unsigned char invalid[4]={IPOPT_RR,2,4,0},valid[8]={IPOPT_RR,7,4,0,0,0,0,IPOPT_END};
    if(fd<0)return 1;
    errno=0;int r=setsockopt(fd,IPPROTO_IP,IP_OPTIONS,invalid,sizeof(invalid));
    if(r!=-1 || errno!=EINVAL)failed=1;
    printf("EDGE_MALFORMED_OPTIONS rejected_with_EINVAL=%d\n",r==-1 && errno==EINVAL);
    if(setsockopt(fd,IPPROTO_IP,IP_OPTIONS,valid,sizeof(valid)) || setsockopt(fd,IPPROTO_IP,IP_OPTIONS,NULL,0))failed=1;
    close(fd);
    failed|=stream(0);failed|=stream(IPPROTO_MPTCP);
    printf("SOCKET_EDGE_RESULT=%d\n",failed);return failed;
}
