#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew=-1;
const int kAdd=1;
const int kDelete=2;




EpollPoller::EpollPoller(EventLoop* loop)
    :Poller(loop),epollfd_(::epoll_create1(EPOLL_CLOEXEC)),events_(kInitEventListSize)
{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

void EpollPoller::updateChannel(Channel* channel)
{
    const int index=channel->index();
    LOG_FIFO("fd=%d,events=%d,index=%d",channel->fd(),channel->events(),channel->index());
    if(index == kNew ||index == kDelete)
    {
        if(index==kNew)
        {
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        channel->set_index(kAdd);
        update(EPOLL_CTL_ADD,channel);
    }
    else
    {
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kNew);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}


void EpollPoller::update(int option,Channel* channel)
{
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=channel->events();
    event.data.ptr=channel;
    int ret=::epoll_ctl(epollfd_,option,channel->fd(),&event);
    if(ret==-1)
    {
        if(option==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl error:%d\n",errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl error:%d\n",errno);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel)
{   
    int fd=channel->fd();
    channels_.erase(fd);

    int index=channel->index();
    if(index==kAdd)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

Timestamp EpollPoller::poll(int timeoutMs,ChannelList* activeChannels)
{
    LOG_DEBUG("%s\n",__FUNCTION__);
    int numEvents=epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    int saveerr=errno;
    if(numEvents>0)
    {
        fillActiveChannels(numEvents,activeChannels);
        if(numEvents==events_.size())
        {
            events_.resize(numEvents*2);
        }
    }
    else if(numEvents==0)
    {
        LOG_DEBUG("%s timeout! \n",__FUNCTION__);
    }
    else
    {
        if(saveerr!=EINTR)
        {
            errno=saveerr;
            LOG_ERROR("%s \n",__FUNCTION__);
        }
    }
    return Timestamp::now();
}

void EpollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels)const
{
    for(int i=0;i<numEvents;i++)
    {
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}