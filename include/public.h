#ifndef PUBLIC_H
#define PUBLIC_H

/*
    server和client的公共文件
*/

enum EnMsgType
{
    REG_MSG = 1,     // 注册消息
    LOGIN_MSG,       // 登录消息
    REG_MSG_ACK,     // 注册消息完成
    LOGIN_MSG_ACK,   // 登录消息完成
    ONECHAT_MSG,     // 点对点聊天消息
};

#endif