#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/epoll.h"
#include "errno.h"
#include "netdb.h"
#include "fcntl.h"
#include "time.h"
#include "stdbool.h"

void hoonow(char *buf,size_t size){
    time_t now=time(NULL);
    struct tm *timeinfo;
    timeinfo=localtime(&now);
    strftime(buf,size,"%Y-%m-%d %H:%M:%S",timeinfo);
}
void hoolog(char *msg){
    char timebuf[20];
    hoonow(timebuf,sizeof(timebuf));
    printf("[%s] %s\n",timebuf,msg);
}
void hoosleep(int seconds){
	sleep(seconds);
}
void hoousleep(int ms){
	usleep(ms);
}

#define MAX_EVENTS 64
static int make_socket_non_blocking(int fd){
	int flags;
	flags=fcntl(fd,F_GETFL,0);
	if(flags==-1){
		perror("fcntl F_GETFL");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flags)==-1){
		perror("fcntl F_SETFL");
		return -1;
	}
	return 0;
}
static int create_and_bind(char *port){
	struct addrinfo hints;
	struct addrinfo *result,*rp;
	int svrfd;
	int err;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;
	if((err=getaddrinfo(NULL,port,&hints,&result))!=0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(err));
		return -1;
	}
	for(rp=result;rp!=NULL;rp=rp->ai_next){
		svrfd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
		if(svrfd==-1){
			continue;
		}
		if(bind(svrfd,rp->ai_addr,rp->ai_addrlen)==0)
			break;
		else{
			close(svrfd);
			perror("bind error");
		}
	}
	if(rp==NULL){
		fprintf(stderr,"Could not bind.\n");
		return -1;
	}
	freeaddrinfo(result);
	return svrfd;
}

static void hooechoserver(char *port){
	int svrfd;
	int epfd;
	int err;
	struct epoll_event event;
	struct epoll_event *events;
	if((svrfd=create_and_bind(port))==-1){
		abort();
	}
	if(make_socket_non_blocking(svrfd)==-1){
		abort();
	}
	if(listen(svrfd,SOMAXCONN)==-1){
		perror("listen");
		abort();
	}
	if((epfd=epoll_create(1))==-1){
		perror("epoll_create");
		abort();
	}
	event.data.fd=svrfd;
	event.events=EPOLLIN | EPOLLET;
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,svrfd,&event)==-1){
		perror("epoll_ctl");
		abort();
	}
	events=calloc(MAX_EVENTS,sizeof(event));
	while(true){
		int n,i;
		fprintf(stdout,"Blocking and waiting for epoll event...\n");
		n=epoll_wait(epfd,events,MAX_EVENTS,-1);
		fprintf(stdout,"Received epoll events\n");
		for(i=0;i<n;i++){
			if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || !(events[i].events & EPOLLIN)){
				perror("epoll error");
				close(events[i].data.fd);
				continue;
			} else if (events[i].data.fd==svrfd){
				while(true){
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];
					in_len=sizeof(in_addr);
					if((infd=accept(svrfd,&in_addr,&in_len))==-1){
						if(errno==EAGAIN || errno==EWOULDBLOCK)
							break;
						else{
							perror("accept");
							break;
						}
					}
					if(getnameinfo(&in_addr,in_len,hbuf,sizeof(hbuf),sbuf,sizeof(sbuf),NI_NUMERICHOST | NI_NUMERICSERV)==0){
						printf("Accepted connection on descriptor %d(host=%s,port=%s)\n",infd,hbuf,sbuf);
					}
					if(make_socket_non_blocking(infd)==-1)
						abort();
					event.data.fd=infd;
					event.events=EPOLLIN | EPOLLET;
					if(epoll_ctl(epfd,EPOLL_CTL_ADD,infd,&event)==-1){
						perror("client epoll_ctl");
						abort();
					}
				}
				continue;
			} else{
				bool done=false;
				while(true){
					ssize_t count;
					char buf[512];
					char echo[512] = {"echo>"};
					if((count=read(events[i].data.fd,buf,sizeof(buf)))==-1){
						if(errno!=EAGAIN){
							perror("client read");
							done=true;
						}
						break;
					} else if(count==0){
						done=true;
						break;
					}
					if(write(STDOUT_FILENO,buf,count)==-1){
						perror("write to stdout");
						abort();
					}
					if(!strncmp(buf,"bye\r\n",5)){
						done=true;
						break;
					}
					strncat(echo,buf,count);
					if(write(events[i].data.fd,echo,count+5)==-1){
						perror("client socket write");
						abort();
					}
				}
				if(done){
					printf("Closed connection on descriptor %d\n",events[i].data.fd);
					close(events[i].data.fd);
				}
			}
		}
	}
	free(events);
	close(svrfd);
}
int main(){
	//hooechoserver("59870");
    hoolog("Server startup.");
    return 0;
}
