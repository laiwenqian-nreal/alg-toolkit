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

Ack XrealLinkgRPC::send_RawImuData(AlgService::Stub& stub,  uint64_t onsensor_timestamp_us, 
                        uint64_t timestamp_ns, uint32_t type, float data_1, float data_2,
                        float data_3, float data_4, float data_5, float data_6) {
    
    RawImuData req;

    req.set_onsensor_timestamp_us(onsensor_timestamp_us);
    req.set_timestamp_ns(timestamp_ns);
    req.set_type(type);
    req.set_data_1(data_1);
    req.set_data_2(data_2);
    req.set_data_3(data_3);
    req.set_data_4(data_4);
    req.set_data_5(data_5);
    req.set_data_6(data_6);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub.SendRawImuData(&ctx, req, &resp);
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}

Ack XrealLinkgRPC::send_BinaryData(AlgService::Stub& stub, uint64_t onsensor_timestamp_us,
                        uint64_t timestamp_ns, uint32_t data_len, uint8_t* data) {
    
    BinaryData req;

    req.set_onsensor_timestamp_us(onsensor_timestamp_us);
    req.set_timestamp_ns(timestamp_ns);
    req.set_data_len(data_len);
    req.set_payload(reinterpret_cast<const char*>(data), static_cast<size_t>(data_len));

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