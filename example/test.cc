#include <mymuduo/TcpServer.h>
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop* loop,InetAddress& addr,const std::string& name)
        :server_(loop,addr,name)
        ,loop_(loop)
    {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }
private:
    TcpServer server_;
    EventLoop* loop_;

    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_FIFO("EchoServer:%s connect successed!",conn->peerAddr().toIpPort().c_str());
        }
        else
        {
            LOG_FIFO("EchoServer:%s disconnect !",conn->peerAddr().toIpPort().c_str());
        }
        
    }

    void onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp receivetime)
    {
        std::string message=buffer->retrieveAllAsString();
        conn->send(message);
        conn->shutdown();
    }
};



int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer echoServer(&loop,addr,"echoServer");
    echoServer.start();
    loop.loop();
    return 0;
}
