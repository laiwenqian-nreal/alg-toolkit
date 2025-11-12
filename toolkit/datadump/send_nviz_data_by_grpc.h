#ifndef PROJECT_XREAL_LINK_PROTOBUF_GRPC_H
#define PROJECT_XREAL_LINK_PROTOBUF_GRPC_H

#include <grpcpp/grpcpp.h>
#include "alg.grpc.pb.h"
#include "xreal_link_common.h"
#include <thread>

using alg::v1::AlgService;
using alg::v1::RawImuData;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::Ack;

namespace xreal {
namespace toolkits {
namespace datadump {
class XrealLinkgRPC : public XrealLinkCommon {
public:
    static std::unique_ptr<AlgService::Stub> make_stub(const std::string& target);
    static Ack send_RawImuData(AlgService::Stub& stub, RawImuDataDumpStruct& imu_values);
    static Ack send_BinaryData(AlgService::Stub& stub, uint64_t onsensor_timestamp_us,
                                uint64_t timestamp_ns, uint32_t data_len, std::vector<uint8_t>& binary_data);
    static Ack send_ImageData(AlgService::Stub& stub, uint64_t onsensor_timestamp_us,
                                uint64_t timestamp_ns, std::string& filename, std::string& buf);
}; // namespace toolkits
} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif
