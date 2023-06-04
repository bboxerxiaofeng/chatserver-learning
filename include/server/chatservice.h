#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "CJsonObject.hpp"
using json = neb::CJsonObject;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js,Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理点对点聊天业务 
    void OneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
private:
    ChatService();

    // 存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;
    // 存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
};

#endif