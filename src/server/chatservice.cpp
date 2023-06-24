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
    m_msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    m_msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    m_msgHandlerMap.insert({ONECHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    m_msgHandlerMap.insert({ADDFRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});

    // 群组业务管理相关事件处理回调注册
    m_msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = m_msgHandlerMap.find(msgid);
    if(it == m_msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return m_msgHandlerMap[msgid];
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    LOG_INFO << "do login..." ;
    int id;
    std::string pwd;
    js.Get("password",pwd);
    js.Get("id",id);

    User user = m_userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online") {
            // 该用户已经登录，不允许重复登录
            std::cout << "该用户已经登录，不允许重复登录" << std::endl;
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("error",2);
            response.Add("errmsg","该账号已经登录,请重新输入新账号");
            conn->send(response.ToString());
            {   
                std::lock_guard<std::mutex> lock(m_connMutex);
                // m_userConnMap.insert({id,conn});  // 调用一次构造，两次拷贝构造
                m_userConnMap.emplace(id,conn);  // 调用一次构造，一次拷贝构造
            }

        }else if(user.getState() == "offline") {
            // 登录成功，记录用户连接信息，因为onMessage是个多线程操作，所以这里也要考虑线程安全问题
            // 这里用中括号括起来是让锁的范围尽可能小，
            // 下面的数据库的线程安全数据库本身会操作，无需关心，
            // json操作也是每调用一次就会创建栈对象，也无需关心
            {   
                std::lock_guard<std::mutex> lock(m_connMutex);
                // m_userConnMap.insert({id,conn});  // 调用一次构造，两次拷贝构造
                m_userConnMap.emplace(id,conn);  // 调用一次构造，一次拷贝构造
            }
            std::cout << " 登录成功,更新用户状态信息" << std::endl;
            // 登录成功,更新用户状态信息 state offline=>online
            user.setState("online");
            m_userModel.updateState(user);   // 更新用户状态信息

            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",0);
            response.Add("id",user.getId());
            response.Add("name",user.getName());

            // 查询该用户是否有离线消息，有就读取出来
            std::vector<std::string> msg = m_offlineMsgModel.query(id);
            if(!msg.empty())
            {
                response.AddEmptySubArray("offlinemessage");
                //for(std::vector<std::string>::iterator msgit = msg.begin(); msgit != msg.end(); msgit++)
                for(auto &msgit : msg)
                {
                    json tmp(msgit);
                    std::string message;
                    tmp.Get("msg",message);
                    response["offlinemessage"].Add(message);
                }
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                m_offlineMsgModel.remove(id);
            }


            // 查询该用户的好友信息并返回

            std::vector<User> userVec = m_friendModel.query(id);
            if(!userVec.empty())
            {
                response.AddEmptySubArray("friends");
                for (User &user : userVec)
                {
                    json tmp;
                    tmp.Add("name",user.getName());
                    tmp.Add("state",user.getState());
                    tmp.Add("id",user.getId());
                    response["friends"].AddAsFirst(tmp);
                }
            }

            // 查询该用户的群组信息
            std::vector<Group> groupUserVec = m_groupModel.queryGroups(id);
            if(!groupUserVec.empty())
            {
                // "groups":[{"groupid":"xxx","groupName":"xxx","groupDesc":"xxx","users":["id":"xx","state":"xx"]}]  显示群成员时需要再多一层嵌套
                response.AddEmptySubArray("groups");
                std::vector<std::string> groupVec;
                for(Group &group : groupUserVec)
                {
                    json grouptmp;
                    grouptmp.Add("groupid",group.getId());
                    grouptmp.Add("groupName",group.getName());
                    grouptmp.Add("groupDesc",group.getDesc());
                    std::vector<std::string> userVec;
                    for(GroupUser &user : group.getUsers())
                    {
                        json usertmp;
                        usertmp.Add("id",user.getId());
                        usertmp.Add("name",user.getName());
                        usertmp.Add("state",user.getState());
                        usertmp.Add("role",user.getRole());
                        grouptmp.AddEmptySubArray("users");
                        grouptmp["users"].AddAsFirst(usertmp);
                    }
                    response["groups"].AddAsFirst(grouptmp);
                }
            }

            // std::cout << response.ToFormattedString() << std::endl;
            // int ii = 0;
            // response["group"][0].Get("groupid", ii);
            // std::cout << "ii======" << ii << std::endl;
            conn->send(response.ToString());
        }
    }
    else 
    {
        if(user.getId() != id) {
            // 用户不存在
            std::cout << " 用户不存在" << std::endl;
            json response;
            response.Add("msgid",LOGIN_MSG_ACK);
            response.Add("errno",1);
            response.Add("errmsg","该用户不存在,请重新输入新账号");
            conn->send(response.ToString());

        }else {
            // 用户存在但是密码错误
            std::cout << " 用户存在但是密码错误" << std::endl;
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
    std::string name,pwd;
    js.Get("name",name);
    js.Get("password",pwd);

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = m_userModel.insert(user);
    std::cout << state << std::endl;
    if(state)
    {
        // 注册成功
        json response;
        response.Add("msgid",REG_MSG_ACK);
        response.Add("error",0);
        response.Add("id",user.getId());
        std::cout << "response " << response.ToString() << std::endl;
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
    std::cout << "clientCloseException: m_userConnMap"<< m_userConnMap.size() << std::endl;
    User user;
    {
        std::lock_guard<std::mutex> lock(m_connMutex);
        for( auto it=m_userConnMap.begin(); it != m_userConnMap.end(); ++it)
        {
            std::cout << "id=" << it->first << std::endl;
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                m_userConnMap.erase(it);
                break;
            }
        }
    }

    // 更新用户的状态信息
    if(user.getId() != -1) // 表示有在map找到对应的用户
    {
        user.setState("offline");
        m_userModel.updateState(user);
    }
}

// 处理点对点聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    LOG_INFO << "do oneChat..." ;
    int toid;
    js.Get("toid",toid);

    {
        std::lock_guard<std::mutex> lock(m_connMutex);
        auto it = m_userConnMap.find(toid);
        if (it != m_userConnMap.end())
        {
            // toid在线，转发消息   服务器主动推送消息给toid用户
            it->second->send(js.ToString());
            return;
        }
    }
    // 存储离线信息
    m_offlineMsgModel.insert(toid,js.ToString());
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    m_userModel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do addFriend..." ;
    
    int userid,friendid;
    js.Get("id",userid);
    js.Get("friendid",friendid);

    // 存储好友信息
    m_friendModel.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do createGroup..." ;
    
    int userid;
    std::string groupName,groupDesc;
    js.Get("userid",userid);
    js.Get("groupName",groupName);
    js.Get("groupDesc",groupDesc);

    // 存储新创建的群组信息
    Group group(-1,groupName,groupDesc);   
    if(m_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        m_groupModel.addGroup(userid,group.getId(),"creator");
    }
    
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do addGroup..." ;
    int userid,groupid;
    js.Get("userid",userid);
    js.Get("groupid",groupid);

    m_groupModel.addGroup(userid,groupid,"normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do groupChat..." ;
    int userid,groupid;
    js.Get("userid",userid);
    js.Get("groupid",groupid);

    std::vector<int> useridVec = m_groupModel.queryGroupUsers(userid,groupid);
    if(useridVec.empty()){
        std::cout << "未查询到相关群信息" << std::endl;
    }
    std::lock_guard<std::mutex> lock(m_connMutex);
    for(int id : useridVec)
    {
        std::cout << "群成员:" << id << std::endl;
        auto it = m_userConnMap.find(id);
        if(it != m_userConnMap.end()) {
            // 用户在线，转发消息
            it->second->send(js.ToString());
            std::cout << "群成员:" << id << "在线,转发群信息" << std::endl;
        }else{
            // 用户不在线，存储离线消息
            m_offlineMsgModel.insert(id,js.ToString());
            std::cout << "群成员:" << id << "不在线" << std::endl;
        }
    }
}
