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

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();


// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 主聊天页面程序
void mainMenu();

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
                std::cin.getline(pwd,50);

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
                    len = recv(clientfd,buffer,1024,0);   // 发送成功之后阻塞等待

                    if(len == -1){
                        std::cerr << "recv login response error" << std::endl;
                    }else{
                        json responsejs(buffer);
                        int error = responsejs.Get("error",error);
                        std::cout << responsejs.ToFormattedString() << std::endl;
                        
                        if(error != 0){  //登录失败

                            std::string errormsg;
                            responsejs.Get("errmsg",errormsg);
                            std::cerr << errormsg << std::endl;

                        }else{  // 登录成功

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
                            showCurrentUserData();

                            // 显示当前用户的离线消息(个人聊天信息或者群组信息)
                            if(responsejs.IsNull("offlinemsg") == false){
                                int friendSize = responsejs["offlinemsg"].GetArraySize();
                                for(int ii = 0; ii < friendSize; ii++)
                                {   
                                    int id;
                                    std::string name,msg,time;
                                    responsejs["offlinemsg"][ii].Get("id",id);
                                    responsejs["offlinemsg"][ii].Get("name",name);
                                    responsejs["offlinemsg"][ii].Get("msg",msg);
                                    responsejs["offlinemsg"][ii].Get("time",time);
                                    
                                    std::cout << time << " [" << id << "]" << name << " said: " << msg << std::endl;
                                }
                            }

                            // 登录成功，启动接收线程负责接收数据
                            std::thread readTask(readTaskHandler,clientfd);
                            readTask.detach();  // 线程分离，让系统自动回收线程退出资源

                            // 进入聊天主菜单页面
                            mainMenu();
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

}

// 主聊天页面程序
void mainMenu()
{

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