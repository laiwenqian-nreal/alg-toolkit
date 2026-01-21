#include <chrono>
#include <dlfcn.h>
#include <fstream>
#include <iomanip> // std::setw, std::setfill
#include <iostream>
#include <libgen.h> // dirname
#include <limits.h> // PATH_MAX
#include <string>
#include <thread>
#include <vector>
#include <unistd.h> // realpath

#include <framework/util/dlutil.h>
typedef void (*GRPCMakeStubFun)(char *target_name, size_t target_size);
typedef void (*SendGRPCRawImuDataFun)(uint64_t onsensor_timestamp_us,
                                      uint64_t timestamp_ns, uint32_t type,
                                      float data_1, float data_2, float data_3,
                                      float data_4, float data_5, float data_6);
typedef void (*SendGRPCBinaryDataFun)(uint64_t onsensor_timestamp_us,
                                      uint64_t timestamp_ns, uint32_t data_len,
                                      uint8_t *data);
typedef void (*SendGRPCImageDataFun)(uint64_t onsensor_timestamp_us,
                                     uint64_t timestamp_ns, char *file_name,
                                     size_t file_name_size, char *buf_name,
                                     size_t buf_size);
typedef void (*SendGRPCLatencyDataFun)(uint64_t onsensor_timestamp_us,
                                      uint64_t timestamp_ns, uint32_t type,
                                      uint64_t data_1, uint64_t data_2, uint64_t data_3,
                                      uint64_t data_4, uint64_t data_5, uint64_t data_6);

uint64_t get_current_timestamp_ns() {
    struct timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC, &cur_time);
    return (uint64_t)cur_time.tv_sec * 1000000000 + (uint64_t)cur_time.tv_nsec;
}

