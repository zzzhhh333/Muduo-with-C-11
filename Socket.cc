#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if(::bind(sockfd_,(sockaddr*)&localaddr.getSockAddr(),sizeof(sockaddr))<0)
    {
        LOG_FATAL("bind error");
    }
}
void Socket::listen()
{
    if(::listen(sockfd_,1024)<0)
    {
        LOG_FATAL("listen error");
    }
}

int Socket::accept(InetAddress &peeraddr)
{
    int len=sizeof(peeraddr);
    sockaddr_in paddr;
    memset(&paddr,0,sizeof(paddr));
    int confd=::accept4(sockfd_,(sockaddr*)&paddr,(socklen_t*)&len,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if(confd>=0)
    {
        peeraddr.setSockAddr(paddr);
    }
    return confd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_FATAL("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}