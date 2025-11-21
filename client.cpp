#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <time.h>
#include "message.h"

#define MYPORT 7000
#define BUFFER_SIZE 1024

// 全局变量
int sock_cli = -1;
int client_id = 0;
char client_name[32] = {0};
bool running = true;

// 心跳线程函数
void heartbeatThread() {
    while (running) {
        // 等待60秒（测试用，正常应该是30秒）
        sleep(60);
        
        if (sock_cli != -1) {
            // 创建心跳消息
            HeartbeatMessage hb_msg;
            initHeartbeatMessage(&hb_msg, client_id, client_name);
            
            // 发送心跳消息
            send(sock_cli, &hb_msg, sizeof(HeartbeatMessage), 0);
            printf("心跳发送: 客户端ID=%d, 名称=%s\n", client_id, client_name);
        }
    }
}

// 发送数据结构体函数
void sendDataMessage(int id, int value, double double_value, const char* string_value) {
    if (sock_cli != -1) {
        DataMessage data_msg;
        initDataMessage(&data_msg, id, value, double_value, string_value, client_id, client_name);
        
        send(sock_cli, &data_msg, sizeof(DataMessage), 0);
        printf("数据发送: ID=%d, 值=%d, 浮点数=%f, 字符串=%s\n", 
               id, value, double_value, string_value);
    }
}

int main()
{
  fd_set rfds;
  struct timeval tv;
  int retval, maxfd;

  ///定义sockfd
  sock_cli = socket(AF_INET,SOCK_STREAM, 0);
  ///定义sockaddr_in
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(MYPORT); ///服务器端口
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); ///服务器ip

  //连接服务器，成功返回0，错误返回-1
  if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
  {
    perror("connect");
    exit(1);
  }
  
  // 生成客户端标识 - 使用时间作为随机种子确保唯一性
  srand(time(NULL)); // 设置随机种子
  client_id = rand(); // 使用随机数作为客户端ID
  sprintf(client_name, "Client_%d", client_id);
  printf("客户端已连接，ID=%d, 名称=%s\n", client_id, client_name);
  
  // 启动心跳线程
  std::thread hb_thread(heartbeatThread);
  hb_thread.detach();

  while(1){
    /*把可读文件描述符的集合清空*/
    FD_ZERO(&rfds);
    /*把标准输入的文件描述符加入到集合中*/
    FD_SET(0, &rfds);
    maxfd = 0;
    /*把当前连接的文件描述符加入到集合中*/
    FD_SET(sock_cli, &rfds);
    /*找出文件描述符集合中最大的文件描述符*/ 
    if(maxfd < sock_cli)
    maxfd = sock_cli;
    /*设置超时时间*/
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    /*等待聊天*/
    retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
    if(retval == -1){
      printf("select出错，客户端程序退出\n");
      break;
    }else if(retval == 0){
      // printf("客户端没有任何输入信息，并且服务器也没有信息到来，waiting...\n");
      continue;
    }else{
      /*服务器发来了消息*/
      if(FD_ISSET(sock_cli,&rfds)){
        // 首先接收消息头
        MessageHeader header;
        int header_len = recv(sock_cli, &header, sizeof(MessageHeader), 0);
        
        if (header_len <= 0) {
          printf("服务器连接断开\n");
          break;
        }
        
        printf("收到消息类型: %d\n", header.type); // 添加调试信息
        
        // 根据消息类型处理
        switch (header.type) {
          case MSG_TYPE_CLIENT_JOIN: {
            // 处理客户端加入通知
            DataMessage join_msg;
            join_msg.header = header;
            int body_len = recv(sock_cli, ((char*)&join_msg) + sizeof(MessageHeader), 
                              header.length, 0);
            
            printf("[系统通知] %s\n", join_msg.string_value);
            break;
          }
          case MSG_TYPE_DATA: {
            // 处理数据消息
            DataMessage data_msg;
            data_msg.header = header;
            int body_len = recv(sock_cli, ((char*)&data_msg) + sizeof(MessageHeader), 
                              header.length, 0);
            
            printf("[来自 %s (ID:%d)] %s", 
                   data_msg.sender.client_name, 
                   data_msg.sender.client_id, 
                   data_msg.string_value);
            break;
          }
          case MSG_TYPE_NORMAL:
          default: {
            // 处理普通消息
            char recvbuf[BUFFER_SIZE];
            memset(recvbuf, 0, sizeof(recvbuf));
            int len = recv(sock_cli, recvbuf, sizeof(recvbuf), 0);
            printf("%s", recvbuf);
            break;
          }
        }
      }
      /*用户输入信息了,开始处理信息并发送*/
      if(FD_ISSET(0, &rfds)){
        char sendbuf[BUFFER_SIZE];
        fgets(sendbuf, sizeof(sendbuf), stdin);
        
        // 简单的命令处理：如果以"data:"开头，则发送数据结构体
        if (strncmp(sendbuf, "data:", 5) == 0) {
            // 解析简单的数据格式: data:id,value,double_value,string
            int id = 0, value = 0;
            double double_value = 0.0;
            char string_value[64] = {0};
            
            // 使用更简单的解析方式，避免转义字符问题
            char* token = strtok(sendbuf + 5, ",");
            if (token) id = atoi(token);
            
            token = strtok(NULL, ",");
            if (token) value = atoi(token);
            
            token = strtok(NULL, ",");
            if (token) double_value = atof(token);
            
            token = strtok(NULL, "\n");
            if (token) strncpy(string_value, token, sizeof(string_value) - 1);
            
            sendDataMessage(id, value, double_value, string_value);
        } else {
            // 普通文本消息，包装成DataMessage发送
            DataMessage data_msg;
            initDataMessage(&data_msg, 0, 0, 0.0, sendbuf, client_id, client_name);
            send(sock_cli, &data_msg, sizeof(DataMessage), 0);
        }
        
        memset(sendbuf, 0, sizeof(sendbuf));
      }
    }
  }

  // 清理资源
  running = false;
  sleep(1); // 等待心跳线程结束
  close(sock_cli);
  return 0;
}