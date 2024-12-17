#pragma once

#include "noncopyable.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;
class Timestamp;

class Poller : noncopyable
{
public:
    using ChannelList=std::vector<Channel*>;
    Poller(EventLoop* loop);
    virtual ~Poller()=default;

    //给所有的IO复用保持统一的接口
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)=0;
    virtual void updateChannel(Channel* channel)=0;
    virtual void removeChannel(Channel* channel)=0;

    //判断当前poller是否含有参数channel
    bool hasChannel(Channel* channel) const;

    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    EventLoop * ownerLoop_;
};