#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class InetAddress;
class EventLoop;


class Acceptor : noncopyable
{
public:
    using NewConnectionCallback=std::function<void(int,const InetAddress&)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb){newconnectioncallback_=cb;}

    void listen();

    bool listenning(){return listenning_;}
    

private:
    void handleRead();

    EventLoop* loop_;

    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newconnectioncallback_;
    bool listenning_;
};