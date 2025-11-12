#ifndef PROJECT_XREAL_LINK_PROTOBUF_GRPC_H
#define PROJECT_XREAL_LINK_PROTOBUF_GRPC_H

#include <grpcpp/grpcpp.h>
#include "alg.grpc.pb.h"
#include "xreal_link_common.h"
#include <iostream>
#include <thread>

using alg::v1::AlgService;
using alg::v1::ChunkMeta;
using alg::v1::RawImuDataDump;
using alg::v1::DataChunk;
using alg::v1::Ack;

using namespace xreal::toolkits::datadump;

std::unique_ptr<AlgService::Stub> make_stub(const std::string& target) {
  grpc::ChannelArguments args;
  // 大消息可放开 (可选)
  // args.SetMaxSendMessageSize(64*1024*1024);
  // args.SetMaxReceiveMessageSize(-1);
  auto channel = grpc::CreateCustomChannel(target, grpc::InsecureChannelCredentials(), args);
  return AlgService::NewStub(channel);
}

Ack send_once(AlgService::Stub& stub, RawImuDataDumpStruct& imu_values) {
    DataChunk req;
    req.mutable_meta()->set_group_id(127);
    req.mutable_meta()->set_msg_id(1);
    req.mutable_meta()->set_seq(1);

    req.mutable_data()->set_onsensor_timestamp_us(imu_values.onsensor_timestamp_us);
    req.mutable_data()->set_timestamp_ns(imu_values.timestamp_ns);
    req.mutable_data()->set_type(imu_values.type);
    req.mutable_data()->set_data_1(imu_values.data[0]);
    req.mutable_data()->set_data_2(imu_values.data[1]);
    req.mutable_data()->set_data_3(imu_values.data[2]);
    req.mutable_data()->set_data_4(imu_values.data[3]);
    req.mutable_data()->set_data_5(imu_values.data[4]);
    req.mutable_data()->set_data_6(imu_values.data[5]);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub.SendOnce(&ctx, req, &resp);
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}

Ack upload_stream(AlgService::Stub& stub, std::vector<RawImuDataDumpStruct>& blocks) {
    grpc::ClientContext ctx;
    Ack resp;
    auto writer = stub.Upload(&ctx, &resp);
    uint64_t seq = 1;
    for(auto& b : blocks) {
        DataChunk req;
        req.mutable_meta()->set_group_id(127);
        req.mutable_meta()->set_msg_id(1);
        req.mutable_meta()->set_seq(1);

        req.mutable_data()->set_onsensor_timestamp_us(b.onsensor_timestamp_us);
        req.mutable_data()->set_timestamp_ns(b.timestamp_ns);
        req.mutable_data()->set_type(b.type);
        req.mutable_data()->set_data_1(b.data[0]);
        req.mutable_data()->set_data_2(b.data[1]);
        req.mutable_data()->set_data_3(b.data[2]);
        req.mutable_data()->set_data_4(b.data[3]);
        req.mutable_data()->set_data_5(b.data[4]);
        req.mutable_data()->set_data_6(b.data[5]);

        if (!writer->Write(req)) break; // 对端关闭/背压
    }
    writer->WritesDone();
    auto status = writer->Finish();
    if (!status.ok()) throw std::runtime_error(status.error_message());
    return resp;
}

#endif
