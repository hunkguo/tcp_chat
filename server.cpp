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
#include <list>
#include <map>
#include <time.h>
#include "message.h"
#include "logger.h"

#define PORT 7000
#define IP "127.0.0.1"
 
int s;
struct sockaddr_in servaddr;
socklen_t len;
std::list<int> li;//用list来存放套接字，没有限制套接字的容量就可以实现一个server跟若干个client通信
std::map<int, ClientInfo> client_map; // 套接字到客户端信息的映射
 
void getConn()
{
 while(1)
 {
 int conn = accept(s, (struct sockaddr*)&servaddr, &len);
 li.push_back(conn);
 
 // 初始化客户端信息
 ClientInfo default_info;
 default_info.client_id = 0;
 strcpy(default_info.client_name, "Unknown");
 client_map[conn] = default_info;
 
 LOG_INFO("新客户端连接，套接字描述符: %d", conn);
 
 // 广播新客户端连接通知给所有已连接的客户端（不包括新连接的客户端自己）
 DataMessage join_notification;
 char notification_msg[64];
 sprintf(notification_msg, "客户端 %d 已加入聊天室", conn);
 initDataMessage(&join_notification, 0, conn, 0.0, notification_msg, 0, "Server");
 join_notification.header.type = MSG_TYPE_CLIENT_JOIN; // 设置为客户端连接通知类型
 
 std::list<int>::iterator it;
 for(it = li.begin(); it != li.end(); ++it)
 {
 // 只发送给其他已连接的客户端，不发送给新连接的客户端自己
 if (*it != conn)
 {
 send(*it, &join_notification, sizeof(DataMessage), 0);
 LOG_DEBUG("已通知客户端 %d: 新客户端 %d 已连接", *it, conn);
 }
 }
 }
}
 
void getData()
{
 struct timeval tv;
 tv.tv_sec = 10;//设置倒计时时间
 tv.tv_usec = 0;
 while(1)
 {
  std::list<int>::iterator it;
  for(it=li.begin(); it!=li.end(); ++it)
  { 
    fd_set rfds; 
    FD_ZERO(&rfds);
    int maxfd = 0;
    int retval = 0;
    FD_SET(*it, &rfds);
    if(maxfd < *it)
    {
      maxfd = *it;
    }
    retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
    if(retval == -1)
    {
      LOG_ERROR("select error");
    }
    else if(retval == 0)
    {
      //LOG_DEBUG("没有收到消息");
    }
    else
    {
      // 首先接收消息头
      MessageHeader header;
      int header_len = recv(*it, &header, sizeof(MessageHeader), 0);
      
      if (header_len <= 0) {
        // 连接关闭或错误
        LOG_INFO("客户端断开连接，套接字: %d", *it);
        client_map.erase(*it);
        close(*it);
        it = li.erase(it);
        continue;
      }
      
      // 根据消息类型处理
      switch (header.type) {
        case MSG_TYPE_HEARTBEAT: {
          // 接收心跳消息
          HeartbeatMessage hb_msg;
          hb_msg.header = header;
          int body_len = recv(*it, ((char*)&hb_msg) + sizeof(MessageHeader), 
                             header.length, 0);
          
          // 更新客户端信息
          client_map[*it] = hb_msg.client_info;
          
          LOG_INFO("收到心跳: 客户端ID=%d, 名称=%s, 时间戳=%ld, 套接字=%d", 
                 hb_msg.client_info.client_id, 
                 hb_msg.client_info.client_name, 
                 hb_msg.timestamp, 
                 *it);
          break;
        }
        case MSG_TYPE_DATA: {
          // 接收数据消息
          DataMessage data_msg;
          data_msg.header = header;
          int body_len = recv(*it, ((char*)&data_msg) + sizeof(MessageHeader), 
                             header.length, 0);
          
          LOG_INFO("收到数据: 发送者=%s(ID:%d), ID=%d, 值=%d, 浮点数=%f, 字符串=%s", 
                 data_msg.sender.client_name, 
                 data_msg.sender.client_id, 
                 data_msg.id, 
                 data_msg.value, 
                 data_msg.double_value, 
                 data_msg.string_value);
          break;
        }
        case MSG_TYPE_NORMAL:
        default: {
          // 处理普通消息
          char buf[1024];
          memset(buf, 0, sizeof(buf));
          int len = recv(*it, buf, sizeof(buf), 0);
          LOG_INFO("收到普通消息: %s", buf);
          break;
        }
      }
    }
  }
  sleep(1);

 }
}
 
void sendMess()
{
 while(1)
 {
 char buf[1024];
 fgets(buf, sizeof(buf), stdin);
 
 // 创建数据消息发送给所有客户端
 DataMessage data_msg;
 initDataMessage(&data_msg, 0, 0, 0.0, buf, 0, "Server");
 
 std::list<int>::iterator it;
 for(it=li.begin(); it!=li.end(); ++it)
 {
 send(*it, &data_msg, sizeof(DataMessage), 0);
 }
 
 LOG_INFO("服务器消息已广播给所有客户端");
 }
}
 
int main()
{
  // 初始化日志文件，同时输出到控制台和文件
  if (LOG_INIT_FILE("server.log", 1) != 0) {
    fprintf(stderr, "初始化日志文件失败\n");
    return -1;
  }
  
  // 确保程序退出时关闭日志文件
    atexit(log_close_file);
  
  //new socket
  s = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  servaddr.sin_addr.s_addr = inet_addr(IP);
  if(bind(s, (struct sockaddr* ) &servaddr, sizeof(servaddr))==-1)
  {
    LOG_ERROR("bind失败: %s", strerror(errno));
  exit(1);
  }
  if(listen(s, 20) == -1)
  {
    LOG_ERROR("listen失败: %s", strerror(errno));
  exit(1);
  }
  len = sizeof(servaddr);

  //thread : while ==>> accpet
  std::thread t(getConn);
  t.detach();//detach的话后面的线程不同等前面的进程完成后才能进行，如果这里改为join则前面的线程无法判断结束，就会
  //一直等待，导致后面的线程无法进行就无法实现操作
  LOG_INFO("服务器启动成功，监听端口: %d，日志文件: server.log", PORT);
  //thread : input ==>> send
  std::thread t1(sendMess);
  t1.detach();
  //thread : recv ==>> show
  std::thread t2(getData);
    t2.detach();
    
    while(1)//做一个死循环使得主线程不会提前退出
    {

    }
    
    // 这里理论上不会执行到，但为了代码完整性保留
    LOG_INFO("服务器关闭");
    return 0;
}