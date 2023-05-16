#include "chatservice.h"
#include "public.h"
#include <muduo/base/Logging.h>
#include <iostream>

// 获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    LOG_INFO << "do login..." ;
    int id;
    string pwd;
    js.Get("password",pwd);
    js.Get("id",id);

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online") {
            // 该用户已经登录，不允许重复登录
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("error",2);
            response.Add("errmsg","该账号已经登录,请重新输入新账号");
            conn->send(response.ToString());

        }else if(user.getState() == "offline") {
            // 登录成功,更新用户状态信息 state offline=>online
            user.setState("oneline");
            _userModel.updateState(user);   // 更新用户状态信息

            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",0);
            response.Add("id",user.getId());
            response.Add("name",user.getName());
            conn->send(response.ToString());
        }
    }
    else 
    {
        if(user.getId() != id) {
            // 用户不存在
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",1);
            response.Add("errmsg","该用户不存在,请重新输入新账号");
            conn->send(response.ToString());

        }else {
            // 用户存在但是密码错误
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",1);
            response.Add("errmsg","该用户密码不正确,请重新输入密码");
            conn->send(response.ToString());
        }
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    LOG_INFO << "do reg..." ;
    string name,pwd;
    js.Get("name",name);
    js.Get("password",pwd);

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    std::cout << state << std::endl;
    if(state)
    {
        // 注册成功
        json response;
        response.Add("msgid",REG_MSG_ACK);
        response.Add("error",0);
        response.Add("id",user.getId());
        cout << "response " << response.ToString() << endl;
        conn->send(response.ToString());
    }
    else
    {
        // 注册失败
        json response;
        response.Add("msgid",REG_MSG_ACK);
        response.Add("error",1);
        conn->send(response.ToString());
    }
}
