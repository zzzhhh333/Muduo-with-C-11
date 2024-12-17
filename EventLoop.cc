#include "EventLoop.h"
#include "Logger.h"
#include "poller.h"
#include "Channel.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <errno.h>

//防止一个线程创建多个EventLoop
__thread EventLoop*t_loopInThisThread=nullptr;

//定义默认的poller IO复用接口的超时时间
const int kPollTimeMs=10000;

int createEventfd()
{
    int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0)
    {
        LOG_FATAL("eventfd error:%d",errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,pthreadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d",this,pthreadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another Eventloop %p exist in this thread %d",t_loopInThisThread,pthreadId_);
    }
    else
    {
        t_loopInThisThread=this;
    }

    wakeupChannel_->setReadEventCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    t_loopInThisThread=nullptr;
}


void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof (one))
    {
        LOG_ERROR ("EventLoop::handleRead() reads %ld bytes instead of 8",n);
    }
}


void EventLoop::loop()
{
    looping_=true;
    quit_=false;

    LOG_FIFO("EventLoop %p start working",this);

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturntime_=poller_->poll(kPollTimeMs,&activeChannels_);
        for(auto&channel:activeChannels_)
        {
            channel->handleEvent(pollReturntime_);
        }

        doPendingFunctors();
    }
}

void EventLoop::quit()
{
    quit_=true;
    
    if(!isInLoopThread())
    {
        wakeup();
    }
}


void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    callingPendingFunctors_=true;
    std::vector<Functor> functors;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor:functors)
    {
        functor();
    }
    callingPendingFunctors_=false;
}


void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=::write(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8",n);
    }
}