#ifndef PROJECT_XREAL_LINK_ONLY_SAVE_FILE_H
#define PROJECT_XREAL_LINK_ONLY_SAVE_FILE_H

#include <errno.h> /* EINPROGRESS, errno */

#include <arpa/inet.h>
#include <chrono>  // 添加时间处理
#include <fstream> // std::ofstream
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sys/stat.h>  // mkdir   rmdir
#include <sys/types.h> // mkdir   rmdir
#include <unistd.h>    // access

#include "utils/logging.h"
#include "xreal_link_common.h"

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

class XrealLinkOnlySaveFile : public XrealLinkCommon {
protected:
  // 文件保存特有的成员变量
  std::map<int64_t, std::string> msg_id_to_msg_name_;
  std::map<int64_t, std::shared_ptr<std::ofstream>> msg_id_to_ofs_;
  std::map<int64_t, std::string> msg_id_to_buffer_; // 添加缓冲区
  std::string filename_prefix_ = "";
  std::string save_dir_ =
      "sdcard/Android/data/ai.xreal.nebula.universal/files/";

  std::string save_dir_with_date_ = "";

  // 批量写入配置
  static const size_t BUFFER_SIZE_LIMIT = 8192; // 8KB缓冲区
  static const size_t BUFFER_COUNT_LIMIT = 100; // 100条记录
  std::map<int64_t, size_t> msg_id_to_count_;   // 记录计数
  std::map<int64_t, std::chrono::steady_clock::time_point>
      msg_id_to_last_flush_; // 上次刷新时间
  static const std::chrono::milliseconds FLUSH_INTERVAL{50}; // 50ms强制刷新

  XrealLinkOnlySaveFile() { log_prefix = "[xreal_link_file]"; }

public:
  ~XrealLinkOnlySaveFile() { _Close(); }

  void _Close() override {
    // 先刷新所有缓冲区
    for (auto &pair : msg_id_to_buffer_) {
      if (!pair.second.empty()) {
        flushBuffer(pair.first);
      }
    }

    filename_prefix_ = "";
    save_dir_with_date_ = "";

    auto it_erase = msg_id_to_ofs_.begin();
    while (it_erase != msg_id_to_ofs_.end()) {
      it_erase = msg_id_to_ofs_.erase(it_erase);
    }

    msg_id_to_buffer_.clear();
    msg_id_to_count_.clear();
    msg_id_to_last_flush_.clear();

    // 清理线程
    cleanupThreads();
  }

  static void Close() { XrealLinkOnlySaveFile::getInstance()->_Close(); }

  // 使用宏实现linkSendStatus，避免重复代码
  IMPLEMENT_LINK_SEND_STATUS(XrealLinkOnlySaveFile)

  // 重载基类的processGroupMsg - 文件保存版本
  void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                       uint64_t timestamp_ns, uint64_t packet_id) override {
    // 文件保存类直接将分组后的消息发送到send队列
    concurrent_lock_free_queue_send_.enqueue(group_msg);
  }

protected:
  // 实现基类的纯虚函数 - 文件保存专用的发送线程
  void sendThread() override {
    DataBuffer msg;

    while (send_thread_running) {
      while (concurrent_lock_free_queue_send_.try_dequeue(msg)) {
        saveMsg(msg); // 文件保存不会失败，直接调用即可
      }

      // 如果队列为空，适当休眠避免忙等待
      if (concurrent_lock_free_queue_send_.size_approx() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1s休眠
      }
    }
  }

public:
  // 这些时间格式化函数已在基类中定义，可以删除
  // std::string file_date_time(std::time_t posix)
  // std::string file_date(std::time_t posix)

  bool saveMsg(DataBuffer msg) {
    int curLen = msg.size();
    uint8_t *curDataPtr = msg.data();

    while (curLen > 0) {
      if (curLen < sizeof(SimpleMessageHeader)) {
        break; // 不够一个消息头的大小
      }

      SimpleMessageHeader *msg_header = (SimpleMessageHeader *)curDataPtr;
      curDataPtr += sizeof(SimpleMessageHeader);
      curLen -= sizeof(SimpleMessageHeader);

      int64_t group_id = msg_header->magic;
      int64_t msg_id = msg_header->msg_id;
      int32_t len = msg_header->payload_length;
      uint64_t timestamp = msg_header->time_stamp; // 入队列时间戳(微秒)

      if (curLen < len) {
        break; // 数据不完整
      }

      // 使用数据结构管理器解析数据
      saveStructuredData(group_id, msg_id, curDataPtr, len, timestamp);

      curDataPtr += len;
      curLen -= len;
    }

    return true;
  }

