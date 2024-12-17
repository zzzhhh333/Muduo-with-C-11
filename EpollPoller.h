#pragma once

#include "poller.h"
#include "Timestamp.h"

#include <sys/epoll.h>

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize=16;

    void fillActiveChannels(int numEvents,ChannelList* activeChannels)const;
    void update(int option,Channel* channel);

    using EventsList=std::vector<epoll_event>;

    int epollfd_;
    EventsList events_;
};