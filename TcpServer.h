#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "poller.h"
#include "Socket.h"
#include "Timestamp.h"


#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>


class TcpServer : noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg,Option option=kNoReusePort);
    ~TcpServer();

    void setThreadNum(int threadNum);

    void setThreadInitCallback(ThreadInitCallback cb){threadInitCallback_=cb;}
    void setConnectionCallback(ConnectionCallback cb){connectionCallback_=cb;}
    void setMessageCallback(MessageCallback cb){messageCallback_=cb;}
    void setWriteCompleteCallback(WriteCompleteCallback cb){writeCompleteCallback_=cb;}

    void start();


private:
    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;

    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_; //baseloop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_;

    int nextConnId_;

    ConnectionMap connections_;
};
