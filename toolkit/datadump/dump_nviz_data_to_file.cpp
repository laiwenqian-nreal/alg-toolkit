#include "dump_nviz_data_to_file.h"
#include "send_nviz_data_by_tcp.h"
#include <chrono>
#include <errno.h> /* EINPROGRESS, errno */
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sys/stat.h>  // mkdir   rmdir
#include <sys/types.h> // mkdir   rmdir
#include <unistd.h>    // access

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(path) _mkdir(path)
#define REMOVE(path) _unlink(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#define REMOVE(path) remove(path)
#endif

#include "../util/logging.h"

using xreal::toolkits::utils::Logger;

namespace xreal {
namespace toolkits {
namespace datadump {

XrealLinkOnlySaveFile::XrealLinkOnlySaveFile(std::string save_dir) {
  log_prefix = "[xreal_link_file]";
  save_dir_ = save_dir + (save_dir.back() == '/' ? "" : "/");

  // Create or clear the directory
#ifdef _WIN32
  struct _finddata_t file_info;
  intptr_t handle = _findfirst((save_dir_ + "*").c_str(), &file_info);
  if (handle != -1) {
    DLOG_INFO("{} Clearing directory: {} {} {}", log_prefix, save_dir_,
              __FILE__, __LINE__);
  } else if (errno == ENOENT) {
    // Directory does not exist, create it
    if (MKDIR(save_dir_.c_str()) != 0) {
      DLOG_ERROR("{} Error creating directory: {} {} {}", log_prefix, save_dir_,
                 __FILE__, __LINE__);
    }
  } else {
    DLOG_INFO("{} Error opening directory: {} {} {}", log_prefix, save_dir_,
              __FILE__, __LINE__);
  }
#else
  DIR *dir = opendir(save_dir_.c_str());
  if (dir) {
    closedir(dir);
    DLOG_INFO("{} Clearing directory: {} {} {}", log_prefix, save_dir_,
              __FILE__, __LINE__);
  } else if (errno == ENOENT) {
    // Directory does not exist, create it
    if (MKDIR(save_dir_.c_str()) != 0) {
      DLOG_ERROR("{} Error creating directory: {} {} {}", log_prefix, save_dir_,
                 __FILE__, __LINE__);
    }
  } else {
    DLOG_ERROR("{} Error opening directory: {} {} {}", log_prefix, save_dir_,
               __FILE__, __LINE__);
  }
#endif
}

XrealLinkOnlySaveFile::~XrealLinkOnlySaveFile() { _Close(); }

void XrealLinkOnlySaveFile::_Close() {
  // 先刷新所有缓冲区
  for (auto &pair : filename_to_buffer_) {
    if (!pair.second.empty()) {
      flushBuffer(pair.first);
    }
  }

  filename_prefix_ = "";

  auto it_erase = filename_to_ofs_.begin();
  while (it_erase != filename_to_ofs_.end()) {
    it_erase = filename_to_ofs_.erase(it_erase);
  }

  filename_to_buffer_.clear();
  filename_to_count_.clear();
  filename_to_last_flush_.clear();

  // 清理线程
  cleanupThreads();
}

void XrealLinkOnlySaveFile::Close() {
  XrealLinkOnlySaveFile::getInstance()->_Close();
}

void XrealLinkOnlySaveFile::processGroupMsg(DataBuffer &group_msg,
                                            uint32_t freq_count,
                                            uint64_t timestamp_ns,
                                            uint64_t packet_id) {
  // 文件保存类直接将分组后的消息发送到send队列
  concurrent_lock_free_queue_send_.enqueue(group_msg);
}

void XrealLinkOnlySaveFile::sendThread() {
  DataBuffer msg;

  while (send_thread_running) {
    while (concurrent_lock_free_queue_send_.try_dequeue(msg)) {
      saveMsg(msg); // 文件保存不会失败，直接调用即可
    }

    // 如果队列为空，适当休眠避免忙等待
    if (concurrent_lock_free_queue_send_.size_approx() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5s休眠
    }
  }
}

bool XrealLinkOnlySaveFile::saveMsg(DataBuffer msg) {
  size_t curLen = msg.size();
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
    size_t len = msg_header->payload_length;
    uint64_t timestamp = get_current_timestamp_us(); // 入队列时间戳(微秒)

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

void XrealLinkOnlySaveFile::saveStructuredData(uint64_t group_id,
                                               uint64_t msg_id,
                                               const void *data,
                                               size_t data_size,
                                               uint64_t timestamp) {
  // 确保文件已创建并获取文件名
  auto ofs_ptr = getOfsByXreallinkIds(group_id, msg_id);

  if (!ofs_ptr) {
    return;
  }

  // 生成文件名作为键
  std::string msg_name = getFilenameByXreallinkIds(group_id, msg_id);

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

  // 添加到缓冲区（使用文件名作为键）
  filename_to_buffer_[msg_name] += csv_line;
  filename_to_count_[msg_name]++;

  // 检查是否需要刷新缓冲区
  auto now = std::chrono::steady_clock::now();
  bool should_flush = false;

  if (filename_to_buffer_[msg_name].size() >= BUFFER_SIZE_LIMIT) {
    should_flush = true;
  } else if (filename_to_count_[msg_name] >= BUFFER_COUNT_LIMIT) {
    should_flush = true;
  } else if (filename_to_last_flush_.find(msg_name) !=
                 filename_to_last_flush_.end() &&
             now - filename_to_last_flush_[msg_name] >= FLUSH_INTERVAL) {
    should_flush = true;
  } else if (filename_to_last_flush_.find(msg_name) ==
             filename_to_last_flush_.end()) {
    filename_to_last_flush_[msg_name] = now;
  }

  if (should_flush) {
    flushBuffer(msg_name);
  }
}

std::string XrealLinkOnlySaveFile::parseAsDefault(const void *data,
                                                  size_t data_size,
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

void XrealLinkOnlySaveFile::flushBuffer(const std::string &filename) {
  // 刷新所有文件的缓冲区
  for (auto &buffer_pair : filename_to_buffer_) {
    const std::string &file_name = buffer_pair.first;
    std::string &buffer = buffer_pair.second;
    
    if (!buffer.empty()) {
      // 查找对应的文件流
      auto ofs_it = filename_to_ofs_.find(file_name);
      if (ofs_it != filename_to_ofs_.end() && ofs_it->second && ofs_it->second->is_open()) {
        // 批量写入
        *ofs_it->second << buffer;
        ofs_it->second->flush(); // 强制刷新到磁盘
        
        // 清空缓冲区
        buffer.clear();
        filename_to_count_[file_name] = 0;
        filename_to_last_flush_[file_name] = std::chrono::steady_clock::now();
      } else {
        DLOG_INFO("No open file stream found for flushing: {} {}", file_name, __LINE__);
      }
    }
  }
}

XrealLinkOnlySaveFile *
XrealLinkOnlySaveFile::getInstance(const std::string &save_dir) {
  static XrealLinkOnlySaveFile instance(save_dir);
  return &instance;
}

void XrealLinkOnlySaveFile::setMapMsgIdToFilename(const uint64_t group_id,
                                                  const uint64_t msg_id,
                                                  const std::string &filename) {

  uint64_t key = XrealLinkCommon::getKey(group_id, msg_id);
  msg_id_to_filename_[key] = filename;
}

std::string
XrealLinkOnlySaveFile::getFilenameByXreallinkIds(const uint64_t group_id,
                                                 const uint64_t msg_id) {

  uint64_t key = XrealLinkCommon::getKey(group_id, msg_id);

  auto it = msg_id_to_filename_.find(key);
  if (it != msg_id_to_filename_.end()) {
    return it->second;
  }

  return "";
}

std::shared_ptr<std::ofstream>
XrealLinkOnlySaveFile::getOfsByXreallinkIds(const uint64_t group_id,
                                            const uint64_t msg_id) {

  std::string file_name = getFilenameByXreallinkIds(group_id, msg_id);
  if (file_name.empty()) {
    return nullptr;
  }
  // 确保文件前缀已设置
  if ("" == filename_prefix_) {
    // get absolute wall time
    auto now = std::chrono::system_clock::now();
    filename_prefix_ =
        file_date_time(std::chrono::system_clock::to_time_t(now));
  }

  auto it = filename_to_ofs_.find(file_name);
  if (it != filename_to_ofs_.end()) {
    if (it->second->is_open()) {
      return it->second;
    } else {
      // 文件存在但未打开，重新打开
      std::string filename = save_dir_+ "/" + filename_prefix_ +
                             "_" + file_name + ".csv";
      it->second->open(filename, std::ofstream::out | std::ofstream::app);
      if (it->second->is_open()) {
        return it->second;
      }
    }
  }

  if (save_dir_.empty()) {
    // get absolute wall time
    auto now = std::chrono::system_clock::now();
    // 创建日期目录
    if (access(save_dir_.c_str(), 0) != 0) {
      if (MKDIR(save_dir_.c_str()) != 0) {
        DLOG_ERROR("{} Error creating directory: {} {} {}", log_prefix,
                   save_dir_, __FILE__, __LINE__);
      }
    }
  }

  std::string filename = save_dir_+ "/" + filename_prefix_ +
                         "_local_" + file_name + ".csv";

  std::shared_ptr<std::ofstream> ofs_ptr;
  ofs_ptr.reset(
      new std::ofstream(filename, std::ofstream::out | std::ofstream::app));

  filename_to_ofs_[file_name] = ofs_ptr;

  // 使用数据结构管理器生成CSV头
  std::string csv_header =
      structure_manager_.generateCsvHeader(group_id, msg_id);
  if (csv_header.empty()) {
    // 使用默认头：只有timestamp和数据
    *ofs_ptr << "timestamp, ";
    for (int i = 0; i < 20; ++i) {
      *ofs_ptr << " data" << i;
      if (i < 19)
        *ofs_ptr << ",";
    }
  } else {
    // 只包含timestamp和结构化数据字段
    *ofs_ptr << "timestamp, " << csv_header;
  }
  *ofs_ptr << "\n";

  DLOG_INFO("create xreal_link filename {} success.", filename);

  filename_to_ofs_[file_name] = ofs_ptr;
  return ofs_ptr;
}

void XrealLinkOnlySaveFile::linkSendStatus(int group_id, int msg_id, 
                                           const uint8_t *data, uint64_t len) {
  DataBuffer raw_msg;
  raw_msg.resize(sizeof(XrealLinkCommon::RawMessageHeader) + len);
  XrealLinkCommon::RawMessageHeader *raw_header = 
      (XrealLinkCommon::RawMessageHeader *)raw_msg.data();
  raw_header->group_id = group_id;
  raw_header->msg_id = msg_id;
  if (data && len > 0) {
    memcpy(raw_msg.data() + sizeof(XrealLinkCommon::RawMessageHeader), data, len);
  }
  XrealLinkOnlySaveFile::getInstance()->messageEnQueue(raw_msg);
}

} // namespace datadump
} // namespace toolkits
} // namespace xreal
