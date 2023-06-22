/*
muduo网络库给用户提供了两个主要的类
TcpServer ： 用于编写服务器程序的
TcpClient ： 用于编写客户端程序的
epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
                        用户的连接和断开       用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写时间的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+Port
               const std::string &nameArg)
        : m_server(loop, listenAddr, nameArg), m_loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 这里function传入的是一个参数，但是onConnection还需要this指针，所以需要提前绑定

        // 给服务器注册用户读写事件回调
        m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量 1个I/O线程   3个worker线程
        m_server.setThreadNum(4);
    }

    void start()
    {
        m_server.start();
    }

private:
    // 专门处理用户的连接创建和断开  epoll listenfd accept
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << std::endl;
            conn->shutdown(); // close(fd)
            // m_loop->quit();
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 接收到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data:" << buf << " time:" << time.toFormattedString() << std::endl;
        conn->send(buf);
    }

    TcpServer m_server; // #1
    EventLoop *m_loop;  // #2 epoll
};

int main()
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl=>epoll
    loop.loop();    // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
}