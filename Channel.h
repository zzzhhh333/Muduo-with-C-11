#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

/**
 * Channel理解为通道，封装了sockfd和其感兴趣的事件event，如EPOLLIN，EPOLLOUT等
 * 还返回了poller具体返回的事件
 */
class Channel : noncopyable
{
public:
    using EventCallback=std::function<void()>;
    using ReadEventCallback=std::function<void(Timestamp)>;

    Channel(EventLoop* loop,int fd);
    ~Channel();

    //fd得到poller通知后，处理事件
    void handleEvent(Timestamp receivetime);

    //设置回调函数对象
    void setReadEventCallback(ReadEventCallback cb){readCallback_=std::move(cb);}
    void setWriteEventCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseEventCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorEventCallback(EventCallback cb){errorCallback_=std::move(cb);}

    //防止当channel被手动remove掉后，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd()const {return fd_;}
    int events()const {return events_;}
    void set_revents(int rev){revents_=rev;}

    //设置fd感兴趣的事件状态
    void enableReading(){events_ |= kReadEvent;update();}
    void disableReading(){events_ &=~kReadEvent;update();}
    void enableWriting(){events_ |=kWriteEvent;update();}
    void disableWriting(){events_ &=~kWriteEvent;update();}
    void disableAll(){events_ =kNoneEvent;update();}

    //返回fd当前的事件状态
    bool isNoneEvent() const{ return events_ == kNoneEvent;}
    bool isWriting() const{return events_ & kWriteEvent;}
    bool isReading() const{return events_ & kReadEvent;}

    int index(){ return index_;}
    void set_index(int idx){index_=idx;}

    //one loop per thread
    EventLoop* ownerloop(){return loop_;}
    void remove();


private:
    void update();
    void handleEventWithGuard(Timestamp receivetime);


    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;//事件循环
    const int fd_;  //fd，Poller的监听对象
    int events_;    //注册fd感兴趣的事件
    int revents_;   //poller返回的具体发生的事件
    int index_;     //channel的状态

    std::weak_ptr<void> tie_;
    bool tied_;

    //因为channel通道里可以获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};