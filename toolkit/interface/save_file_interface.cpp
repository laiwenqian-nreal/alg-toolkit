#include "dump_nviz_data_to_file.h"
#include <cstring>

using xreal::toolkits::datadump::XrealLinkOnlySaveFile;

#define MY_API __attribute__((visibility("default")))

extern "C" {

// 初始化保存文件接口
MY_API void InitSaveFile(const char *save_dir, bool add_header) {
  if (save_dir == nullptr) {
    save_dir = "./data";
  }
  // 设置全局配置
  XrealLinkOnlySaveFile::setAddHeader(add_header);
  XrealLinkOnlySaveFile::getInstance(save_dir);
}
// 设置group_id和msg_id对应的文件名
MY_API void SetFilenameMapping(uint64_t group_id, uint64_t msg_id,
                               const char *filename) {
  if (filename == nullptr) {
    return;
  }
  XrealLinkOnlySaveFile::getInstance()->setMapMsgIdToFilename(group_id, msg_id,
                                                              filename);
}

MY_API void SaveData(int group_id, int msg_id, const uint8_t *data,
                     uint64_t len) {
  if (data == nullptr || len == 0) {
    return;
  }
  XrealLinkOnlySaveFile::linkSendStatus(group_id, msg_id, data, len);
}

MY_API void CloseSaveFile() { XrealLinkOnlySaveFile::Close(); }
}
