#include "alg.grpc.pb.h"
#include "alg.pb.h"
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

// cpp-refactor 的 Publisher 和 Message 类
#include "publisher.h"
#include "message.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// Import proto namespace
using alg::v1::Ack;
using alg::v1::AlgService;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::LatencyData;
using alg::v1::RawImuData;

// 使用 echo 命名空间
using echo::Publisher;
using echo::ImuMessage;
using echo::LatencyMessage;
using echo::ImageMessage;
using echo::BinaryMessage;

/**
 * @brief gRPC 服务端实现类 - 通过 Topic 发布接收到的数据
 */
class AlgServiceTopicImpl final : public AlgService::Service {
public:
  AlgServiceTopicImpl() {
    std::cout << "=== gRPC Receiver with Topic Publisher Started ===" << std::endl;
    received_count_ = 0;

    // 初始化发布者 - 为不同数据类型创建不同的 topic
    imu_publisher_ = std::make_shared<Publisher>("grpc/imu");
    latency_publisher_ = std::make_shared<Publisher>("grpc/latency");
    image_publisher_ = std::make_shared<Publisher>("grpc/image");
    binary_publisher_ = std::make_shared<Publisher>("grpc/binary");

    std::cout << "  IMU Publisher on port: " << imu_publisher_->getPort() << std::endl;
    std::cout << "  Latency Publisher on port: " << latency_publisher_->getPort()
              << std::endl;
    std::cout << "  Image Publisher on port: " << image_publisher_->getPort()
              << std::endl;
    std::cout << "  Binary Publisher on port: " << binary_publisher_->getPort()
              << std::endl;
  }

  ~AlgServiceTopicImpl() { }

  // 接收 RawImuData 并通过 topic 发布
  Status SendRawImuData(ServerContext *context, const RawImuData *request,
                        Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] RawImuData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Type:" << request->type() << std::endl;

    // 提取数据
    uint64_t timestamp_ns = request->timestamp_ns();
    int type = request->type();
    double data[6] = {request->data_1(), request->data_2(), request->data_3(),
                      request->data_4(), request->data_5(), request->data_6()};

    std::cout << "  timestamp_ns: " << timestamp_ns << std::endl;
    std::cout << "  type: " << type << std::endl;
    std::cout << "  data: [";
    for (int i = 0; i < 6; ++i) {
      std::cout << data[i];
      if (i < 5)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    // 创建 IMU 消息并发布
    auto imu_msg = std::make_shared<ImuMessage>(
        "grpc/imu", timestamp_ns, type, data[0], data[1], data[2], data[3],
        data[4], data[5]);

    try {
      imu_publisher_->publish(imu_msg);
      std::cout << "  Published to grpc/imu topic" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "  Error publishing IMU message: " << e.what() << std::endl;
      response->set_note("Failed to publish IMU data");
      return Status(grpc::StatusCode::INTERNAL,
                    "Failed to publish IMU data");
    }

    response->set_note("RawImuData received and published");
    return Status::OK;
  }

  // 接收 LatencyData 并通过 topic 发布
  Status SendLatencyData(ServerContext *context, const LatencyData *request,
                         Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] LatencyData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Type:" << request->type() << std::endl;

    // 提取数据
    uint64_t timestamp_ns = request->timestamp_ns();
    int type = request->type();
    double data[6] = {(double)request->data_1(), (double)request->data_2(),
                      (double)request->data_3(), (double)request->data_4(),
                      (double)request->data_5(), (double)request->data_6()};

