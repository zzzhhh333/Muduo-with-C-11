#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>
#include <string>
#include <atomic>

class EventLoop;
class Socket;
class Channel;


class TcpConnection :noncopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name(){return name_;}
    const InetAddress& localAddr(){return localAddr_;}
    const InetAddress& peerAddr(){return peerAddr_;}
    bool connected(){return state_==kConnected;}
    bool disconnected(){return state_==kDisconnected;}

    void send(const std::string message);
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    Buffer* inputBuffer(){return &inputBuffer_;}
    Buffer* outputBuffer(){return &outputBuffer_;}

    void connectEstablished();
    void connectDestroyed();
private:
    enum StateE
    {
        kDisconnected,kConnecting,kConnected,kDisconnecting
    };
    void setState(StateE state){state_=state;}

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data,size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress peerAddr_;
    const InetAddress localAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};