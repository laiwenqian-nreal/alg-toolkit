#ifndef PROJECT_XREAL_LINK_TCP_H
#define PROJECT_XREAL_LINK_TCP_H

#include "xreal_link_common.h"
#include <arpa/inet.h> // for inet_addr
#include <fstream> // std::ofstream 头文件中需要 std::shared_ptr<std::ofstream>
#include <netinet/in.h> // for sockaddr_in, AF_INET

namespace xreal {
namespace toolkits {
namespace datadump {

// packet 0
// ////
//+----------------+
//|header(with crc)|
//+----------------+
//|     14         |
//+----------------+
// ////
//
// packet 1
// ////
//+----------------+
//|      datas     |
//+----------------+
//|  header.length |
//+----------------+
//
// aboule per_data_length = 61
//+------+-------+
//|header|payload|
//+------+-------+
//|  17  |   44  | =  61
//+------+-------+
// ////

class XrealLinkTcp : public XrealLinkCommon {
protected:
  int sockfd{-1};

  struct sockaddr_in server_addr;
  bool connected = false;
  bool connected_once = false;
  bool flag_start_collect = true;

  std::shared_ptr<std::ofstream> ofs_ptr_;
  bool tcp_log_;

  XrealLinkTcp(const char *server_address = LOCAL_ADDRESS);

public:
  ~XrealLinkTcp();

  void _Close() override;

  // CRC8校验算法 - 继承自基类
  // uint8_t crc8(uint8_t *data, int size) - 已在基类中

  // 时间格式化函数 - 继承自基类
  // void getNowTimeFormat(char *time_format) - 已在基类中

  bool connectServer();

  void closeSockfd();

  static void Close();

  // 其他工具函数使用基类实现
  // static void msgInit() - 已在基类中
  // static void msgSetBuf() - 已在基类中

  static XrealLinkTcp *getInstance(const char *addr);
  static XrealLinkTcp *getInstance();

  static int setServerAddr(const char *addr);

public:
  // 静态接口函数 - 显式声明避免链接问题
  static void linkSendStatus(int group_id, int msg_id, const uint8_t *data,
                             uint64_t len);
  
  template <typename T>
  static void linkSendStatus(int group_id, int msg_id,
                             const std::vector<T> &vec) {
    size_t data_size = vec.size() * sizeof(T);
    linkSendStatus(group_id, msg_id, (const uint8_t *)vec.data(), data_size);
  }

  // TCP特有的标志位重载
  void messageEnQueue(DataBuffer &msg) override;

  // 重载基类的processGroupMsg - TCP版本：包装成TCP包并发送
  void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                       uint64_t timestamp_ns, uint64_t packet_id) override;

private:
  // 更新组消息中所有消息头的时间戳为入send队列时间
  void updateGroupMsgTimestamps(DataBuffer &group_msg);

protected:
  // 实现基类的纯虚函数 - TCP专用的发送线程
  void sendThread() override;

  void setPacketHeader(DataBuffer &msg_header, uint32_t freq_count,
                       uint32_t data_length, uint64_t timestamp_ns,
                       uint64_t packet_id);

  void setPacketData(DataBuffer &group_msg, DataBuffer &msg);

  bool tcpIpSendPacketMsg(DataBuffer msg);

  bool tcpIpSendMsg(DataBuffer msg);
}; // namespace toolkits
} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif
