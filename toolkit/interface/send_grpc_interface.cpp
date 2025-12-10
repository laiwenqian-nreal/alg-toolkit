#include "send_nviz_data_by_grpc.h"

using xreal::toolkits::datadump::XrealLinkgRPC;
using namespace xreal::toolkits::datadump; // 添加这个命名空间

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

// 发送原始 IMU 数据
MY_API void SendGRPCRawImuData(uint64_t onsensor_timestamp_us,
                               uint64_t timestamp_ns, uint32_t type,
                               float data_1, float data_2, float data_3,
                               float data_4, float data_5, float data_6) {
  // 构建 RawImuDataDumpStruct 结构
  RawImuDataDumpStruct imu_data;
  imu_data.timestamp_ns = timestamp_ns;
  imu_data.type = type;
  imu_data.data[0] = data_1;
  imu_data.data[1] = data_2;
  imu_data.data[2] = data_3;
  imu_data.data[3] = data_4;
  imu_data.data[4] = data_5;
  imu_data.data[5] = data_6;
  
  // 发送数据到 gRPC
  XrealLinkgRPC::linkSendStatus(1, 1, (const uint8_t *)&imu_data, sizeof(imu_data));
}

// 发送二进制数据
MY_API void SendGRPCBinaryData(uint64_t onsensor_timestamp_us,
                               uint64_t timestamp_ns, uint32_t data_len,
                               uint8_t *data) {
  if (data == nullptr || data_len == 0) {
    return;
  }
  
  // 发送数据到 gRPC
  XrealLinkgRPC::linkSendStatus(2, 9999, data, data_len);
}

// 发送图像数据
MY_API void SendGRPCImageData(uint64_t onsensor_timestamp_us,
                              uint64_t timestamp_ns, char *file_name,
                              size_t file_name_size, char *buf_name,
                              size_t buf_size) {
  if (buf_name == nullptr || buf_size == 0) {
    return;
  }
  
  // 发送数据到 gRPC
  XrealLinkgRPC::linkSendStatus(3, 401, (const uint8_t *)buf_name, buf_size);
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