    std::cout << "  timestamp_ns: " << timestamp_ns << std::endl;
    std::cout << "  type: " << type << std::endl;
    std::cout << "  data: [";
    for (int i = 0; i < 6; ++i) {
      std::cout << data[i];
      if (i < 5)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    // 创建延迟消息并发布
    auto latency_msg = std::make_shared<LatencyMessage>(
        "grpc/latency", timestamp_ns, type, data[0], data[1], data[2],
        data[3], data[4], data[5]);

    try {
      latency_publisher_->publish(latency_msg);
      std::cout << "  Published to grpc/latency topic" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "  Error publishing Latency message: " << e.what()
                << std::endl;
      response->set_note("Failed to publish Latency data");
      return Status(grpc::StatusCode::INTERNAL,
                    "Failed to publish Latency data");
    }

    response->set_note("LatencyData received and published");
    return Status::OK;
  }

  // 接收 BinaryData 并通过 topic 发布
  Status SendBinaryData(ServerContext *context, const BinaryData *request,
                        Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] BinaryData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Size:" << request->data_len() << " bytes" << std::endl;

    // 提取二进制数据
    const std::string &payload = request->payload();
    std::vector<uint8_t> binary_data(payload.begin(), payload.end());

    std::cout << "  payload size: " << binary_data.size() << " bytes"
              << std::endl;

    // 创建二进制消息并发布
    auto binary_msg = std::make_shared<BinaryMessage>("grpc/binary", binary_data);

    try {
      binary_publisher_->publish(binary_msg);
      std::cout << "  Published to grpc/binary topic" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "  Error publishing Binary message: " << e.what()
                << std::endl;
      response->set_note("Failed to publish Binary data");
      return Status(grpc::StatusCode::INTERNAL,
                    "Failed to publish Binary data");
    }

    response->set_note("BinaryData received and published");
    return Status::OK;
  }

  // 接收 ImageData 并通过 topic 发布
  Status SendImageData(ServerContext *context, const ImageData *request,
                       Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] ImageData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Size:" << request->image_binary_data().size() << " bytes"
              << std::endl;

    // 提取图像数据
    const std::string &image_payload = request->image_binary_data();
    std::vector<uint8_t> image_data(image_payload.begin(), image_payload.end());

    std::cout << "  image data size: " << image_data.size() << " bytes"
              << std::endl;

    // 创建图像消息并发布
    auto image_msg = std::make_shared<ImageMessage>("grpc/image", image_data);

    try {
      image_publisher_->publish(image_msg);
      std::cout << "  Published to grpc/image topic" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "  Error publishing Image message: " << e.what() << std::endl;
      response->set_note("Failed to publish Image data");
      return Status(grpc::StatusCode::INTERNAL,
                    "Failed to publish Image data");
    }

    response->set_note("ImageData received and published");
    return Status::OK;
  }

private:
  uint64_t received_count_;
  std::shared_ptr<Publisher> imu_publisher_;
  std::shared_ptr<Publisher> latency_publisher_;
  std::shared_ptr<Publisher> image_publisher_;
  std::shared_ptr<Publisher> binary_publisher_;
};

void RunTopicServer(const std::string &server_address) {
  AlgServiceTopicImpl service;

  ServerBuilder builder;

  // 设置大消息支持
  builder.SetMaxReceiveMessageSize(128 * 1024 * 1024); // 128MB
  builder.SetMaxSendMessageSize(128 * 1024 * 1024);

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  std::cout << "\nPress Ctrl+C to quit\n" << std::endl;

  server->Wait();
}

int main(int argc, char **argv) {
  std::string server_address = "127.0.0.1:50051";

  // 解析命令行参数
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                << "Options:\n"
                << "  -a, --address <addr>  Server address (default: "
                   "127.0.0.1:50051)\n"
                << "  -h, --help            Show this help message\n"
                << "\nData will be published through ROS-like topics:\n"
                << "  - grpc/imu: IMU data\n"
                << "  - grpc/latency: Latency data\n"
                << "  - grpc/image: Image data\n"
                << "  - grpc/binary: Binary data\n"
                << std::endl;
      return 0;
    } else if ((arg == "-a" || arg == "--address") && i + 1 < argc) {
      server_address = argv[++i];
    }
  }

  try {
    RunTopicServer(server_address);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
