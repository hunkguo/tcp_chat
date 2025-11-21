#ifndef MESSAGE_H
#define MESSAGE_H

#include <string.h>

// 消息类型枚举
enum MessageType {
    MSG_TYPE_NORMAL = 0,  // 普通消息
    MSG_TYPE_HEARTBEAT = 1, // 心跳消息
    MSG_TYPE_DATA = 2,    // 数据结构体消息
    MSG_TYPE_CLIENT_JOIN = 3 // 客户端连接通知
};

// 客户端信息结构体
typedef struct {
    int client_id;        // 客户端ID
    char client_name[32]; // 客户端名称
} ClientInfo;

// 基本消息头
typedef struct {
    MessageType type;     // 消息类型
    int length;           // 消息体长度
} MessageHeader;

// 心跳消息结构体
typedef struct {
    MessageHeader header;
    ClientInfo client_info;
    long timestamp;       // 时间戳
} HeartbeatMessage;

// 数据传输结构体
typedef struct {
    MessageHeader header;
    int id;               // 数据ID
    int value;            // 整数值
    double double_value;  // 浮点数值
    char string_value[64]; // 字符串值
    ClientInfo sender;    // 发送者信息
} DataMessage;

// 初始化心跳消息的函数
inline void initHeartbeatMessage(HeartbeatMessage* msg, int client_id, const char* client_name) {
    msg->header.type = MSG_TYPE_HEARTBEAT;
    msg->header.length = sizeof(HeartbeatMessage) - sizeof(MessageHeader);
    msg->client_info.client_id = client_id;
    strncpy(msg->client_info.client_name, client_name, sizeof(msg->client_info.client_name) - 1);
    msg->timestamp = time(NULL);
}

// 初始化数据消息的函数
inline void initDataMessage(DataMessage* msg, int id, int value, double double_value, const char* string_value, int client_id, const char* client_name) {
    msg->header.type = MSG_TYPE_DATA;
    msg->header.length = sizeof(DataMessage) - sizeof(MessageHeader);
    msg->id = id;
    msg->value = value;
    msg->double_value = double_value;
    strncpy(msg->string_value, string_value, sizeof(msg->string_value) - 1);
    msg->sender.client_id = client_id;
    strncpy(msg->sender.client_name, client_name, sizeof(msg->sender.client_name) - 1);
}

#endif // MESSAGE_H