#include <chrono>
#include <dlfcn.h>
#include <fstream>
#include <iomanip> // std::setw, std::setfill
#include <iostream>
#include <libgen.h> // dirname
#include <limits.h> // PATH_MAX
#include <thread>
#include <unistd.h> // realpath

#include <framework/util/dlutil.h>

typedef void (*GRPCMakeStubFun)(char *target_name, size_t target_size);
typedef void (*SendGRPCRawImuDataFun)(uint64_t onsensor_timestamp_us,
                                      uint64_t timestamp_ns, uint32_t type,
                                      float data_1, float data_2, float data_3,
                                      float data_4, float data_5, float data_6);
typedef void (*SendGRPCDataFun)(int group_id, int msg_id, const uint8_t *data,
                                uint64_t len);
typedef void (*SendGRPCBinaryDataFun)(uint64_t onsensor_timestamp_us,
                                      uint64_t timestamp_ns, uint32_t data_len,
                                      uint8_t *data);
typedef void (*SendGRPCImageDataFun)(uint64_t onsensor_timestamp_us,
                                     uint64_t timestamp_ns, char *file_name,
                                     size_t file_name_size, char *buf_name,
                                     size_t buf_size);

int main(int argc, char **argv) {
  // 1. 加载动态库,括号里面的路径根据实际情况调整
  void *handle = framework::util::LibraryOpen("libsend_grpc.so");
  if (!handle) {
    std::cout << "Failed to load dynamic library: " << dlerror() << std::endl;
    return 1;
  }

  auto GRPCMakeStub =
      (GRPCMakeStubFun)framework::util::LibraryGetProcAddr(handle, "GRPCMakeStub");
  auto SendGRPCRawImuData =
      (SendGRPCRawImuDataFun)framework::util::LibraryGetProcAddr(
          handle, "SendGRPCRawImuData");
  auto SendGRPCBinaryData =
      (SendGRPCBinaryDataFun)framework::util::LibraryGetProcAddr(
          handle, "SendGRPCBinaryData");
  auto SendGRPCImageData =
      (SendGRPCImageDataFun)framework::util::LibraryGetProcAddr(
          handle, "SendGRPCImageData");

  if (!GRPCMakeStub || !SendGRPCRawImuData || !SendGRPCBinaryData || !SendGRPCImageData) {
    std::cout << "Failed to load functions: "
              << framework::util::LibraryOpenError("libsend_grpc.so")
              << std::endl;
    framework::util::LibraryClose(handle);
    return 1;
  }
  // 1.加载动态库结束

  std::cout << "=== Protobuf + gRPC Send Test (Using Your Code) ==="
            << std::endl;
  std::string target = "localhost:50051";
  std::cout << "make_stub for " << target << std::endl;
  GRPCMakeStub(const_cast<char*>(target.data()), target.size());

  // 这里i受限于images目录中的pgm图片数量
  for (int i = 0; i <= 20; ++i) {
    SendGRPCRawImuData(i, i, 1, i * 0.5, i * 1.0, i * 1.5, 0, 0, 0);

    size_t len = 1024 * 100;
    uint8_t buf[1024 * 100];
    for (size_t j = 0; j < len; j++)
      buf[j] = static_cast<uint8_t>(j % 256);
    SendGRPCBinaryData(i, i, len, buf);

    std::ostringstream oss;
    oss << "m" << std::setw(7) << std::setfill('0') << i << ".pgm";
    std::string filename = oss.str();
    char abs_src[PATH_MAX];
    if (!realpath(__FILE__, abs_src))
      throw std::runtime_error("realpath(__FILE__) failed");
    char dirbuf[PATH_MAX];
    std::snprintf(dirbuf, sizeof(dirbuf), "%s", abs_src);
    char *src_dir = dirname(dirbuf);
    std::string full_path = std::string(src_dir) + "/images/" + filename;
    std::ifstream f(full_path, std::ios::binary | std::ios::ate);
    if (!f)
      throw std::runtime_error("open failed");
    std::streamsize n = f.tellg();
    f.seekg(0);
    std::string buffer(n, '\0');
    f.read(&buffer[0], n);
    SendGRPCImageData(i, i, const_cast<char*>(filename.data()), filename.size(), 
                      const_cast<char*>(buffer.data()), buffer.size());

    // 间隔一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  std::cout << "Protobuf + gRPC send test completed!" << std::endl;
}