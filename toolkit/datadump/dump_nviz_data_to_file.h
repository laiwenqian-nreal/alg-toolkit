#ifndef PROJECT_XREAL_LINK_ONLY_SAVE_FILE_H
#define PROJECT_XREAL_LINK_ONLY_SAVE_FILE_H

#include "xreal_link_common.h"
#include <chrono>  // 头文件中需要 std::chrono::steady_clock::time_point
#include <fstream> // 头文件中需要 std::shared_ptr<std::ofstream>
#include <map>     // 头文件中需要 std::map

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
  std::map<uint64_t, std::string> msg_id_to_filename_;
  std::map<std::string, std::shared_ptr<std::ofstream>> filename_to_ofs_;
  std::map<std::string, std::string>
      filename_to_buffer_; // 使用文件名作为键的缓冲区
  std::string filename_prefix_ = "";
  std::string save_dir_ = "";

  // 批量写入配置
  static const size_t BUFFER_SIZE_LIMIT = 8192; // 8KB缓冲区
  static const size_t BUFFER_COUNT_LIMIT = 100; // 100条记录
  std::map<std::string, size_t> filename_to_count_; // 使用文件名的记录计数
  std::map<std::string, std::chrono::steady_clock::time_point>
      filename_to_last_flush_; // 使用文件名的上次刷新时间
  static constexpr std::chrono::milliseconds FLUSH_INTERVAL{50}; // 50ms强制刷新

  XrealLinkOnlySaveFile(std::string save_dir);

public:
  ~XrealLinkOnlySaveFile();

  void _Close() override;

  static void Close();

  // 静态接口函数 - 显式声明避免链接问题
  static void linkSendStatus(int group_id, int msg_id, const uint8_t *data,
                             uint64_t len);
  
  template <typename T>
  static void linkSendStatus(int group_id, int msg_id,
                             const std::vector<T> &vec) {
    size_t data_size = vec.size() * sizeof(T);
    linkSendStatus(group_id, msg_id, (const uint8_t *)vec.data(), data_size);
  }

  // 重载基类的processGroupMsg - 文件保存版本
  void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                       uint64_t timestamp_ns, uint64_t packet_id) override;

protected:
  // 实现基类的纯虚函数 - 文件保存专用的发送线程
  void sendThread() override;

public:
  bool saveMsg(DataBuffer msg);

private:
  // 使用数据结构管理器保存结构化数据
  void saveStructuredData(uint64_t group_id, uint64_t msg_id, const void *data,
                          size_t data_size, uint64_t timestamp);

  // 默认解析方式（当没有找到对应数据结构时）
  std::string parseAsDefault(const void *data, size_t data_size,
                             uint64_t timestamp);

  // 刷新缓冲区到文件
  void flushBuffer(const std::string &filename);

public:
  static XrealLinkOnlySaveFile *getInstance(const std::string &save_dir = "./");

  void setMapMsgIdToFilename(const uint64_t group_id, const uint64_t msg_id,
                             const std::string &filename);

  std::string getFilenameByXreallinkIds(const uint64_t group_id,
                                        const uint64_t msg_id);

private:
  std::shared_ptr<std::ofstream> getOfsByXreallinkIds(const uint64_t group_id,
                                                      const uint64_t msg_id);
};
} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif
