#include <grpcpp/grpcpp.h>
#include "alg.grpc.pb.h"
#include "../toolkit/datadump/send_nviz_data_by_grpc.h"

using alg::v1::AlgService;
using alg::v1::RawImuData;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::Ack;
using xreal::toolkits::datadump::XrealLinkgRPC;

std::unique_ptr<AlgService::Stub> stub;

#define MY_API __attribute__((visibility("default")))

extern "C" {
MY_API void MakeStub(char* target_name, size_t target_size) {
    std::string target = "";
    if (target_size > 0) {
        target = std::string(target_name, target_size);
    }
    stub = XrealLinkgRPC::make_stub(target);
}

MY_API void SendRawImuData(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        uint32_t type, float data_1, float data_2, float data_3,
                        float data_4, float data_5, float data_6) {
    
    auto ack1 = XrealLinkgRPC::send_RawImuData(*stub, onsensor_timestamp_us, timestamp_ns,
                                        type, data_1, data_2, data_3, data_4, data_5, data_6);
}

MY_API void SendBinaryData(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        uint32_t data_len, uint8_t* data) {
    
    auto ack2 = XrealLinkgRPC::send_BinaryData(*stub, onsensor_timestamp_us, 
                                                timestamp_ns, data_len, data);
}


MY_API void SendImageData(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        char* file_name, size_t file_name_size,
                        char* buf_name, size_t buf_size) {

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