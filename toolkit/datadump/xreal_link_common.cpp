#include "xreal_link_common.h"
#include <chrono>
#include <errno.h>
#include <iostream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#include "../util/logging.h"

using xreal::toolkits::utils::Logger;

namespace xreal {
namespace toolkits {
namespace datadump {

// 静态成员定义
DataStructureManager XrealLinkCommon::structure_manager_;

// 时间戳获取函数实现
uint64_t get_current_timestamp_us() {
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC, &cur_time);
  return (uint64_t)cur_time.tv_sec * 1000000 +
         (uint64_t)cur_time.tv_nsec / 1000;
}

uint64_t get_current_timestamp_ns() {
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC, &cur_time);
  return (uint64_t)cur_time.tv_sec * 1000000000 + (uint64_t)cur_time.tv_nsec;
}

// XrealLinkCommon 实现
bool XrealLinkCommon::loadDataStructuresFromJson(
    const std::string &json_config) {
  return structure_manager_.loadFromJsonString(json_config);
}

bool XrealLinkCommon::loadDataStructuresFromFile(const std::string &filename) {
  return structure_manager_.loadFromJsonFile(filename);
}

DataStructureManager &XrealLinkCommon::getStructureManager() {
  return structure_manager_;
}

uint64_t XrealLinkCommon::getKey(const uint64_t group_id,
                                 const uint64_t msg_id) {
  union grouped_msg_id GroupedMsgId;
  GroupedMsgId.data.group_id = group_id;
  GroupedMsgId.data.msg_id = msg_id;
  return GroupedMsgId.id;
}

void XrealLinkCommon::msgInit(SimpleMessageHeader &_msg, int _group_id,
                              int _msg_id, int32_t _len, uint64_t _ts) {
  _msg.magic = _group_id;
  _msg.msg_id = _msg_id;
  _msg.payload_length = _len;
  if (!_ts) {
    _msg.time_stamp = get_current_timestamp_us();
  } else {
    _msg.time_stamp = _ts;
  }
}

void XrealLinkCommon::msgSetBuf(const SimpleMessageHeader &_msg,
                                DataBuffer &_buffer, const char *_payload,
                                size_t payload_size) {
  _buffer.resize(sizeof(SimpleMessageHeader) + payload_size);
  memcpy(_buffer.data(), &_msg, sizeof(SimpleMessageHeader));
  if (_payload && payload_size > 0) {
    memcpy(_buffer.data() + sizeof(SimpleMessageHeader), _payload,
           payload_size);
  }
}

std::string XrealLinkCommon::file_date_time(std::time_t posix) {
  char buf[20];
  std::tm tp = *std::localtime(&posix);
  return {buf, std::strftime(buf, sizeof(buf), "%m_%d_%H_%M_%S", &tp)};
}

std::string XrealLinkCommon::file_date(std::time_t posix) {
  char buf[20];
  std::tm tp = *std::localtime(&posix);
  return {buf, std::strftime(buf, sizeof(buf), "%Y%m%d", &tp)};
}

void XrealLinkCommon::getNowTimeFormat(char *time_format) {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(time_format, 80, "%F %T", timeinfo);
}

uint8_t XrealLinkCommon::crc8(uint8_t *data, int size) {
  uint8_t crc = 0x00;
  uint8_t poly = 0x07;
  int bit;

  while (size--) {
    crc ^= *data++;
    for (bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void XrealLinkCommon::messageEnQueue(DataBuffer &msg) {
  concurrent_lock_free_queue_collect_.enqueue(msg);

  if (false == collect_thread_running) {
    collect_thread_running = true;
    collect_thread_ = std::thread(&XrealLinkCommon::CollectThread, this);
  }

  if (false == send_thread_running) {
    send_thread_running = true;
    send_thread_ = std::thread(&XrealLinkCommon::sendThreadWrapper, this);
  }
}

void XrealLinkCommon::sendThreadWrapper() { sendThread(); }

void XrealLinkCommon::CollectThread() {
  uint64_t last_timestamp_s = 0;
  uint32_t freq_count = 0;
  DataBuffer group_msg;
  static uint64_t packet_id = 0;

  while (collect_thread_running) {
    DataBuffer raw_msg;
    bool processed_data = false;

    while (concurrent_lock_free_queue_collect_.try_dequeue(raw_msg)) {
      // 解析原始消息：包含group_id, msg_id, payload_data
      if (raw_msg.size() < sizeof(RawMessageHeader))
        continue;

      RawMessageHeader *raw_header = (RawMessageHeader *)raw_msg.data();
      int group_id = raw_header->group_id;
      int msg_id = raw_header->msg_id;
      size_t payload_size = raw_msg.size() - sizeof(RawMessageHeader);
      uint8_t *payload_data = raw_msg.data() + sizeof(RawMessageHeader);

      // 在CollectThread中创建SimpleMessageHeader
      SimpleMessageHeader msg_header;
      msgInit(msg_header, group_id, msg_id, payload_size, 0);

      // 在CollectThread中创建完整的消息缓冲区
      DataBuffer complete_msg;
      msgSetBuf(msg_header, complete_msg, (const char *)payload_data,
                payload_size);

      uint64_t current_timestamp_ns = get_current_timestamp_ns();
      uint64_t current_timestamp_s = current_timestamp_ns / 1e9;

      if (0 == last_timestamp_s) {
        last_timestamp_s = current_timestamp_s;
      }

      if (current_timestamp_s == last_timestamp_s) { // 1s内所有的都收集起来
        freq_count++;
        group_msg.insert(group_msg.end(), complete_msg.begin(),
                         complete_msg.end());
      } else {

        // // 在发送数据前，添加队列大小信息
        // publishQueueSizeInfo(getCollectQueueSize(), getSendQueueSize(),
        //                      group_id, get_current_timestamp_us(),
        //                      current_timestamp_ns, freq_count, group_msg);

        // 发送累积的数据
        processGroupMsg(group_msg, freq_count, current_timestamp_ns, packet_id);

        packet_id++;

        DLOG_INFO(
            "{} packet_msg packet_id:{} freq_count:{} "
            "header.timestamp_ns:{}, collect_queue_size:{}, send_queue_size:{}",
            log_prefix, packet_id, freq_count, current_timestamp_ns,
            getCollectQueueSize(), getSendQueueSize());

        freq_count = 1;
        group_msg.clear();
        group_msg.insert(group_msg.end(), complete_msg.begin(),
                         complete_msg.end());
        last_timestamp_s = current_timestamp_s;
      }
      processed_data = true;
    }

        // 处理剩余的消息
    if (!group_msg.empty()) {
      uint64_t current_timestamp_ns = get_current_timestamp_ns();
      
      // 发送剩余的消息
      processGroupMsg(group_msg, freq_count, current_timestamp_ns, packet_id);
      
      packet_id++;
      
      DLOG_INFO(
          "{} packet_msg packet_id:{} freq_count:{} "
          "header.timestamp_ns:{}, collect_queue_size:{}, send_queue_size:{}",
          log_prefix, packet_id, freq_count, current_timestamp_ns,
          getCollectQueueSize(), getSendQueueSize());
      
      group_msg.clear();
    }
    // 如果队列为空，短暂休眠避免忙等待
    if (!processed_data) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

}

size_t XrealLinkCommon::getSendQueueSize() const {
  return concurrent_lock_free_queue_send_.size_approx();
}

size_t XrealLinkCommon::getCollectQueueSize() const {
  return concurrent_lock_free_queue_collect_.size_approx();
}

void XrealLinkCommon::publishQueueSizeInfo(
    size_t collect_queue_size, size_t send_queue_size, uint32_t group_id,
    uint64_t onsensor_timestamp_us, uint64_t timestamp_ns, uint32_t &freq_count,
    DataBuffer &group_msg) {
  // 创建队列大小信息数据结构
  struct QueueSizeData {
    uint64_t onsensor_timestamp_us;
    uint64_t timestamp_ns;
    uint32_t type;
    float data[6];
  };

  QueueSizeData queue_count_data_struct;
  queue_count_data_struct.onsensor_timestamp_us = onsensor_timestamp_us;
  queue_count_data_struct.timestamp_ns = timestamp_ns;
  queue_count_data_struct.type = 24; // 队列大小信息类型
  queue_count_data_struct.data[0] = static_cast<float>(collect_queue_size);
  queue_count_data_struct.data[1] = static_cast<float>(send_queue_size);
  queue_count_data_struct.data[2] = -1;
  queue_count_data_struct.data[3] = -1;
  queue_count_data_struct.data[4] = -1;
  queue_count_data_struct.data[5] = -1;

  // 直接创建SimpleMessageHeader并调用processGroupMsg
  SimpleMessageHeader msg_header_count;
  msgInit(msg_header_count, XREAL_LINK_COUNT_GROUP_ID, 11, sizeof(QueueSizeData),
          0); // msg_id=11 for queue count info
  // 创建完整的消息缓冲区
  DataBuffer queue_count_data;
  msgSetBuf(msg_header_count, queue_count_data,
            (const char *)&queue_count_data_struct, sizeof(QueueSizeData));
  // 直接调用processGroupMsg发送
  group_msg.insert(group_msg.end(), queue_count_data.begin(),
                   queue_count_data.end());
  freq_count++;
}

void XrealLinkCommon::cleanupThreads() {
  if (send_thread_running && send_thread_.joinable()) {
    send_thread_running = false;
    send_thread_.join();
  }

  if (collect_thread_running && collect_thread_.joinable()) {
    collect_thread_running = false;
    collect_thread_.join();
  }
}

} // namespace datadump
} // namespace toolkits
} // namespace xreal