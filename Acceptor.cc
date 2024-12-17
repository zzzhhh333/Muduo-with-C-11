#include "Logger.h"
#include "Acceptor.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>


static int createNonblocking()
{
    int sockfd=::socket(AF_INET,SOCK_NONBLOCK|SOCK_CLOEXEC|SOCK_STREAM,IPPROTO_TCP);
    if(sockfd<0)
    {
        LOG_FATAL("%s:%s:%d:socket create error:%d",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    :loop_(loop)
    ,acceptSocket_(::createNonblocking())
    ,acceptChannel_(loop,acceptSocket_.fd())
    ,listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadEventCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd=acceptSocket_.accept(peerAddr);
    if(connfd>=0)
    {
        if(newconnectioncallback_)
        {
            newconnectioncallback_(connfd,peerAddr);//轮询找到subloop，唤醒，分发当前的新客户端的channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d:accept error:%d",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE)
        {
             LOG_ERROR("%s:%s:%d:sockfd reach limit:%d",__FILE__,__FUNCTION__,__LINE__,errno);
        }
    }
}