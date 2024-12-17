#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>


class Poller;
class Channel;

class EventLoop : noncopyable
{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturntime()const {return pollReturntime_;}

    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在线程，执行cb
    void queueInLoop(Functor cb);

    //唤醒loop所在线程
    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread()const{return pthreadId_==CurrentThread::tid();}




private:
    //wake up
    void handleRead();
    //执行回调
    void doPendingFunctors();

    using ChannelList=std::vector<Channel*>;
    //原子操作，通过CAS实现的
    std::atomic_bool looping_;
    //标识退出loop循环
    std::atomic_bool quit_;
    
    //记录当前loop所在的线程的id
    const pid_t pthreadId_;
    //poller返回发生事件的channel的时间点
    Timestamp pollReturntime_;

    std::unique_ptr<Poller> poller_;
    
    //主要作用，当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    int wakeupFd_;

    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    //标识当前loop是否有需要执行的回调操作
    std::atomic_bool callingPendingFunctors_;
    //用来保护下面的vector容器的线程安全操作
    std::mutex mutex_;
    //存储loop需要的所有回调操作
    std::vector<Functor> pendingFunctors_;
};