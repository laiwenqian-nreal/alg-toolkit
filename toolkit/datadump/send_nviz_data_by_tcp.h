#ifndef PROJECT_XREAL_LINK_TCP_H
#define PROJECT_XREAL_LINK_TCP_H

#include "utils/datadump/xreal_link_common.h"
#include <errno.h> /* EINPROGRESS, errno */

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include <fstream> // std::ofstream

#include "utils/logging.h"

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

  XrealLinkTcp(const char *server_address = LOCAL_ADDRESS,
               const bool tcp_log = false) {
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_address);
    server_addr.sin_port = htons(SERVER_PORT_TCP);
    tcp_log_ = tcp_log;
    log_prefix = "[xreal_link_tcp]";
  }

public:
  ~XrealLinkTcp() { _Close(); }

  void _Close() override {
    cleanupThreads();

    if (-1 != sockfd) {
#ifdef _WIN32
      closesocket(sockfd);
      WSACleanup();
#else
      close(sockfd);
#endif
      connected = false;
      sockfd = -1;
    }
  }

  // CRC8校验算法 - 继承自基类
  // uint8_t crc8(uint8_t *data, int size) - 已在基类中

  // 时间格式化函数 - 继承自基类
  // void getNowTimeFormat(char *time_format) - 已在基类中

  bool connectServer() {
    TRACKING_LOG_TRACE("{} connected:{} in connectServer. {} {}", log_prefix,
                       connected, __FILE__, __LINE__);

    if (connected) {
      return true;
    }

    char time_format[80];
    getNowTimeFormat(time_format);

    TRACKING_LOG_TRACE("{} sockfd:{} addr:{} port:{} in connectServer. {} {}",
                       log_prefix, sockfd, inet_ntoa(server_addr.sin_addr),
                       ntohs(server_addr.sin_port), __FILE__, __LINE__);

    if (-1 == sockfd) {
#ifdef _WIN32
      /// Winsows uses wsasocket
      WSADATA wsadata;
      if (WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR) {
        printf("WSAStartup() fail\n");
        exit(0);
      }
#endif
      sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      TRACKING_LOG_WARN("{} new sockfd:{} addr:{}  port:{} in connectServer.",
                        log_prefix, sockfd, inet_ntoa(server_addr.sin_addr),
                        ntohs(server_addr.sin_port));
      if (tcp_log_) {
        *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                  << "new sockfd:" << sockfd << " in connectServer."
                  << "\n";
      }
    }

    TRACKING_LOG_TRACE("{} sockfd:{} in connectServer. {} {}", log_prefix,
                       sockfd, __FILE__, __LINE__);
    int err =
        connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    TRACKING_LOG_TRACE("{} err:{} in connectServer. {} {}", log_prefix, err,
                       __FILE__, __LINE__);
    if (-1 == err) {
      TRACKING_LOG_WARN(
          "{} new sockfd connect failed. sockfd:{} addr:{}  port:{} in "
          "connectServer. errno:{} strerror:{}",
          log_prefix, sockfd, inet_ntoa(server_addr.sin_addr),
          ntohs(server_addr.sin_port), errno, strerror(errno));
      if (tcp_log_) {
        *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                  << "connect failed. sockfd:" << sockfd
                  << " addr:" << inet_ntoa(server_addr.sin_addr)
                  << " port:" << ntohs(server_addr.sin_port)
                  << " in connectServer."
                  << "\n";
      }

      return false;
    }

    connected = true;
    connected_once = true;
    TRACKING_LOG_TRACE("{} connected:{} in connectServer. {} {}", log_prefix,
                       connected, __FILE__, __LINE__);
    return true;
  }

  void closeSockfd() {
    if (-1 != sockfd) {
#ifdef _WIN32
      closesocket(sockfd);
      WSACleanup();
#else
      close(sockfd);
#endif
      connected = false;
      sockfd = -1;
    }
  }

  static void Close() { XrealLinkTcp::getInstance()->_Close(); }

  // 使用基类的工具函数，覆盖默认时间戳获取方式
  static void msgSetTime(XrealLinkMsgHeader &_msg, uint64_t _ts) {
    if (!_ts) {
      _msg.time_stamp = get_current_timestamp_us();
    } else {
      _msg.time_stamp = _ts;
    }
  }

  // 其他工具函数使用基类实现
  // static void msgInit() - 已在基类中
  // static void msgSetBuf() - 已在基类中