private:
  // 使用数据结构管理器保存结构化数据
  void saveStructuredData(int64_t group_id, int64_t msg_id, const void *data,
                          size_t data_size, uint64_t timestamp) {
    int64_t key = XrealLinkCommon::getKey(group_id, msg_id, 0);

    // 确保文件已创建
    getOfsByXreallinkIds(group_id, msg_id, 0);

    // 使用数据结构管理器解析数据
    std::string csv_line =
        structure_manager_.parseDataToCsv(group_id, msg_id, data, data_size);

    if (csv_line.empty()) {
      // 如果没有找到对应的数据结构，使用默认解析方式
      csv_line = parseAsDefault(data, data_size, timestamp);
    } else {
      // 只在CSV行前添加timestamp
      csv_line = std::to_string(timestamp) + "," + csv_line;
    }

    csv_line += "\n";

    // 添加到缓冲区
    msg_id_to_buffer_[key] += csv_line;
    msg_id_to_count_[key]++;

    // 检查是否需要刷新缓冲区
    auto now = std::chrono::steady_clock::now();
    bool should_flush = false;

    if (msg_id_to_buffer_[key].size() >= BUFFER_SIZE_LIMIT) {
      should_flush = true;
    } else if (msg_id_to_count_[key] >= BUFFER_COUNT_LIMIT) {
      should_flush = true;
    } else if (msg_id_to_last_flush_.find(key) != msg_id_to_last_flush_.end() &&
               now - msg_id_to_last_flush_[key] >= FLUSH_INTERVAL) {
      should_flush = true;
    } else if (msg_id_to_last_flush_.find(key) == msg_id_to_last_flush_.end()) {
      msg_id_to_last_flush_[key] = now;
    }

    if (should_flush) {
      flushBuffer(key);
    }
  }

  // 默认解析方式（当没有找到对应数据结构时）
  std::string parseAsDefault(const void *data, size_t data_size,
                             uint64_t timestamp) {
    std::string line = std::to_string(timestamp);

    // 按float数组处理
    if (data_size >= sizeof(float)) {
      const float *float_data = static_cast<const float *>(data);
      size_t float_count = data_size / sizeof(float);

      for (size_t i = 0; i < float_count && i < 20; ++i) {
        line += "," + std::to_string(float_data[i]);
      }
    }

    return line;
  }

  // 刷新缓冲区到文件
  void flushBuffer(int64_t key) {
    auto buffer_it = msg_id_to_buffer_.find(key);
    auto ofs_it = msg_id_to_ofs_.find(key);

    if (buffer_it != msg_id_to_buffer_.end() &&
        ofs_it != msg_id_to_ofs_.end() && !buffer_it->second.empty()) {

      // 批量写入
      *ofs_it->second << buffer_it->second;
      ofs_it->second->flush(); // 强制刷新到磁盘

      // 清空缓冲区
      buffer_it->second.clear();
      msg_id_to_count_[key] = 0;
      msg_id_to_last_flush_[key] = std::chrono::steady_clock::now();
    }
  }

  static XrealLinkOnlySaveFile *getInstance() {
    static XrealLinkOnlySaveFile instance;
    return &instance;
  }

  void setSaveDir(const std::string save_dir) { save_dir_ = save_dir; }

  void setMapMsgIdToMsgName(const int64_t group_id, const int64_t msg_id,
                            const int64_t src_id, const std::string msg_name) {

    int64_t key = XrealLinkCommon::getKey(group_id, msg_id, src_id);
    msg_id_to_msg_name_[key] = msg_name;
  }

  std::string getMsgNameByXreallinkIds(const int64_t group_id,
                                       const int64_t msg_id,
                                       const int64_t src_id) {

    int64_t key = XrealLinkCommon::getKey(group_id, msg_id, src_id);

    auto it = msg_id_to_msg_name_.find(key);
    if (it != msg_id_to_msg_name_.end()) {
      return it->second;
    }

    return "unkown";
  }

  std::shared_ptr<std::ofstream> getOfsByXreallinkIds(const int64_t group_id,
                                                      const int64_t msg_id,
                                                      const int64_t src_id) {

    int64_t key = XrealLinkCommon::getKey(group_id, msg_id, 0);
    auto it = msg_id_to_ofs_.find(key);
    if (it != msg_id_to_ofs_.end()) {
      if (it->second->is_open()) {
        return it->second;
      }
    }

    if ("" == filename_prefix_) {
      // get absolute wall time
      auto now = std::chrono::system_clock::now();
      filename_prefix_ =
          file_date_time(std::chrono::system_clock::to_time_t(now));
    }

    if ("" == save_dir_with_date_) {
      auto now = std::chrono::system_clock::now();
      save_dir_with_date_ =
          save_dir_ + "/" +
          file_date(std::chrono::system_clock::to_time_t(now));
      if (-1 ==
          access(
              save_dir_with_date_.c_str(),
              F_OK)) { // access第二个参数：F_OK表示判断存在，6表示判断是否可以读写
        if (mkdir(save_dir_with_date_.c_str(), 0777) == 0) {
          TRACKING_LOG_DEBUG("create xreal_link save_dir {} success.",
                             save_dir_with_date_);
        } else {
          TRACKING_LOG_WARN("create xreal_link save_dir {} failed.",
                            save_dir_with_date_);
        }
      }
    }

    std::string msg_name = getMsgNameByXreallinkIds(group_id, msg_id, 0);
    std::string filename =
        save_dir_with_date_ + "/" + filename_prefix_ + "_" + msg_name + ".csv";

    std::shared_ptr<std::ofstream> ofs_ptr;
    ofs_ptr.reset(
        new std::ofstream(filename, std::ofstream::out | std::ofstream::app));

    msg_id_to_ofs_[key] = ofs_ptr;

    // 使用数据结构管理器生成CSV头
    std::string csv_header =
        structure_manager_.generateCsvHeader(group_id, msg_id);
    if (csv_header.empty()) {
      // 使用默认头：只有timestamp和数据
      *ofs_ptr << "timestamp, ";
      for (int i = 0; i < 20; ++i) {
        *ofs_ptr << "data" << i;
        if (i < 19)
          *ofs_ptr << ", ";
      }
    } else {
      // 只包含timestamp和结构化数据字段
      *ofs_ptr << "timestamp, " << csv_header;
    }
    *ofs_ptr << "\n";

    TRACKING_LOG_DEBUG("create xreal_link filename {} success.", filename);

    return ofs_ptr;
  }
};
} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif
