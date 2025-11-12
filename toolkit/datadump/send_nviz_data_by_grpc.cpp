#include "send_nviz_data_by_grpc.h"
#include <errno.h> /* EINPROGRESS, errno */
#include <iostream>
#include "../util/logging.h"
using xreal::toolkits::utils::Logger;

namespace xreal {
namespace toolkits {
namespace datadump {

std::unique_ptr<AlgService::Stub> XrealLinkgRPC::make_stub(const std::string& target) {
  grpc::ChannelArguments args;
  // 大消息可放开 (可选)
  // args.SetMaxSendMessageSize(64*1024*1024);
  // args.SetMaxReceiveMessageSize(-1);
  auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
  return AlgService::NewStub(channel);
}

Ack XrealLinkgRPC::send_RawImuData(AlgService::Stub& stub, RawImuDataDumpStruct& imu_values) {
    RawImuData req;

    req.set_onsensor_timestamp_us(imu_values.onsensor_timestamp_us);
    req.set_timestamp_ns(imu_values.timestamp_ns);
    req.set_type(imu_values.type);
    req.set_data_1(imu_values.data[0]);
    req.set_data_2(imu_values.data[1]);
    req.set_data_3(imu_values.data[2]);
    req.set_data_4(imu_values.data[3]);
    req.set_data_5(imu_values.data[4]);
    req.set_data_6(imu_values.data[5]);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub.SendRawImuData(&ctx, req, &resp);
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}

Ack XrealLinkgRPC::send_BinaryData(AlgService::Stub& stub, uint64_t onsensor_timestamp_us,
    uint64_t timestamp_ns, uint32_t data_len, std::vector<uint8_t>& binary_data) {
    
    BinaryData req;

    req.set_onsensor_timestamp_us(onsensor_timestamp_us);
    req.set_timestamp_ns(timestamp_ns);
    req.set_data_len(data_len);
    req.set_payload(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub.SendBinaryData(&ctx, req, &resp);
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}


Ack XrealLinkgRPC::send_ImageData(AlgService::Stub& stub, uint64_t onsensor_timestamp_us,
    uint64_t timestamp_ns, std::string& filename, std::string& buf) {

    ImageData req;

    req.set_onsensor_timestamp_us(onsensor_timestamp_us);
    req.set_timestamp_ns(timestamp_ns);
    req.set_filename(filename);
    req.set_image_binary_data(std::move(buf));

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub.SendImageDate(&ctx, req, &resp);
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}


} // namespace datadump
} // namespace toolkits
} // namespace xreal
