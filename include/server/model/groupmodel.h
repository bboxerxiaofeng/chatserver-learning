#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <string>
#include <vector>
#include "group.h"

// 维护群组信息的操作接口方法
class GroupModel 
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    void addGroup(int userid, int groupid, std::string role);

    // // 查询用户所在群组信息(一个或者多个，所以返回类型是vector)以及用户自身信息(组角色以及昵称，状态)
    std::vector<Group> queryGroups(int userid);

    // 根据指定的groupid查询群组用户id列表，除userid自己，用于群聊时给群组其他用户群发信息
    std::vector<int> queryGroupUsers(int userid, int groupid);
};


#endif