#ifndef XREAL_LINK_COMMON_H
#define XREAL_LINK_COMMON_H

#include "data_structure_manager.h"
#include "threadsafe_queue/blockingconcurrentqueue.h"
#include <cstring> // for memcpy in template function
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

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

// 时间戳获取函数声明
uint64_t get_current_timestamp_us();
uint64_t get_current_timestamp_ns();

// 公共数据结构 - 简化版本，只保留必要的结构
#pragma pack(push, 1)
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
  uint64_t id;
  struct data {
    uint64_t group_id : 16; // low 16 bit
    uint64_t msg_id : 16;   // low 16 bit
  } data;
};

// 简化的消息头结构
struct SimpleMessageHeader {
  uint8_t magic;
  uint32_t msg_id;
  uint32_t payload_length;
  uint64_t time_stamp;
};
#pragma pack(pop)

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
  static bool loadDataStructuresFromJson(const std::string &json_config);
  static bool loadDataStructuresFromFile(const std::string &filename);

  // 获取数据结构管理器
  static DataStructureManager &getStructureManager();

  // 公共静态工具函数
  static uint64_t getKey(const uint64_t group_id, const uint64_t msg_id);

  // 简化的消息初始化 - 使用SimpleMessageHeader
  static void msgInit(SimpleMessageHeader &_msg, int _group_id, int _msg_id,
                      int32_t _len, uint64_t _ts);

  // 简化的缓冲区设置 - 直接使用char*和size
  static void msgSetBuf(const SimpleMessageHeader &_msg, DataBuffer &_buffer,
                        const char *_payload, size_t payload_size);

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

  static void GetMsgPayload(uint8_t *buffer, size_t buffer_size,
                            SimpleMessageHeader *out_msg,
                            uint8_t *&out_payload, size_t &out_payload_size) {
    // Parse header from the buffer and return pointer/size for payload
    *out_msg = *(SimpleMessageHeader *)buffer;
    out_payload = buffer + sizeof(SimpleMessageHeader);
    out_payload_size = buffer_size - sizeof(SimpleMessageHeader);
  }

  // 时间格式化工具函数
  static std::string file_date_time(std::time_t posix);
  static std::string file_date(std::time_t posix);
  static void getNowTimeFormat(char *time_format);

  // CRC8校验算法
  static uint8_t crc8(uint8_t *data, int size);

  // 虚函数，子类需要实现
  virtual void _Close() = 0;

  // 通用的messageEnQueue实现
  virtual void messageEnQueue(DataBuffer &msg);

private:
  // 线程包装器，调用子类的sendThread
  void sendThreadWrapper();

  void publishQueueSizeInfo(size_t collect_queue_size, size_t send_queue_size,
                            uint32_t group_id, uint64_t onsensor_timestamp_us,
                            uint64_t timestamp_ns, uint32_t &freq_count,
                            DataBuffer &group_msg);

protected:
  // 纯虚函数：子类必须实现具体的发送逻辑
  virtual void sendThread() = 0;

  // 通用的CollectThread - 在这个线程中创建消息头和缓冲区
  virtual void CollectThread();

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
                             uint64_t len) {                                   \
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
                             const std::vector<T> &vec) {                      \
    size_t data_size = vec.size() * sizeof(T);                                 \
    linkSendStatus(group_id, msg_id, (const uint8_t *)vec.data(), data_size);  \
  }

  // 公共的队列大小获取接口
  virtual size_t getSendQueueSize() const;
  virtual size_t getCollectQueueSize() const;

protected:
  // 公共的线程清理函数
  void cleanupThreads();
};

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif // XREAL_LINK_COMMON_H