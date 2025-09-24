#ifndef XREAL_LINK_COMMON_H
#define XREAL_LINK_COMMON_H

#include "utils/blockingconcurrentqueue.h"
#include "utils/datadump/data_structure_manager.h"
#include "utils/xreal_link/xreal_link.h"
#include <chrono>
#include <errno.h>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

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

#include "utils/logging.h"

namespace xreal {
namespace toolkits {
namespace datadump {

// 公共常量定义
#define SERVER_PORT_TCP 8099
#define TCP_MAGIC_NUM 239
#define TCP_PACKET_MAX_SIZE 1448
#define XREAL_LINK_COUNT_GROUP_ID 1
#define XREAL_LINK_TCP_FILE_LOG
#define LOCAL_ADDRESS "127.0.0.1"

// 时间戳获取函数
static uint64_t get_current_timestamp_us() {
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC, &cur_time);
  return (uint64_t)cur_time.tv_sec * 1000000 +
         (uint64_t)cur_time.tv_nsec / 1000;
}

static uint64_t get_current_timestamp_ns() {
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC, &cur_time);
  return (uint64_t)cur_time.tv_sec * 1000000000 + (uint64_t)cur_time.tv_nsec;
}

// 公共数据结构 - 简化版本，只保留必要的结构
#pragma pack(1)
// TCP消息头 - 保留用于网络传输
typedef struct TcpMsgHeader {
  uint8_t version;          // version=1
  uint8_t magic_num;        // magic_num for check
  uint8_t serialize_method; // serialize_method, 0 is unpack
  uint8_t service_num;      // like SEND, GET, ADD something
  uint8_t msg_type;         // 1 sensor_data
  uint32_t msg_count;       // freq_count
  uint32_t length;          // length of (header+crc+msg)
  uint64_t timestamp_ns;    // header's timestamp_ns
  uint64_t packet_id;       // header's packet_id (可选)
  uint8_t crc;              // crc check
} TcpMsgHeader;

union TcpPacketMsgHeader {
  uint8_t header;
  struct data {
    uint8_t submsg_type : 1; // 0:header, 1:data
    uint8_t reserved : 7;
  } data;
};

union grouped_msg_id {
  int64_t id;
  struct data {
    int64_t group_id : 16; // low 16 bit
    int64_t msg_id : 16;   // low 16 bit
    int64_t src_id : 16;   // low 16 bit
    int64_t reserved : 16;
  } data;
};

// 简化的消息头结构
struct SimpleMessageHeader {
  uint8_t magic;
  int32_t msg_id;
  int32_t payload_length;
  uint64_t time_stamp;
};
#pragma pack()

// 公共枚举
enum MsgContentType : uint8_t { SENSOR_DATA = 1 };
enum PacketMsgType : uint8_t { SUBMSG_TYPE_HEADER = 0, SUBMSG_TYPE_DATA = 1 };

// 基本数据类型定义
using DataBuffer = std::vector<uint8_t>;
using FloatVector = std::vector<float>;
using IntVector = std::vector<int>;
using StringVector = std::vector<std::string>;

// 公共基类
class XrealLinkCommon {
protected:
  // 公共成员变量
  std::string log_prefix = "[xreal_link_common]";
  bool use_xreal_link_{0};

  // 数据结构管理器
  static DataStructureManager structure_manager_;

  // 线程管理
  moodycamel::ConcurrentQueue<std::vector<uint8_t>>
      concurrent_lock_free_queue_collect_;
  bool collect_thread_running = false;
  std::thread collect_thread_;

  moodycamel::ConcurrentQueue<std::vector<uint8_t>>
      concurrent_lock_free_queue_send_;
  bool send_thread_running = false;
  std::thread send_thread_;

public:
  virtual ~XrealLinkCommon() = default;

  // 静态方法：加载JSON配置
  static bool loadDataStructuresFromJson(const std::string &json_config) {
    return structure_manager_.loadFromJsonString(json_config);
  }

  static bool loadDataStructuresFromFile(const std::string &filename) {
    return structure_manager_.loadFromJsonFile(filename);
  }

  // 获取数据结构管理器
  static DataStructureManager &getStructureManager() {
    return structure_manager_;
  }

  // 公共静态工具函数
  static int64_t getKey(const int64_t group_id, const int64_t msg_id,
                        const int64_t src_id) {
    union grouped_msg_id GroupedMsgId;
    GroupedMsgId.data.group_id = group_id;
    GroupedMsgId.data.msg_id = msg_id;
    return GroupedMsgId.id;
  }

  // 简化的消息初始化 - 使用SimpleMessageHeader
  static void msgInit(SimpleMessageHeader &_msg, int _group_id, int _msg_id,
                      int32_t _len, uint64_t _ts) {
    _msg.magic = _group_id;
    _msg.msg_id = _msg_id;
    _msg.payload_length = _len;
    if (!_ts) {
      _msg.time_stamp = get_current_timestamp_us();
    } else {
      _msg.time_stamp = _ts;
    }
  }

  // 简化的缓冲区设置 - 直接使用char*和size
  static void msgSetBuf(const SimpleMessageHeader &_msg, DataBuffer &_buffer,
                        const char *_payload, size_t payload_size) {
    _buffer.resize(sizeof(SimpleMessageHeader) + payload_size);
    memcpy(_buffer.data(), &_msg, sizeof(SimpleMessageHeader));
    if (_payload && payload_size > 0) {
      memcpy(_buffer.data() + sizeof(SimpleMessageHeader), _payload,
             payload_size);
    }
  }

  // 模板版本 - 支持vector<T>
  template <typename T>
  static void msgSetBuf(const SimpleMessageHeader &_msg, DataBuffer &_buffer,
                        const std::vector<T> &_payload) {
    size_t payload_size = _payload.size() * sizeof(T);
    _buffer.resize(sizeof(SimpleMessageHeader) + payload_size);
    memcpy(_buffer.data(), &_msg, sizeof(SimpleMessageHeader));
    if (!_payload.empty()) {
      memcpy(_buffer.data() + sizeof(SimpleMessageHeader), _payload.data(),
             payload_size);
    }
  }

  // 时间格式化工具函数
  static std::string file_date_time(std::time_t posix) {
    char buf[20];
    std::tm tp = *std::localtime(&posix);
    return {buf, std::strftime(buf, sizeof(buf), "%m_%d_%H_%M_%S", &tp)};
  }

  static std::string file_date(std::time_t posix) {
    char buf[20];
    std::tm tp = *std::localtime(&posix);
    return {buf, std::strftime(buf, sizeof(buf), "%Y%m%d", &tp)};
  }

  static void getNowTimeFormat(char *time_format) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_format, 80, "%F %T", timeinfo);
  }

  // CRC8校验算法
  static uint8_t crc8(uint8_t *data, int size) {
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

  // 虚函数，子类需要实现
  virtual void _Close() = 0;

  // 通用的messageEnQueue实现
  virtual void messageEnQueue(DataBuffer &msg) {
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

private:
  // 线程包装器，调用子类的sendThread
  void sendThreadWrapper() { sendThread(); }

protected:
  // 纯虚函数：子类必须实现具体的发送逻辑
  virtual void sendThread() = 0;

  // 通用的CollectThread - 在这个线程中创建消息头和缓冲区
  virtual void CollectThread() {
    uint64_t last_timestamp_s = 0;
    uint32_t freq_count = 0;
    DataBuffer group_msg;
    static uint64_t packet_id = 0;

    while (collect_thread_running) {
      DataBuffer raw_msg;
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

        if (current_timestamp_s == last_timestamp_s) {
          freq_count++;
          group_msg.insert(group_msg.end(), complete_msg.begin(),
                           complete_msg.end());
        } else {
          // 发送累积的数据
          processGroupMsg(group_msg, freq_count, current_timestamp_ns,
                          packet_id);
          packet_id++;

          TRACKING_LOG_WARN(
              "{} packet_msg packet_id:{} freq_count:{} header.timestamp_ns:{}",
              log_prefix, packet_id, freq_count, current_timestamp_ns);

          freq_count = 1;
          group_msg.clear();
          group_msg.insert(group_msg.end(), complete_msg.begin(),
                           complete_msg.end());
          last_timestamp_s = current_timestamp_s;
        }
      }
    }
  }

  // 虚函数：处理分组后的消息，子类重载实现不同的处理逻辑
  virtual void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                               uint64_t timestamp_ns, uint64_t packet_id) = 0;

// 原始消息头结构 - 用于在队列中传输
#pragma pack(1)
struct RawMessageHeader {
  int32_t group_id;
  int32_t msg_id;
};
#pragma pack()

// 宏定义：在linkSendStatus中只发送原始数据，让CollectThread创建消息头
#define IMPLEMENT_LINK_SEND_STATUS(ClassName)                                  \
  static void linkSendStatus(int group_id, int msg_id, const uint8_t *data,    \
                             uint64_t len, uint64_t timestamp_us = 0) {        \
    DataBuffer raw_msg;                                                        \
    raw_msg.resize(sizeof(RawMessageHeader) + len);                            \
    RawMessageHeader *raw_header = (RawMessageHeader *)raw_msg.data();         \
    raw_header->group_id = group_id;                                           \
    raw_header->msg_id = msg_id;                                               \
    if (data && len > 0) {                                                     \
      memcpy(raw_msg.data() + sizeof(RawMessageHeader), data, len);            \
    }                                                                          \
    ClassName::getInstance()->messageEnQueue(raw_msg);                         \
  }                                                                            \
  template <typename T>                                                        \
  static void linkSendStatus(int group_id, int msg_id,                         \
                             const std::vector<T> &vec,                        \
                             uint64_t timestamp_us = 0) {                      \
    size_t data_size = vec.size() * sizeof(T);                                 \
    linkSendStatus(group_id, msg_id, (const uint8_t *)vec.data(), data_size,   \
                   timestamp_us);                                              \
  }

  // 公共的队列大小获取接口
  virtual size_t getSendQueueSize() const {
    return concurrent_lock_free_queue_send_.size_approx();
  }

  virtual size_t getCollectQueueSize() const {
    return concurrent_lock_free_queue_collect_.size_approx();
  }

protected:
  // 公共的线程清理函数
  void cleanupThreads() {
    if (send_thread_running && send_thread_.joinable()) {
      send_thread_running = false;
      send_thread_.join();
    }

    if (collect_thread_running && collect_thread_.joinable()) {
      collect_thread_running = false;
      collect_thread_.join();
    }
  }
};

// 静态成员定义
DataStructureManager XrealLinkCommon::structure_manager_;

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif // XREAL_LINK_COMMON_H