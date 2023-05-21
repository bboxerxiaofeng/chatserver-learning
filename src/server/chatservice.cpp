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
            cout << "该用户已经登录，不允许重复登录" << endl;
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("error",2);
            response.Add("errmsg","该账号已经登录,请重新输入新账号");
            conn->send(response.ToString());
            {   
                lock_guard<mutex> lock(_connMutex);
                // _userConnMap.insert({id,conn});  // 调用一次构造，两次拷贝构造
                _userConnMap.emplace(id,conn);  // 调用一次构造，一次拷贝构造
            }

        }else if(user.getState() == "offline") {
            // 登录成功，记录用户连接信息，因为onMessage是个多线程操作，所以这里也要考虑线程安全问题
            // 这里用中括号括起来是让锁的范围尽可能小，
            // 下面的数据库的线程安全数据库本身会操作，无需关心，
            // json操作也是每调用一次就会创建栈对象，也无需关心
            {   
                lock_guard<mutex> lock(_connMutex);
                // _userConnMap.insert({id,conn});  // 调用一次构造，两次拷贝构造
                _userConnMap.emplace(id,conn);  // 调用一次构造，一次拷贝构造
            }
            cout << " 登录成功,更新用户状态信息" << endl;
            // 登录成功,更新用户状态信息 state offline=>online
            user.setState("online");
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
            cout << " 用户不存在" << endl;
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",1);
            response.Add("errmsg","该用户不存在,请重新输入新账号");
            conn->send(response.ToString());

        }else {
            // 用户存在但是密码错误
            cout << " 用户存在但是密码错误" << endl;
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

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    cout << "clientCloseException: _userConnMap"<< _userConnMap.size() << endl;
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for( auto it=_userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            cout << "id=" << it->first << endl;
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 更新用户的状态信息
    if(user.getId() != -1) // 表示有在map找到对应的用户
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}