// 生成简单的PNG图像数据（假数据）
std::vector<uint8_t> generate_fake_png(int width = 64, int height = 64) {
    // PNG文件头
    const uint8_t png_header[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    
    // 简单的IHDR块（最小有效PNG结构）
    const uint8_t ihdr_chunk[] = {
        0x00, 0x00, 0x00, 0x0D, // 块长度
        0x49, 0x48, 0x44, 0x52, // 块类型 "IHDR"
        static_cast<uint8_t>((width >> 24) & 0xFF), static_cast<uint8_t>((width >> 16) & 0xFF), static_cast<uint8_t>((width >> 8) & 0xFF), static_cast<uint8_t>(width & 0xFF), // 宽度
        static_cast<uint8_t>((height >> 24) & 0xFF), static_cast<uint8_t>((height >> 16) & 0xFF), static_cast<uint8_t>((height >> 8) & 0xFF), static_cast<uint8_t>(height & 0xFF), // 高度
        0x08, // 位深度
        0x02, // 颜色类型（RGB）
        0x00, // 压缩方法
        0x00, // 过滤方法
        0x00, // 隔行扫描
        0x00, 0x00, 0x00, 0x00  // CRC（简化为0，实际应用中需要计算）
    };
    
    // 简单的IDAT块（包含渐变图像数据）
    std::vector<uint8_t> idat_data;
    // 块长度（占位符）
    idat_data.push_back(0x00); idat_data.push_back(0x00); idat_data.push_back(0x00); idat_data.push_back(0x00);
    // 块类型 "IDAT"
    idat_data.push_back(0x49); idat_data.push_back(0x44); idat_data.push_back(0x41); idat_data.push_back(0x54);
    
    // 生成简单的RGB渐变图像数据（宽度*高度*3字节）
    for (int y = 0; y < height; ++y) {
        idat_data.push_back(0x00); // 过滤类型
        for (int x = 0; x < width; ++x) {
            // 简单的颜色渐变：红色随x变化，绿色随y变化，蓝色固定
            uint8_t r = static_cast<uint8_t>((x * 255) / width);
            uint8_t g = static_cast<uint8_t>((y * 255) / height);
            uint8_t b = 128;
            idat_data.push_back(r);
            idat_data.push_back(g);
            idat_data.push_back(b);
        }
    }
    // CRC（占位符）
    idat_data.push_back(0x00); idat_data.push_back(0x00); idat_data.push_back(0x00); idat_data.push_back(0x00);
    
    // 更新IDAT块长度
    uint32_t idat_len = idat_data.size() - 8; // 减去长度和类型字段
    idat_data[0] = (idat_len >> 24) & 0xFF;
    idat_data[1] = (idat_len >> 16) & 0xFF;
    idat_data[2] = (idat_len >> 8) & 0xFF;
    idat_data[3] = idat_len & 0xFF;
    
    // IEND块
    const uint8_t iend_chunk[] = {
        0x00, 0x00, 0x00, 0x00, // 块长度
        0x49, 0x45, 0x4E, 0x44, // 块类型 "IEND"
        0xAE, 0x42, 0x60, 0x82  // CRC
    };
    
    // 组装完整的PNG数据
    std::vector<uint8_t> png_data;
    png_data.insert(png_data.end(), std::begin(png_header), std::end(png_header));
    png_data.insert(png_data.end(), std::begin(ihdr_chunk), std::end(ihdr_chunk));
    png_data.insert(png_data.end(), idat_data.begin(), idat_data.end());
    png_data.insert(png_data.end(), std::begin(iend_chunk), std::end(iend_chunk));
    
    return png_data;
}

int main(int argc, char **argv) {
  // 1. 加载动态库,括号里面的路径根据实际情况调整

  void *handle = framework::util::LibraryOpen("../toolkit/interface/libsend_grpc.so");
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
  auto SendGRPCLatencyData =
      (SendGRPCLatencyDataFun)framework::util::LibraryGetProcAddr(
          handle, "SendGRPCLatencyData");

  if (!GRPCMakeStub || !SendGRPCRawImuData || !SendGRPCBinaryData || !SendGRPCImageData || !SendGRPCLatencyData) {
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
  for (int i = 0; i <= 200000000; ++i) {
    // 发送IMU数据
    uint64_t timestamp_ns = get_current_timestamp_ns();
    SendGRPCRawImuData(i, timestamp_ns, 1, i * 0.5, i * 1.0, i * 1.5, i * 2.0, i * 2.5, i * 3.0);

    // 发送二进制数据
    // size_t len = 1024 * 100;
    // uint8_t buf[1024 * 100];
    // for (size_t j = 0; j < len; j++)
    //   buf[j] = static_cast<uint8_t>(j % 256);
    // SendGRPCBinaryData(i, i, len, buf);

    // 发送图像数据（使用生成的假PNG数据）
    // std::vector<uint8_t> fake_png_data = generate_fake_png(128, 128);
    // std::string filename = "fake_image_" + std::to_string(i) + ".png";
    // SendGRPCImageData(i, i, const_cast<char*>(filename.data()), filename.size(), 
    //                   reinterpret_cast<char*>(fake_png_data.data()), fake_png_data.size());
    
    // 发送延迟数据
    // if (SendGRPCLatencyData) {
    //     // 生成一些合理的延迟数据（纳秒）
    //     uint64_t delay_1 = 1000000 + i * 10000;  // 1ms + i*10us
    //     uint64_t delay_2 = 2000000 + i * 20000;  // 2ms + i*20us
    //     uint64_t delay_3 = 3000000 + i * 30000;  // 3ms + i*30us
    //     uint64_t delay_4 = 4000000 + i * 40000;  // 4ms + i*40us
    //     uint64_t delay_5 = 5000000 + i * 50000;  // 5ms + i*50us
    //     uint64_t delay_6 = 6000000 + i * 60000;  // 6ms + i*60us
    //     SendGRPCLatencyData(i, i, 1, delay_1, delay_2, delay_3, delay_4, delay_5, delay_6);
    // }

    // 间隔一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  std::cout << "Protobuf + gRPC send test completed!" << std::endl;
}