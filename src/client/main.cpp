#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.h"
#include "user.h"
#include "public.h"
#include "CJsonObject.hpp"
#include "string.h"

using json = neb::CJsonObject;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 记录登录状态
std::atomic_bool g_isLoginSuccess{false};

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if(argc < 3){
        std::cerr << "comannd invalid! example: ./ChatClient 127.0.0.1 6000" << std::endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(clientfd == -1){
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in))){
        std::cerr << "connect server error" << std::endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用户接收用户输入，负责发送数据
    while(1)
    {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "==============================" << std::endl;
        std::cout << "1.register" << std::endl;
        std::cout << "2.login" << std::endl;
        std::cout << "3.quit" << std::endl;
        std::cout << "==============================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice;
        std::cin.get();     // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1:          // 注册业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                std::cout << "username:";
                std::cin.getline(name,50);
                std::cout << "userpassword:";
                std::cin.getline(pwd,50);     // 这里不用cin << 是因为这个是和scanf一样，遇到空格结束读取

                json js;
                js.Add("msgid",REG_MSG);
                js.Add("name",name);
                js.Add("password",pwd);
                std::string request = js.ToString();
                int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
                if(len == -1){
                    std::cerr << "send register msg error:" << request << std::endl;
                }else{
                    char buffer[1024] = {0};
                    len = recv(clientfd,buffer,1024,0);
                    if(len == -1){
                        std::cerr << "recv register response error" << std::endl;
                    }else{
                        json responsejs(buffer);
                        int error;
                        responsejs.Get("error",error);

                        if(error != 0){  // 注册失败
                            std::cerr << name << " is already exist, register error!" << std::endl;
                        }else{           // 注册成功  
                            int id;
                            responsejs.Get("id",id);
                            std::cout << name << " register success, userid is " << id << ", do not forget it!" << std::endl;
                        }                      
                    }
                }
            }
            break;
        case 2:         // 登录业务
            {
                int id = 0;
                char pwd[50] = {0};
                std::cout << "userid:";
                std::cin >> id;
                std::cin.get();     // 读掉缓冲区残留的回车
                std::cout << "userpassword:";
                std::cin.getline(pwd,50);

                json js;
                js.Add("msgid",LOGIN_MSG);
                js.Add("id",id);
                js.Add("password",pwd);
                std::string request = js.ToString();

                int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
                if(len == -1){
                    std::cerr << "send login msg error:" << request << std::endl;
                }else{

                    char buffer[1024] = {0};
                    std::cout << "等待回复..." << std::endl;
                    len = recv(clientfd,buffer,1024,0);   // 发送成功之后阻塞等待
                    std::cout << "收到回复" << std::endl;
                    if(len == -1){
                        std::cerr << "recv login response error" << std::endl;
                    }else{
                        json responsejs(buffer);
                        int errnumber;
                        responsejs.Get("errno",errnumber);
                        std::cout << responsejs.ToFormattedString() << std::endl;
                        
                        if(errnumber != 0){  // 登录失败

                            std::string errormsg;
                            responsejs.Get("errmsg",errormsg);
                            std::cerr << errormsg << std::endl;
                            g_isLoginSuccess = false;

                        }else{  // 登录成功
                            g_isLoginSuccess = true;
                            // 记录当前用户的id和name
                            int id;
                            std::string name;
                            responsejs.Get("id",id);
                            responsejs.Get("name",name);
                            g_currentUser.setId(id);
                            g_currentUser.setName(name);

                            // 记录当前用户的好友列表信息
                            if(responsejs.IsNull("friends") == false){
                                int friendSize = responsejs["friends"].GetArraySize();
                                for(int ii = 0; ii < friendSize; ii++)
                                {
                                    User user;
                                    int friendId;
                                    std::string friendName,friendState;
                                    responsejs["friends"][ii].Get("id",friendId);
                                    responsejs["friends"][ii].Get("name",friendName);
                                    responsejs["friends"][ii].Get("state",friendState);
                                    user.setId(friendId);
                                    user.setName(friendName);
                                    user.setState(friendState);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            // 记录当前用户的群组列表信息
                            if(responsejs.IsNull("groups") == false){

                                int groupSize = responsejs["groups"].GetArraySize();
                                for(int ii = 0; ii < groupSize; ii++)
                                {
                                    Group group;
                                   
                                    int groupId;
                                    std::string groupName,groupDesc;
                                    responsejs["groups"][ii].Get("groupid",groupId);
                                    responsejs["groups"][ii].Get("groupName",groupName);
                                    responsejs["groups"][ii].Get("groupDesc",groupDesc);
                                    group.setId(groupId);
                                    group.setName(groupName);
                                    group.setDesc(groupDesc);

                                    int gourpUserSize = responsejs["groups"][ii]["users"].GetArraySize();
                                    for(int jj =0; jj < gourpUserSize; jj++)
                                    {
                                        GroupUser groupUser;
                                        int groupUserid;
                                        std::string groupUserName,groupUserState,groupUserRole;
                                        responsejs["groups"][ii]["users"][jj].Get("id",groupUserid);
                                        responsejs["groups"][ii]["users"][jj].Get("name",groupUserName);
                                        responsejs["groups"][ii]["users"][jj].Get("state",groupUserState);
                                        responsejs["groups"][ii]["users"][jj].Get("role",groupUserRole);
                                        groupUser.setId(groupUserid);
                                        groupUser.setName(groupUserName);
                                        groupUser.setState(groupUserState);    
                                        groupUser.setRole(groupUserRole);
                                        group.getUsers().push_back(groupUser);    // 存储群成员信息                            
                                    }
                                    
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            // 显示登录用户的基本信息
                            if(g_isLoginSuccess == true)
                                showCurrentUserData();

                            // 显示当前用户的离线消息(个人聊天信息或者群组信息)
                            if(responsejs.IsNull("offlinemessage") == false){
                                int friendSize = responsejs["offlinemessage"].GetArraySize();
                                for(int ii = 0; ii < friendSize; ii++)
                                {   
                                    int id;
                                    std::string name,msg,time;
                                    responsejs["offlinemessage"][ii].Get("id",id);
                                    responsejs["offlinemessage"][ii].Get("name",name);
                                    responsejs["offlinemessage"][ii].Get("msg",msg);
                                    responsejs["offlinemessage"][ii].Get("time",time);
                                    
                                    std::cout << time << " [" << id << "]" << name << " said: " << msg << std::endl;
                                }
                            }

                            // 登录成功，启动接收线程负责接收数据
                            std::thread readTask(readTaskHandler,clientfd);
                            readTask.detach();  // 线程分离，让系统自动回收线程退出资源

                            // 进入聊天主菜单页面
                            isMainMenuRunning = true;
                            mainMenu(clientfd);
                        }
                    }
                }

            }
            break;
        case 3:      // 退出
            close(clientfd);
            exit(0);
        default:
            break;
        }
    }

    return 0;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    while(1)
    {
        std::cout << "readTaskHandler" << std::endl;
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 阻塞了
        if((len == -1) || (len == 0)){
            close(clientfd);
            std::cerr << "连接异常！退出程序" << std::endl;
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json responsejs(buffer);
        int msgtype;
        responsejs.Get("msgid",msgtype);

        if (ONE_CHAT_MSG == msgtype)
        {
            int fromid;
            std::string msg;
            responsejs.Get("fromid",fromid);
            responsejs.Get("msg",msg);
            std::cout  << " 好友消息[" << fromid << "]" << " said: " << msg << std::endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            int userid;
            std::string msg;
            responsejs.Get("userid",userid);
            responsejs.Get("msg",msg);
            std::cout << "群消息[" << userid << "]:" << " said: " << msg << std::endl;
            continue;
        }
#if 0
        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成
            continue;
        }
#endif
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    std::cout << "======================login user======================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "----------------------friend list---------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "----------------------group list----------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "======================================================" << std::endl;    
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "char" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void loginout(int, std::string);

// 存储系统帮助help显示的所有命令
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令,格式为: help"},
    {"chat", "一对一聊天,格式为: chat:friendid:message"},
    {"addfriend", "添加好友,格式为: addfriend:friendid"},
    {"creategroup", "创建群组,格式为: creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式为: addgroup:groupid"},
    {"groupchat", "群聊,格式为: groupchat:groupid:message"},
    {"loginout", "注销,格式为: loginout"}};

// 存储系统支持的客户端命令以及对应的处理函数
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        std::cin.getline(buffer,1024);
        std::string commandbuf(buffer);
        std::string command;           // 存储命令
        int idx = commandbuf.find(":");
        if(idx == -1){
            command = commandbuf;
        }else{
            command = commandbuf.substr(0,idx);
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it -> second(clientfd,commandbuf.substr(idx+1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for(auto &p : commandMap)
    {
        std::cout << p.first << ":" << p.second << std::endl;
    }
    std::cout << std::endl;
}

// "addfriend" command handler
// addfriend:friendid
// {"msgid":6,"id":17,"friendid":19} 
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js.Add("msgid",ADD_FRIEND_MSG);
    js.Add("id",g_currentUser.getId());
    js.Add("friendid",friendid);
    std::string request = js.ToString();
    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << request << std::endl;
    }
}

// "chat" command handler
// chat:friendid:message
// {"msgid":5,"fromid":18,"toid":17,"msg":"hello2225"}  
void chat(int clientfd, std::string str)
{
    int idx = str.find(":");  // friendid:message
    if(idx == -1){
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }
    int friendid = atoi(str.substr(0,idx).c_str());
    std::string message =  str.substr(idx + 1, str.size() - idx);

    json js;
    js.Add("msgid",ONE_CHAT_MSG);
    js.Add("fromid",g_currentUser.getId());
    js.Add("toid",friendid);
    js.Add("msg",message);
    js.Add("time",getCurrentTime());
    std::string request = js.ToString();

    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << request << std::endl;
    }
}

// "creategroup" command handler
// creategroup:groupname:groupdesc
// {"msgid":7,"userid":17,"groupName":"三傻联盟","groupDesc":"三个活宝的群"}  
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");  // groupname:groupdesc
    if(idx == -1){
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js.Add("msgid",CREATE_GROUP_MSG);
    js.Add("userid",g_currentUser.getId());
    js.Add("groupName",groupname);
    js.Add("groupDesc",groupdesc);
    std::string request = js.ToString();
    
    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << request << std::endl;
    }
}

// "addgroup" command handler
// addgroup:groupid
// {"msgid":8,"userid":18,"groupid":1}
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js.Add("msgid",ADD_GROUP_MSG);
    js.Add("userid",g_currentUser.getId());
    js.Add("groupid",groupid);
    std::string request = js.ToString();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << request << std::endl;
    }
}

// "groupchat" command handler
// groupchat:groupid:message
// {"msgid":9,"userid":18,"groupid":3,"msg":"hhhhh"}  
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");  // groupid:message
    if(idx == -1){
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js.Add("msgid",GROUP_CHAT_MSG);
    js.Add("userid",g_currentUser.getId());
    js.Add("groupid",groupid);
    js.Add("msg",message);
    std::string request = js.ToString();
    
    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << request << std::endl;
    }
}

// "loginout" command handler
// loginout
void loginout(int clientfd, std::string str)   // 这里加上形参的目的是配合function函数使用
{
    json js;
    js.Add("msgid",LOGINOUT_MSG);
    js.Add("userid",g_currentUser.getId());
    std::string request = js.ToString();
    
    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1, 0);
    if(len == -1){
        std::cerr << "send addfriend msg error -> " << request << std::endl;
    }else{
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}