public:
  // 使用宏实现linkSendStatus，避免重复代码
  IMPLEMENT_LINK_SEND_STATUS(XrealLinkTcp)

  // TCP特有的标志位重载
  void messageEnQueue(DataBuffer &msg) override {
    if (true == flag_start_collect) {
      XrealLinkCommon::messageEnQueue(msg); // 调用基类实现
    }
  }

  // 重载基类的processGroupMsg - TCP版本：包装成TCP包并发送
  void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                       uint64_t timestamp_ns, uint64_t packet_id) override {
    // 更新组消息中每个SimpleMessageHeader的时间戳为入send队列时间
    updateGroupMsgTimestamps(group_msg);

    // 使用入send队列的时间作为TCP包头的时间戳
    uint64_t send_queue_timestamp_ns =
        get_current_timestamp_ns(); // 纳秒级时间戳

    DataBuffer msg_header;
    setPacketHeader(msg_header, freq_count, group_msg.size(),
                    send_queue_timestamp_ns, packet_id);
    concurrent_lock_free_queue_send_.enqueue(msg_header);

    DataBuffer msg_data;
    setPacketData(group_msg, msg_data);
    concurrent_lock_free_queue_send_.enqueue(msg_data);
  }

private:
  // 更新组消息中所有消息头的时间戳为入send队列时间
  void updateGroupMsgTimestamps(DataBuffer &group_msg) {
    uint64_t send_queue_timestamp =
        get_current_timestamp_us(); // 入send队列的时间戳

    int curLen = group_msg.size();
    uint8_t *curDataPtr = group_msg.data();

    while (curLen > 0) {
      if (curLen < sizeof(SimpleMessageHeader)) {
        break;
      }

      SimpleMessageHeader *msg_header = (SimpleMessageHeader *)curDataPtr;

      // 更新时间戳为入send队列的时间
      msg_header->time_stamp = send_queue_timestamp;

      curDataPtr += sizeof(SimpleMessageHeader);
      curLen -= sizeof(SimpleMessageHeader);

      int32_t payload_len = msg_header->payload_length;
      if (curLen < payload_len) {
        break;
      }

      curDataPtr += payload_len;
      curLen -= payload_len;
    }
  }

