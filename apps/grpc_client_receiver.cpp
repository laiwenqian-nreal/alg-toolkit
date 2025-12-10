#include "alg.grpc.pb.h"
#include "alg.pb.h"
#include "toolkit/datadump/data_structure_manager.h"
#include <chrono>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace xreal::toolkits::datadump;

// Import proto namespace
using alg::v1::Ack;
using alg::v1::AlgService;
using alg::v1::BinaryData;
using alg::v1::ImageData;
using alg::v1::LatencyData;
using alg::v1::RawImuData;

// 创建目录（如果不存在）
bool createDirectory(const std::string &path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    // 目录不存在，创建它
    if (mkdir(path.c_str(), 0755) != 0) {
      std::cerr << "Failed to create directory: " << path << std::endl;
      return false;
    }
  } else if (!(info.st_mode & S_IFDIR)) {
    std::cerr << "Path exists but is not a directory: " << path << std::endl;
    return false;
  }
  return true;
}

// gRPC 服务端实现类
class AlgServiceImpl final : public AlgService::Service {
public:
  AlgServiceImpl(const std::string &save_dir)
      : save_dir_(save_dir), structure_manager_() {

    std::cout << "=== gRPC Receiver Server Started ===" << std::endl;
    std::cout << "Save Directory: " << save_dir_ << std::endl;

    // 确保保存目录存在
    if (!createDirectory(save_dir_)) {
      throw std::runtime_error("Failed to create save directory");
    }

    received_count_ = 0;
  }

  ~AlgServiceImpl() {
    // 关闭所有文件并刷新缓冲区
    for (auto &pair : filename_to_buffer_) {
      flushBuffer(pair.first);
    }
    filename_to_ofs_.clear();
  }

  // 接收 RawImuData
  Status SendRawImuData(ServerContext *context, const RawImuData *request,
                        Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] RawImuData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Type:" << request->type() << std::endl;

    // 构造 RawImuDataDumpStruct 并保存
    RawImuDataDumpStruct imu_data;
    imu_data.timestamp_ns = request->timestamp_ns();
    imu_data.type = request->type();
    imu_data.data[0] = request->data_1();
    imu_data.data[1] = request->data_2();
    imu_data.data[2] = request->data_3();
    imu_data.data[3] = request->data_4();
    imu_data.data[4] = request->data_5();
    imu_data.data[5] = request->data_6();

    std::cout << "  onsensor_timestamp_us: " << header.onsensor_timestamp_us()
              << std::endl;
    std::cout << "  timestamp_ns: " << imu_data.timestamp_ns << std::endl;
    std::cout << "  type: " << imu_data.type << std::endl;
    std::cout << "  data: [";
    for (int i = 0; i < 6; ++i) {
      std::cout << imu_data.data[i];
      if (i < 5)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    saveStructuredData(header.group_id(), header.msg_id(), &imu_data,
                       sizeof(imu_data), request->timestamp_ns());

    response->set_note("RawImuData received");
    return Status::OK;
  }

  // 接收 LatencyData
  Status SendLatencyData(ServerContext *context, const LatencyData *request,
                         Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] LatencyData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Type:" << request->type() << std::endl;

    // 构造 RawLatencyDataDumpStruct 并保存
    RawLatencyDataDumpStruct latency_data;
    latency_data.timestamp_ns = request->timestamp_ns();
    latency_data.type = request->type();
    latency_data.data[0] = request->data_1();
    latency_data.data[1] = request->data_2();
    latency_data.data[2] = request->data_3();
    latency_data.data[3] = request->data_4();
    latency_data.data[4] = request->data_5();
    latency_data.data[5] = request->data_6();

    std::cout << "  onsensor_timestamp_us: " << header.onsensor_timestamp_us()
              << std::endl;
    std::cout << "  timestamp_ns: " << latency_data.timestamp_ns << std::endl;
    std::cout << "  type: " << latency_data.type << std::endl;
    std::cout << "  data: [";
    for (int i = 0; i < 6; ++i) {
      std::cout << latency_data.data[i];
      if (i < 5)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    saveStructuredData(header.group_id(), header.msg_id(), &latency_data,
                       sizeof(latency_data), request->timestamp_ns());

    response->set_note("LatencyData received");
    return Status::OK;
  }

  // 接收 BinaryData
  Status SendBinaryData(ServerContext *context, const BinaryData *request,
                        Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] BinaryData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Size:" << request->data_len() << " bytes" << std::endl;

    // 保存二进制数据（使用默认格式）
    std::string filename = getFilename(header.group_id(), header.msg_id());
    auto ofs = getOrCreateFile(filename, header.group_id(), header.msg_id());

    if (ofs && ofs->is_open()) {
      // 写入时间戳和数据长度
      std::string line = std::to_string(0ULL) + "," +
                         std::to_string(request->data_len()) + "," +
                         "binary_data\n";

      addToBuffer(filename, line);
    }

    response->set_note("BinaryData received");
    return Status::OK;
  }

  // 接收 ImageData
  Status SendImageData(ServerContext *context, const ImageData *request,
                       Ack *response) override {
    received_count_++;

    const auto &header = request->header();
    std::cout << "[" << received_count_ << "] ImageData - "
              << "Group:" << header.group_id() << " Msg:" << header.msg_id()
              << " Size:" << request->image_binary_data().size() << " bytes"
              << std::endl;

    // 保存图片数据到单独的文件
    std::string img_filename = "image_" + std::to_string(header.group_id()) + "_" + 
                               std::to_string(header.msg_id()) + ".bin";
    std::string img_path = save_dir_ + "/" + img_filename;
    std::ofstream img_ofs(img_path, std::ios::binary);
    if (img_ofs.is_open()) {
      img_ofs.write(request->image_binary_data().data(),
                    request->image_binary_data().size());
      img_ofs.close();
      std::cout << "  Saved image to: " << img_path << std::endl;
    }

    // 同时记录到 CSV
    std::string filename = getFilename(header.group_id(), header.msg_id());
    auto ofs = getOrCreateFile(filename, header.group_id(), header.msg_id());

    if (ofs && ofs->is_open()) {
      std::string line =
          std::to_string(0ULL) + "," + img_filename +
          "," + std::to_string(request->image_binary_data().size()) + "\n";

      addToBuffer(filename, line);
    }

    response->set_note("ImageData received");
    return Status::OK;
  }

  uint64_t GetReceivedCount() const { return received_count_; }

private:
  // 使用数据结构管理器保存结构化数据
  void saveStructuredData(uint64_t group_id, uint64_t msg_id, const void *data,
                          size_t data_size, uint64_t timestamp) {
    std::string filename = getFilename(group_id, msg_id);
    auto ofs = getOrCreateFile(filename, group_id, msg_id);

    if (!ofs || !ofs->is_open()) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return;
    }

    // 使用 DataStructureManager 解析数据
    std::string csv_line =
        structure_manager_.parseDataToCsv(group_id, msg_id, data, data_size);

    if (!csv_line.empty()) {
      // 在CSV行前添加timestamp，与 dump_nviz_data_to_file 格式一致
      csv_line = std::to_string(timestamp) + "," + csv_line;
      addToBuffer(filename, csv_line + "\n");
      std::cout << "  Saved to: " << filename << std::endl;
    } else {
      std::cerr << "  Warning: parseDataToCsv returned empty for group_id="
                << group_id << ", msg_id=" << msg_id
                << ", data_size=" << data_size << std::endl;
    }
  }

  // 获取文件名
  std::string getFilename(uint64_t group_id, uint64_t msg_id) {
    std::string filename = save_dir_ + "/group_" + std::to_string(group_id) +
                           "_msg_" + std::to_string(msg_id) + ".csv";
    return filename;
  }

  // 获取或创建文件流
  std::shared_ptr<std::ofstream> getOrCreateFile(const std::string &filename,
                                                 uint64_t group_id,
                                                 uint64_t msg_id) {
    auto it = filename_to_ofs_.find(filename);
    if (it != filename_to_ofs_.end()) {
      return it->second;
    }

    // 创建新文件
    auto ofs = std::make_shared<std::ofstream>(filename, std::ios::app);
    if (!ofs->is_open()) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return nullptr;
    }

    // 如果是新文件，写入 CSV 头部
    ofs->seekp(0, std::ios::end);
    if (ofs->tellp() == 0) {
      // 使用 DataStructureManager 生成 CSV 头
      std::string header =
          structure_manager_.generateCsvHeader(group_id, msg_id);
      if (!header.empty()) {
        // 添加 timestamp 列，与 dump_nviz_data_to_file 格式一致
        *ofs << "timestamp, " << header << "\n";
        ofs->flush();
      }
    }

    filename_to_ofs_[filename] = ofs;
    filename_to_buffer_[filename] = "";
    filename_to_count_[filename] = 0;
    filename_to_last_flush_[filename] = std::chrono::steady_clock::now();

    return ofs;
  }

  // 添加到缓冲区
  void addToBuffer(const std::string &filename, const std::string &data) {
    filename_to_buffer_[filename] += data;
    filename_to_count_[filename]++;

    auto now = std::chrono::steady_clock::now();
    auto time_since_flush =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - filename_to_last_flush_[filename]);

    // 检查是否需要刷新
    if (filename_to_buffer_[filename].size() >= BUFFER_SIZE_LIMIT ||
        filename_to_count_[filename] >= BUFFER_COUNT_LIMIT ||
        time_since_flush >= FLUSH_INTERVAL) {
      flushBuffer(filename);
    }
  }

  // 刷新缓冲区到文件
  void flushBuffer(const std::string &filename) {
    auto it_buf = filename_to_buffer_.find(filename);
    auto it_ofs = filename_to_ofs_.find(filename);

    if (it_buf != filename_to_buffer_.end() &&
        it_ofs != filename_to_ofs_.end() && !it_buf->second.empty()) {

      auto &ofs = it_ofs->second;
      if (ofs && ofs->is_open()) {
        *ofs << it_buf->second;
        ofs->flush();
      }

      it_buf->second.clear();
      filename_to_count_[filename] = 0;
      filename_to_last_flush_[filename] = std::chrono::steady_clock::now();
    }
  }

private:
  std::string save_dir_;
  uint64_t received_count_;
  DataStructureManager structure_manager_;

  std::map<std::string, std::shared_ptr<std::ofstream>> filename_to_ofs_;
  std::map<std::string, std::string> filename_to_buffer_;
  std::map<std::string, size_t> filename_to_count_;
  std::map<std::string, std::chrono::steady_clock::time_point>
      filename_to_last_flush_;

  static const size_t BUFFER_SIZE_LIMIT = 8192;                  // 8KB
  static const size_t BUFFER_COUNT_LIMIT = 100;                  // 100条记录
  static constexpr std::chrono::milliseconds FLUSH_INTERVAL{50}; // 50ms
};

void RunServer(const std::string &server_address, const std::string &save_dir) {
  AlgServiceImpl service(save_dir);

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
  std::string save_dir = "./grpc_received_data";

  // 解析命令行参数
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                << "Options:\n"
                << "  -a, --address <addr>  Server address (default: "
                   "127.0.0.1:50051)\n"
                << "  -d, --dir <path>      Save directory (default: "
                   "./grpc_received_data)\n"
                << "  -h, --help            Show this help message\n"
                << "\nData will be saved in CSV format similar to "
                   "dump_nviz_data_to_file\n"
                << "Files will be named: group_<id>_msg_<id>.csv\n"
                << std::endl;
      return 0;
    } else if ((arg == "-a" || arg == "--address") && i + 1 < argc) {
      server_address = argv[++i];
    } else if ((arg == "-d" || arg == "--dir") && i + 1 < argc) {
      save_dir = argv[++i];
    }
  }

  try {
    RunServer(server_address, save_dir);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
