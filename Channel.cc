#include "Channel.h"
#include "EventLoop.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_=obj;
    tied_=true;
}


/**
 *当改变channel所表示fd的event事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 *
 */
void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receivetime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receivetime);
        }
    }
    else
    {
        handleEventWithGuard(receivetime);
    }
}

void Channel::handleEventWithGuard(Timestamp receivetime)
{
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
            readCallback_(receivetime);
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
            writeCallback_();
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
            errorCallback_();
    }
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
            closeCallback_();
    }
}