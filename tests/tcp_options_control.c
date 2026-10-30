/* Local-only VM regression control. No external addresses or exploit payloads. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int listener;
static struct sockaddr_in target;
static atomic_int accepted, reflected, completed, failed;
static const int clients=4, iterations=500;
static void *server(void *unused) {
    (void)unused;
    for (int i=0;i<clients*iterations;i++) {
        int fd=accept(listener,NULL,NULL);
        if(fd<0){atomic_fetch_add(&failed,1);break;}
        unsigned char options[40]={0};socklen_t n=sizeof(options);
        if(getsockopt(fd,IPPROTO_IP,IP_OPTIONS,options,&n)==0 && n>0) {
            for(unsigned j=0;j<n;j++)if(options[j]==IPOPT_RR){atomic_fetch_add(&reflected,1);break;}
        }
        atomic_fetch_add(&accepted,1);
        char byte;
        if(recv(fd,&byte,1,MSG_WAITALL)==1 && send(fd,&byte,1,MSG_NOSIGNAL)==1){}
        else atomic_fetch_add(&failed,1);
        close(fd);
    }
    return NULL;
}
static void *client(void *unused) {
    (void)unused;
    for(int i=0;i<iterations;i++) {
        int fd=socket(AF_INET,SOCK_STREAM,0);
        unsigned char rr[8]={IPOPT_RR,7,4,0,0,0,0,IPOPT_END};
        struct timeval timeout={3,0};
        setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
        if(fd<0 || setsockopt(fd,IPPROTO_IP,IP_OPTIONS,rr,sizeof(rr)) ||
           connect(fd,(struct sockaddr*)&target,sizeof(target))) {
            atomic_fetch_add(&failed,1);if(fd>=0)close(fd);continue;
        }
        char sent='x',received=0;
        if(send(fd,&sent,1,MSG_NOSIGNAL)==1 && recv(fd,&received,1,MSG_WAITALL)==1 && received==sent)
            atomic_fetch_add(&completed,1);
        else atomic_fetch_add(&failed,1);
        close(fd);
    }
    return NULL;
}
int main(int argc,char **argv) {
    int mapped=argc>1 && strcmp(argv[1],"mapped")==0;
    listener=socket(mapped?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(listener<0){perror("socket");return 1;}
    int yes=1;setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    target=(struct sockaddr_in){.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_LOOPBACK),.sin_port=0};
    if(mapped) {
        int no=0;setsockopt(listener,IPPROTO_IPV6,IPV6_V6ONLY,&no,sizeof(no));
        struct sockaddr_in6 any={.sin6_family=AF_INET6,.sin6_addr=IN6ADDR_ANY_INIT,.sin6_port=0};
        if(bind(listener,(struct sockaddr*)&any,sizeof(any)) || listen(listener,128)){perror("listen6");return 1;}
        socklen_t n=sizeof(any);getsockname(listener,(struct sockaddr*)&any,&n);target.sin_port=any.sin6_port;
    } else {
        if(bind(listener,(struct sockaddr*)&target,sizeof(target)) || listen(listener,128)){perror("listen");return 1;}
        socklen_t n=sizeof(target);getsockname(listener,(struct sockaddr*)&target,&n);
    }
    pthread_t s,c[4];struct timespec start,end;clock_gettime(CLOCK_MONOTONIC,&start);
    pthread_create(&s,NULL,server,NULL);
    for(int i=0;i<clients;i++)pthread_create(&c[i],NULL,client,NULL);
    for(int i=0;i<clients;i++)pthread_join(c[i],NULL);
    pthread_join(s,NULL);close(listener);clock_gettime(CLOCK_MONOTONIC,&end);
    double elapsed=(end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/1e9;
    printf("TCP_OPTIONS_RESULT mapped=%d accepted=%d reflected_rr=%d completed=%d failed=%d elapsed_seconds=%.3f\n",mapped,accepted,reflected,completed,failed,elapsed);
    return accepted==clients*iterations && reflected==accepted && completed==accepted && failed==0 ? 0:1;
}
