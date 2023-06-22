#include "friendmodel.h"
#include "mysql.h"
#include <iostream>

// 添加好友信息
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
 
}

// 返回用户好友列表
std::vector<User> FriendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};

    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);

    std::vector<User> msg;
    MySQL mysql;
    if( mysql.connect() )
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row ;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setName(row[0]);
                user.setState(row[2]);
                user.setId(atoi(row[3]));
                msg.push_back(user);
            }
            mysql_free_result(res);
            return msg;
        }
    }

    return msg;   
}