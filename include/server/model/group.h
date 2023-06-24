#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
#include "user.h"

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
class GroupUser : public User
{
public:
    void setRole(std::string role) { this->role = role; }
    std::string getRole() { return this->role; }

private:
    std::string role;
};

// Group表的ORM表
class Group
{
public:
    Group(int id = -1,std::string name = "",std::string desc = "")
    {
        m_id = id;
        m_name = name;
        m_desc = desc;
    }

    void setId(int id) { m_id = id; }
    void setName(std::string name) { m_name = name; }
    void setDesc(std::string desc) { m_desc = desc; }

    int getId() { return m_id; }
    std::string getName() { return m_name; }
    std::string getDesc() { return m_desc; }
    std::vector<GroupUser> &getUsers() { return m_users; }

private:
    int m_id;
    std::string m_name;
    std::string m_desc;
    std::vector<GroupUser> m_users;   // 存储所有群成员信息
};

#endif