protected:
  // 实现基类的纯虚函数 - TCP专用的发送线程
  void sendThread() override {
    DataBuffer msg;

    // first ensure that the server can be connected
    while (false == connectServer()) {
      // drop msg at first while connect server failed
      concurrent_lock_free_queue_send_.try_dequeue(msg);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1s
    }

    while (send_thread_running) {
      while (concurrent_lock_free_queue_send_.try_dequeue(msg)) {
        while (false == XrealLinkTcp::getInstance()->tcpIpSendMsg(msg)) {
          continue;
        }
      }

      // if empty in queue, sleep for performance
      if (0 == concurrent_lock_free_queue_send_.size_approx()) {
        //  sleep 0.1s , because of the data that accumulates for 1 second.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

  void setPacketHeader(DataBuffer &msg_header, uint32_t freq_count,
                       uint32_t data_length, uint64_t timestamp_ns,
                       uint64_t packet_id) {
  TcpMsgHeader header;
  header.version = 1;
  header.magic_num = TCP_MAGIC_NUM;
  header.serialize_method = 0; // pack-unpack
  header.service_num = 1;      // SEND
  header.msg_type = MsgContentType::SENSOR_DATA;
  header.msg_count = freq_count;
  header.length = sizeof(TcpMsgHeader) + data_length;
  header.timestamp_ns = timestamp_ns;
  header.packet_id = packet_id;
  header.crc = 0;
  uint8_t crc = crc8((uint8_t *)&header, sizeof(TcpMsgHeader));
  header.crc = crc;

  TcpPacketMsgHeader tcp_packet_msg_header = {.header = 0};
  tcp_packet_msg_header.data.submsg_type = PacketMsgType::SUBMSG_TYPE_HEADER;
  msg_header.emplace_back(tcp_packet_msg_header.header);

  // 添加完整的header数据
  size_t old_size = msg_header.size();
  msg_header.resize(old_size + sizeof(TcpMsgHeader));
  memcpy(msg_header.data() + old_size, &header, sizeof(TcpMsgHeader));
}

void setPacketData(DataBuffer &group_msg, DataBuffer &msg) {
  TcpPacketMsgHeader tcp_packet_msg_header = {.header = 0};
  tcp_packet_msg_header.data.submsg_type = PacketMsgType::SUBMSG_TYPE_DATA;

  msg.emplace_back(tcp_packet_msg_header.header);
  msg.insert(msg.end(), group_msg.begin(), group_msg.end());
}

bool tcpIpSendPacketMsg(DataBuffer msg) {
  if (msg.size() <= 0) {
    TRACKING_LOG_WARN("{} msg.size() <= 1 in tcpIpSendPacketMsg.", log_prefix);
    return true;
  }
  char time_format[80];
  getNowTimeFormat(time_format);

  if (send(sockfd, (const char *)msg.data(), msg.size(), 0) >= 0) {
    char receiveMessage[100] = {};
    if (recv(sockfd, receiveMessage, sizeof(receiveMessage), 0) >= 0) {
      if (0 == strncmp(receiveMessage, "ok", 2)) {
        std::string receiveString = receiveMessage;
        std::string delimiter = ",";
        std::string actionInfo;

        size_t pos = receiveString.find(delimiter, 2);
        if (pos != std::string::npos) {
          actionInfo = receiveString.substr(pos + 1);
        }
        if ("stop_collect" == actionInfo) {
          flag_start_collect = false;
        }

        return true;
      } else if ("" == receiveMessage) { // There may be a problem with Nviz
        TRACKING_LOG_WARN("{} bad receiveMessage content 1. content[{}] "
                          "errno:{} strerror:{}",
                          log_prefix, receiveMessage, errno, strerror(errno));

        if (tcp_log_) {
          *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                    << "bad receiveMessage content 1. content["
                    << receiveMessage << "] errno:" << errno
                    << " strerr:" << strerror(errno) << "\n";
        }

        return false;
      } else {
        TRACKING_LOG_WARN("{} bad receiveMessage content 2. content[{}] "
                          "errno:{} strerror:{}",
                          log_prefix, receiveMessage, errno, strerror(errno));
        if (tcp_log_) {
          *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                    << "bad receiveMessage content 2. content["
                    << receiveMessage << "] errno:" << errno
                    << " strerr:" << strerror(errno) << "\n";
        }

        return false;
      }
    } else {
      TRACKING_LOG_WARN("{} tcpip recv fail. errno:{} strerror:{}", log_prefix,
                        errno, strerror(errno));
      if (tcp_log_) {
        *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                  << "tcpip recv fail. errno:" << errno
                  << " strerr:" << strerror(errno) << "\n";
      }
      return false;
    }

  } else {
    TRACKING_LOG_WARN("{} tcpip send fail. msg_len:{}  errno:{} strerror:{}",
                      log_prefix, msg.size(), errno, strerror(errno));
    if (tcp_log_) {
      *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                << "tcpip send fail. msg_len" << msg.size()
                << ". errno:" << errno << " strerr:" << strerror(errno) << "\n";
    }

    if (EPIPE == errno) { // EPIPE = 32, strerror is "Broken Pipe"
      closeSockfd();
      if (connectServer()) {
        TRACKING_LOG_WARN("{} reconnect server by errno:{} strerror:{}",
                          log_prefix, errno, strerror(errno));
        if (tcp_log_) {
          *ofs_ptr_ << "[" << time_format << "]" << log_prefix
                    << "reconnect server by errno:" << errno
                    << " strerr:" << strerror(errno) << "\n";
        }
      }
    }

    TRACKING_LOG_WARN("{} return false in tcpIpSendPacketMsg.", log_prefix);
    return false;
  }
}

bool tcpIpSendMsg(DataBuffer msg) {
  if (connectServer()) {
    int packet_msg_header_length = sizeof(TcpPacketMsgHeader);
    if (msg.size() <= packet_msg_header_length) {
      TRACKING_LOG_WARN(
          "{} tcpIpSendMsg fail return true. reason: msg.size() <= "
          "packet_msg_header_length.  msg.size:{} "
          "packet_msg_header_length:{}",
          log_prefix, msg.size(), packet_msg_header_length);
      return true;
    }

    union TcpPacketMsgHeader tcp_packet_msg_header = {.header = 0};
    tcp_packet_msg_header.header = msg[0];
    int msg_length = msg.size() - packet_msg_header_length;

    int buffer_size = TCP_PACKET_MAX_SIZE - packet_msg_header_length;
    buffer_size = std::min<int>(buffer_size, msg_length);
    int current_index =
        0 + packet_msg_header_length; // not start from 0, we will strip
                                      // tcp_packet_msg_header
    int all_length = msg_length - buffer_size;

    int i = 0;
    while (current_index < all_length) {
      i++;
      DataBuffer packet_msg;
      packet_msg.emplace_back(tcp_packet_msg_header.header);
      packet_msg.insert(packet_msg.end(), msg.begin() + current_index,
                        msg.begin() + current_index +
                            buffer_size); // [first,last)
      if (tcpIpSendPacketMsg(packet_msg)) {
        current_index = current_index + buffer_size;
      } else {
        TRACKING_LOG_WARN("{} tcpIpSendMsg fail return false to retry.",
                          log_prefix);
        return false; // return false to resend all packet.
      }
    }

    // send last packet_msg
    DataBuffer packet_msg;
    packet_msg.emplace_back(tcp_packet_msg_header.header);
    packet_msg.insert(packet_msg.end(), msg.begin() + current_index,
                      msg.end()); // [first,last)
    if (tcpIpSendPacketMsg(packet_msg)) {
      // last send
    } else {
      TRACKING_LOG_WARN(
          "{} tcpIpSendMsg fail with last packet_msg. return false to retry.",
          log_prefix);
      return false; // return false to resend all packet.
    }

    // tcpIpSendMsg last packet_msg success
    // (twice, once is header, once is body)
    return true;
  } else {
    TRACKING_LOG_WARN("{} tcpIpSendMsg fail with connect server failed. "
                      "return false to retry.",
                      log_prefix);
    return false;
  }
}

static XrealLinkTcp *getInstance(const char *addr = LOCAL_ADDRESS,
                                 const bool tcp_log = false) {
  static XrealLinkTcp instance(addr, tcp_log);
  return &instance;
}

static int setServerAddr(const char *addr) {
  auto instance = getInstance(addr);
  if (addr != LOCAL_ADDRESS) {
    if (instance->server_addr.sin_addr.s_addr != inet_addr(addr)) {
      sockaddr_in new_addr;
      memset(&new_addr, 0, sizeof(new_addr));
      new_addr.sin_family = AF_INET;
      new_addr.sin_addr.s_addr = inet_addr(addr);
      new_addr.sin_port = htons(SERVER_PORT_TCP);
      instance->server_addr = new_addr;
    }
  }
  return 0;
}

void setLogFilename(const std::string log_filename) {
  if (tcp_log_) {
    ofs_ptr_.reset(new std::ofstream(log_filename,
                                     std::ofstream::out | std::ofstream::app));
    TRACKING_LOG_WARN("{} set log_filename:{}", log_prefix, log_filename);
  }
}

static bool &use_xreal_link() { return getInstance()->use_xreal_link_; }
}; // namespace toolkits
} // namespace xreal
} // namespace toolkits
} // namespace xreal

#endif
