#include "groupmodel.h"
#include "mysql.h"
#include <iostream>
// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allGroup(groupname,groupdesc) values('%s', '%s')", 
            group.getName().c_str(),group.getDesc().c_str());
    std::cout << sql << std::endl;
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupUser values(%d, %d, '%s')",
            groupid, userid, role.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息(一个或者多个，所以返回类型是vector)以及用户自身信息(组角色以及昵称，状态)
std::vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1. 先根据userid在groupUser表中查询出该用户所属的群组id，再去allGroup表查询群组信息
    2. 再根据userid在user表查询到用户信息，在groupUser表中查询到用户在群里的角色、
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allGroup a inner join \
         groupUser b on a.id = b.groupid where b.userid=%d",
            userid);

    vector<Group> groupVec;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            // 查出userid所有的群组信息
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for(Group &group : groupVec)  
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupUser b on b.userid = a.id where b.groupid=%d",
                group.getId());

        std::cout << sql << std::endl;
        MYSQL_RES *res =  mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.gerUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，用于群聊时给群组其他用户群发信息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupUser where groupid = %d and userid != %d",groupid, userid);

    vector<int> useridVec;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            // 查出群组除自己外的所有成员id
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                useridVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return useridVec;
}