#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"


static EventLoop* CheckNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d main loop is null! \n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}



TcpConnection::TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
                :loop_(CheckNotNull(loop))
                ,name_(name)
                ,state_(kConnecting)
                ,reading_(true)
                ,localAddr_(localAddr)
                ,peerAddr_(peerAddr)
                ,socket_(new Socket(sockfd))
                ,channel_(new Channel(loop,sockfd))
                ,highWaterMark_(64*1024*1024)    //64M
{
    channel_->setReadEventCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteEventCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseEventCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorEventCallback(std::bind(&TcpConnection::handleError,this));

    LOG_FIFO("TcpConnection::ctor[%s] at %p fd=%d \n",name_.c_str(),this,sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_FIFO("TcpConnection::dtor[%s] at %p fd=%d state=%d \n",name_.c_str(),this,channel_->fd(),(int)state_);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno=0;
    ssize_t n =inputBuffer_.readfd(channel_->fd(),&saveErrno);
    if(n>0)
    {
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno=saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    int saveErrno=0;
    if(channel_->isWriting())
    {
        ssize_t n=outputBuffer_.writefd(channel_->fd(),&saveErrno);
        if(n>0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes()==0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_==kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("Connection fd=%d is down,no more writing \n",channel_->fd());
    }
}
void TcpConnection::handleClose()
{
    LOG_FIFO("fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connptr(shared_from_this());
    connectionCallback_(connptr);
    closeCallback_(connptr);
}
void TcpConnection::handleError()
{
    int err=channel_->fd();
    LOG_ERROR("TcpConnection::handleError[%s]-SO_ERROR=%d \n",name_.c_str(),err);
}

void TcpConnection::send(std::string message)
{
    if(state_==kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(message.c_str(),message.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,message.c_str(),sizeof(message)));
        }
    }
}

void TcpConnection::sendInLoop(const void * data,size_t len)
{
    ssize_t nwrote=0;
    size_t remaining=len;
    bool faultError=false;

    if(state_==kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing!");
        return;
    }
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len);
        if(nwrote>0)
        {
            remaining=len-nwrote;
            if(remaining==0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(TcpConnection::writeCompleteCallback_,shared_from_this()));
            }
            else
            {
                nwrote=0;
                if(errno!=EWOULDBLOCK)
                {
                    LOG_ERROR("TcpConnection::sendInLoop");
                    if(errno==EPIPE || errno==ECONNRESET)
                    {
                        faultError=true;
                    }
                }
            }
        }
    }
    
    if(!faultError && remaining >0)
    {
        size_t oldLen=outputBuffer_.readableBytes();
        if(oldLen + remaining>highWaterMark_
        &&oldLen<highWaterMark_
        &&highWaterMark_)
        {
            loop_->queueInLoop(std::bind(TcpConnection::highWaterMarkCallback_,shared_from_this(),oldLen + remaining));
        }
        outputBuffer_.append((char*)data+nwrote,remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}


void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::shutdown()
{
    if(state_==kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}