#include "offlinemessagemodel.h"
#include "mysql.h"
#include <iostream>

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, std::string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlineMessage values(%d, '%s')", userid, msg.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
 
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlineMessage where userid=%d", userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlineMessage where userid = %d", userid);
    std::vector<std::string> msg;
    MySQL mysql;
    if( mysql.connect() )
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row ;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                msg.push_back(row[0]);
            }
            mysql_free_result(res);
            return msg;
        }
    }

    return msg;   
}