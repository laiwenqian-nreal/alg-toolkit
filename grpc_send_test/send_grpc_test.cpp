#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <libgen.h>   // dirname
#include <unistd.h>   // realpath
#include <limits.h>   // PATH_MAX
#include <iomanip>   // std::setw, std::setfill
#include <chrono>
#include <thread>

typedef void (*MakeStubFun)(char* target_name, size_t target_size);
typedef void (*SendRawImuDataFun)(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        uint32_t type, float data_1, float data_2, float data_3,
                        float data_4, float data_5, float data_6);
typedef void (*SendBinaryDataFun)(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        uint32_t data_len, uint8_t* data);
typedef void (*SendImageDataFun)(uint64_t onsensor_timestamp_us, uint64_t timestamp_ns,
                        char* file_name, size_t file_name_size,
                        char* buf_name, size_t buf_size);

int main(int argc, char **argv) {
    // 1. 加载动态库,括号里面的路径根据实际情况调整
    void *handle = dlopen("libsend_grpc.so", RTLD_NOW);
    if (!handle) {
        std::cout << "Failed to load dynamic library: " << dlerror() << std::endl;
        return 1;
    }

    auto MakeStub = (MakeStubFun)dlsym(handle, "MakeStub");
    auto SendRawImuData = (SendRawImuDataFun)dlsym(handle, "SendRawImuData");
    auto SendBinaryData = (SendBinaryDataFun)dlsym(handle, "SendBinaryData");
    auto SendImageData = (SendImageDataFun)dlsym(handle, "SendImageData");

    if (!MakeStub || !SendRawImuData || !SendBinaryData || !SendImageData) {
        std::cout << "Failed to load functions: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }
    // 1.加载动态库结束

    std::cout << "=== Protobuf + gRPC Send Test (Using Your Code) ===" << std::endl;
    std::string target = "localhost:50051";
    std::cout << "make_stub for " << target << std::endl;
    MakeStub(target.data(), target.size());
    
    // 这里i受限于images目录中的pgm图片数量
    for (int i = 0; i <= 20; ++i) {
        SendRawImuData(i, i, 1, i*0.5, i*1.0, i*1.5, 0, 0, 0);

        size_t len = 1024 * 100;
        uint8_t buf[1024 * 100];
        for (size_t j = 0; j < len; j++) buf[j] = static_cast<uint8_t>(j % 256);
        SendBinaryData(i, i, len, buf);

        std::ostringstream oss;
        oss << "m" << std::setw(7) << std::setfill('0') << i << ".pgm";
        std::string filename = oss.str();
        char abs_src[PATH_MAX];
        if (!realpath(__FILE__, abs_src)) throw std::runtime_error("realpath(__FILE__) failed");
        char dirbuf[PATH_MAX];
        std::snprintf(dirbuf, sizeof(dirbuf), "%s", abs_src);
        char* src_dir = dirname(dirbuf);
        std::string full_path = std::string(src_dir) + "/images/" + filename;
        std::ifstream f(full_path, std::ios::binary | std::ios::ate);
        if (!f) throw std::runtime_error("open failed");
        std::streamsize n = f.tellg();
        f.seekg(0);
        std::string buffer(n, '\0');
        f.read(buffer.data(), n);
        SendImageData(i, i, filename.data(), filename.size(), buffer.data(), buffer.size());

        // 间隔一段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "Protobuf + gRPC send test completed!" << std::endl;
}