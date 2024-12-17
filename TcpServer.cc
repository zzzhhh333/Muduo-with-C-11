#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

static EventLoop* CheckNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d main loop is null!",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}


TcpServer::TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg,Option option)
    :loop_(CheckNotNull(loop))
    ,ipPort_(listenAddr.toIpPort())
    ,name_(nameArg)
    ,acceptor_(new Acceptor(loop,listenAddr,option==kReusePort))
    ,threadPool_(new EventLoopThreadPool(loop,name_))
    ,connectionCallback_()
    ,messageCallback_()
    ,nextConnId_(1)
    ,started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    LOG_FIFO("TcpServer::~TcpServer [%s] destructing",name_.c_str());

    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int threadNum)
{
    threadPool_->setThreadNum(threadNum);
}

void TcpServer::start()
{
    if(started_++==0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop* ioLoop=threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    nextConnId_++;
    std::string connName=name_+buf;

    LOG_FIFO("TcpServer::newConnection[%s]- new connection [%s] from %s",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    struct sockaddr_in localaddr;
    memset(&localaddr,0, sizeof(localaddr));
    socklen_t addrlen = sizeof(localaddr);
    if (::getsockname(sockfd,(sockaddr*)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddress(localaddr);

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddress,
                                            peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_FIFO("TcpServer::removeConnectionInLoop [%s] - connection %s",name_.c_str(),conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
