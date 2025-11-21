#include "send_nviz_data_by_grpc.h"
#include "alg.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using alg::v1::Ack;
using alg::v1::AlgService;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::RawImuData;
using xreal::toolkits::datadump::XrealLinkgRPC;

std::unique_ptr<AlgService::Stub> stub;

struct RawImuDataDumpStruct {
  uint64_t onsensor_timestamp_us;
  uint64_t timestamp_ns;
  uint32_t type;
  float data[6];
};

#define MY_API __attribute__((visibility("default")))

extern "C" {
MY_API void GRPCMakeStub(char *target_name, size_t target_size) {
  std::string target = "";
  if (target_size > 0) {
    target = std::string(target_name, target_size);
  }
  stub = XrealLinkgRPC::make_stub(target);
}

// 为了分辨不同的IMU，这里可能需要传入group_id和msg_id，暂时先不加
MY_API void SendGRPCRawImuData(uint64_t onsensor_timestamp_us,
                               uint64_t timestamp_ns, uint32_t type,
                               float data_1, float data_2, float data_3,
                               float data_4, float data_5, float data_6) {

  auto ack1 = XrealLinkgRPC::send_RawImuData(*stub, onsensor_timestamp_us,
                                             timestamp_ns, type, data_1, data_2,
                                             data_3, data_4, data_5, data_6);
}

MY_API void SendGRPCData(int group_id, int msg_id, const uint8_t *data,
                         uint64_t len) {

  RawImuDataDumpStruct *imu_data = (RawImuDataDumpStruct *)data;

  auto ack1 = XrealLinkgRPC::send_RawImuData(
      *stub, imu_data->onsensor_timestamp_us, imu_data->timestamp_ns,
      imu_data->type, imu_data->data[0], imu_data->data[1], imu_data->data[2],
      imu_data->data[3], imu_data->data[4], imu_data->data[5]);
}

MY_API void SendGRPCBinaryData(uint64_t onsensor_timestamp_us,
                               uint64_t timestamp_ns, uint32_t data_len,
                               uint8_t *data) {

  auto ack2 = XrealLinkgRPC::send_BinaryData(*stub, onsensor_timestamp_us,
                                             timestamp_ns, data_len, data);
}

MY_API void SendGRPCImageData(uint64_t onsensor_timestamp_us,
                              uint64_t timestamp_ns, char *file_name,
                              size_t file_name_size, char *buf_name,
                              size_t buf_size) {

  std::string filename = "";
  if (file_name_size > 0) {
    filename = std::string(file_name, file_name_size);
  }

  std::string buf = "";
  if (buf_size > 0) {
    buf = std::string(buf_name, buf_size);
  }

  auto ack3 = XrealLinkgRPC::send_ImageData(*stub, onsensor_timestamp_us,
                                            timestamp_ns, filename, buf);
}
}