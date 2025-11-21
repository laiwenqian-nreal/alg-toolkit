#ifndef PROJECT_XREAL_LINK_PROTOBUF_GRPC_H
#define PROJECT_XREAL_LINK_PROTOBUF_GRPC_H

#include "alg.grpc.pb.h"
#include "xreal_link_common.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

using alg::v1::Ack;
using alg::v1::AlgService;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::LatencyData;
using alg::v1::RawImuData;

namespace xreal {
namespace toolkits {
namespace datadump {

class XrealLinkgRPC : public XrealLinkCommon {
protected:
  std::unique_ptr<AlgService::Stub> stub_;
  std::string server_address_;
  bool connected_ = false;

  XrealLinkgRPC(const std::string &server_address = "localhost:50051");

public:
  ~XrealLinkgRPC();

  void _Close() override;

  static void Close();

  // 连接到 gRPC 服务器
  bool connectServer();

  // 静态接口函数 - 显式声明避免链接问题
  static void linkSendStatus(int group_id, int msg_id, const uint8_t *data,
                             uint64_t len);

  template <typename T>
  static void linkSendStatus(int group_id, int msg_id,
                             const std::vector<T> &vec) {
    size_t data_size = vec.size() * sizeof(T);
    linkSendStatus(group_id, msg_id, (const uint8_t *)vec.data(), data_size);
  }

  // 重载基类的processGroupMsg - gRPC版本
  void processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                       uint64_t timestamp_ns, uint64_t packet_id) override;

  static XrealLinkgRPC *
  getInstance(const std::string &server_address = "localhost:50051");

protected:
  // 实现基类的纯虚函数 - gRPC专用的发送线程
  void sendThread() override;

private:
  // 发送单个消息
  bool sendMsg(DataBuffer msg);

  // 发送 RawImuData
  bool sendRawImuData(uint32_t group_id, uint32_t msg_id,
                      uint64_t onsensor_timestamp_us,
                      RawImuDataDumpStruct *data, uint32_t data_len);

  // 发送 LatencyData
  bool sendLatencyData(uint32_t group_id, uint32_t msg_id,
                       uint64_t onsensor_timestamp_us,
                       RawLatencyDataDumpStruct *data, uint32_t data_len);

  // 发送 BinaryData
  bool sendBinaryData(uint32_t group_id, uint32_t msg_id,
                      uint64_t onsensor_timestamp_us, const uint8_t *data,
                      uint32_t data_len);

  // 发送 ImageData
  bool sendImageData(uint32_t group_id, uint32_t msg_id,
                     uint64_t onsensor_timestamp_us, const uint8_t *data,
                     uint32_t data_len);
};

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif
