#include "poller.h"
#include "EpollPoller.h"

#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;     //poll
    }
    else
    {
        return new EpollPoller(loop);        //epoll
    }
}