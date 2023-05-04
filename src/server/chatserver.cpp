#include "chatserver.h"
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;

ChatServer::ChatServer(EventLoop* loop, 
                       const InetAddress& listenAddr,
                       const string& nameArg)
                : _server(loop, listenAddr, nameArg), _loop(loop) 
{
    // 给服务器注册用户连接的创建和断开回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 这里function传入的是一个参数，但是onConnection还需要this指针，所以需要提前绑定

    // 给服务器注册用户读写事件回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务器端的线程数量 1个I/O线程   3个worker线程
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

// 专门处理用户的连接创建和断开  epoll listenfd accept
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << std::endl;
    }
    else
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << std::endl;
        conn->shutdown(); // close(fd)
        // _loop->quit();
    }
}

// 专门处理用户的读写事件
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 连接
                Buffer *buffer,               // 缓冲区
                Timestamp time)               // 接收到数据的时间信息
{
    string buf = buffer->retrieveAllAsString();
    std::cout << "recv data:" << buf << " time:" << time.toFormattedString() << std::endl;
    conn->send(buf);
}