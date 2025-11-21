#include "send_nviz_data_by_grpc.h"

using xreal::toolkits::datadump::XrealLinkgRPC;

#define MY_API __attribute__((visibility("default")))

extern "C" {

// 初始化 gRPC 连接
MY_API void GRPCMakeStub(char *target_name, size_t target_size) {
  std::string target = "localhost:50051";
  if (target_size > 0) {
    target = std::string(target_name, target_size);
  }
  // 使用新的接口初始化 gRPC 实例
  XrealLinkgRPC::getInstance(target);
}

// 发送数据
MY_API void SendGRPCData(int group_id, int msg_id, const uint8_t *data,
                         uint64_t len) {
  if (data == nullptr || len == 0) {
    return;
  }
  XrealLinkgRPC::linkSendStatus(group_id, msg_id, data, len);
}

// 关闭 gRPC 连接
MY_API void CloseGRPC() {
  XrealLinkgRPC::Close();
}

}