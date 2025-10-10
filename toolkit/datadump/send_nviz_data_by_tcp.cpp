#include "send_nviz_data_by_tcp.h"
#include <errno.h> /* EINPROGRESS, errno */
#include <iostream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include "../util/logging.h"

using xreal::toolkits::utils::Logger;

namespace xreal {
namespace toolkits {
namespace datadump {

XrealLinkTcp::XrealLinkTcp(const char *server_address) {
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server_address);
  server_addr.sin_port = htons(SERVER_PORT_TCP);
  log_prefix = "[xreal_link_tcp]";
}

XrealLinkTcp::~XrealLinkTcp() { _Close(); }

void XrealLinkTcp::_Close() {
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

bool XrealLinkTcp::connectServer() {
  DLOG_TRACE("{} connected:{} in connectServer. {} {}", log_prefix, connected,
             __FILE__, __LINE__);

  if (connected) {
    return true;
  }

  char time_format[80];
  getNowTimeFormat(time_format);

  DLOG_TRACE("{} sockfd:{} addr:{} port:{} in connectServer. {} {}", log_prefix,
             sockfd, inet_ntoa(server_addr.sin_addr),
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

    DLOG_WARN("{} new sockfd:{} addr:{}  port:{} in connectServer.", log_prefix,
              sockfd, inet_ntoa(server_addr.sin_addr),
              ntohs(server_addr.sin_port));
  }

  DLOG_TRACE("{} sockfd:{} in connectServer. {} {}", log_prefix, sockfd,
             __FILE__, __LINE__);
  int err =
      connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  DLOG_TRACE("{} err:{} in connectServer. {} {}", log_prefix, err, __FILE__,
             __LINE__);
  if (-1 == err) {
    DLOG_WARN("{} new sockfd connect failed. sockfd:{} addr:{}  port:{} in "
              "connectServer. errno:{} strerror:{}",
              log_prefix, sockfd, inet_ntoa(server_addr.sin_addr),
              ntohs(server_addr.sin_port), errno, strerror(errno));
    return false;
  }

  connected = true;
  connected_once = true;
  DLOG_TRACE("{} connected:{} in connectServer. {} {}", log_prefix, connected,
             __FILE__, __LINE__);
  return true;
}

void XrealLinkTcp::closeSockfd() {
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

void XrealLinkTcp::Close() { XrealLinkTcp::getInstance()->_Close(); }

void XrealLinkTcp::messageEnQueue(DataBuffer &msg) {
  if (true == flag_start_collect) {
    XrealLinkCommon::messageEnQueue(msg); // 调用基类实现
  }
}

void XrealLinkTcp::processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                                   uint64_t timestamp_ns, uint64_t packet_id) {
  // 更新组消息中每个SimpleMessageHeader的时间戳为入send队列时间
  updateGroupMsgTimestamps(group_msg);

  // 使用入send队列的时间作为TCP包头的时间戳
  uint64_t send_queue_timestamp_ns = get_current_timestamp_ns(); // 纳秒级时间戳

  DataBuffer msg_header;
  setPacketHeader(msg_header, freq_count, group_msg.size(),
                  send_queue_timestamp_ns, packet_id);
  concurrent_lock_free_queue_send_.enqueue(msg_header);

  DataBuffer msg_data;
  setPacketData(group_msg, msg_data);
  concurrent_lock_free_queue_send_.enqueue(msg_data);
}

void XrealLinkTcp::updateGroupMsgTimestamps(DataBuffer &group_msg) {
  uint64_t send_queue_timestamp =
      get_current_timestamp_us(); // 入send队列的时间戳

  size_t curLen = group_msg.size();
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
    if (curLen < (size_t)payload_len) {
      break;
    }

    curDataPtr += payload_len;
    curLen -= payload_len;
  }
}

void XrealLinkTcp::sendThread() {
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
      //  sleep 0.5s , because of the data that accumulates for 1 second.
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}

void XrealLinkTcp::setPacketHeader(DataBuffer &msg_header, uint32_t freq_count,
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

void XrealLinkTcp::setPacketData(DataBuffer &group_msg, DataBuffer &msg) {
  TcpPacketMsgHeader tcp_packet_msg_header = {.header = 0};
  tcp_packet_msg_header.data.submsg_type = PacketMsgType::SUBMSG_TYPE_DATA;

  msg.emplace_back(tcp_packet_msg_header.header);
  msg.insert(msg.end(), group_msg.begin(), group_msg.end());
}

bool XrealLinkTcp::tcpIpSendPacketMsg(DataBuffer msg) {
  if (msg.size() <= 0) {
    DLOG_WARN("{} msg.size() <= 1 in tcpIpSendPacketMsg.", log_prefix);
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
      } else if (strlen(receiveMessage) ==
                 0) { // There may be a problem with Nviz
        DLOG_WARN("{} bad receiveMessage content 1. content[{}] "
                  "errno:{} strerror:{}",
                  log_prefix, receiveMessage, errno, strerror(errno));

        return false;
      } else {
        DLOG_WARN("{} bad receiveMessage content 2. content[{}] "
                  "errno:{} strerror:{}",
                  log_prefix, receiveMessage, errno, strerror(errno));

        return false;
      }
    } else {
      DLOG_WARN("{} tcpip recv fail. errno:{} strerror:{}", log_prefix, errno,
                strerror(errno));
      return false;
    }

  } else {
    DLOG_WARN("{} tcpip send fail. msg_len:{}  errno:{} strerror:{}",
              log_prefix, msg.size(), errno, strerror(errno));
    if (EPIPE == errno) { // EPIPE = 32, strerror is "Broken Pipe"
      closeSockfd();
      if (connectServer()) {
        DLOG_WARN("{} reconnect server by errno:{} strerror:{}", log_prefix,
                  errno, strerror(errno));
      }
    }

    DLOG_WARN("{} return false in tcpIpSendPacketMsg.", log_prefix);
    return false;
  }
}

bool XrealLinkTcp::tcpIpSendMsg(DataBuffer msg) {
  if (connectServer()) {
    size_t packet_msg_header_length = sizeof(TcpPacketMsgHeader);
    if (msg.size() <= packet_msg_header_length) {
      DLOG_WARN("{} tcpIpSendMsg fail return true. reason: msg.size() <= "
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
        DLOG_WARN("{} tcpIpSendMsg fail return false to retry.", log_prefix);
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
      DLOG_WARN(
          "{} tcpIpSendMsg fail with last packet_msg. return false to retry.",
          log_prefix);
      return false; // return false to resend all packet.
    }

    // tcpIpSendMsg last packet_msg success
    // (twice, once is header, once is body)
    return true;
  } else {
    DLOG_WARN("{} tcpIpSendMsg fail with connect server failed. "
              "return false to retry.",
              log_prefix);
    return false;
  }
}

XrealLinkTcp *XrealLinkTcp::getInstance(const char *addr) {
  static XrealLinkTcp instance(addr);
  return &instance;
}

XrealLinkTcp *XrealLinkTcp::getInstance() { return getInstance(LOCAL_ADDRESS); }

int XrealLinkTcp::setServerAddr(const char *addr) {
  auto instance = getInstance(addr);
  if (strcmp(addr, LOCAL_ADDRESS) != 0) {
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

void XrealLinkTcp::linkSendStatus(int group_id, int msg_id, const uint8_t *data,
                                  uint64_t len) {
  DataBuffer raw_msg;
  raw_msg.resize(sizeof(XrealLinkCommon::RawMessageHeader) + len);
  XrealLinkCommon::RawMessageHeader *raw_header = 
      (XrealLinkCommon::RawMessageHeader *)raw_msg.data();
  raw_header->group_id = group_id;
  raw_header->msg_id = msg_id;
  if (data && len > 0) {
    memcpy(raw_msg.data() + sizeof(XrealLinkCommon::RawMessageHeader), data, len);
  }
  XrealLinkTcp::getInstance()->messageEnQueue(raw_msg);
}

} // namespace datadump
} // namespace toolkits
} // namespace xreal
