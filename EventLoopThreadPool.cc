#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"



EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    :baseLoop_(baseLoop)
    ,name_(nameArg)
    ,started_(false)
    ,numThreads_(0)
    ,next_(0)
{

}
EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_=true;

    for(int i=0;i<numThreads_;i++)
    {
        char name[name_.size()+32];
        snprintf(name,sizeof(name),"%s%d",name_.c_str(),i);
        EventLoopThread* t=new EventLoopThread(cb,name);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(t));
        loops_.emplace_back(t->startLoop());
    }

    if(numThreads_==0 && cb)
    {
        cb(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop=baseLoop_;

    if(!loops_.empty())
    {
        loop=loops_[next_];
        next_++;

        if(next_>=loops_.size())
        {
            next_=0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else
    {
        return loops_;
    }

}    

