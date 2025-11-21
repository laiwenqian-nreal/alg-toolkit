#include "../../toolkit/datadump/send_nviz_data_by_grpc.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <libgen.h>   // dirname
#include <unistd.h>   // realpath
#include <limits.h>   // PATH_MAX
#include <iomanip>   // std::setw, std::setfill
#include <thread>

using namespace xreal::toolkits::datadump;

int main() {
    std::cout << "=== Protobuf + gRPC Send Test (Using Your Code) ===" << std::endl;

    try {
        std::string target = "localhost:50051";
        std::cout << "make_stub for " << target << std::endl;
        auto stub = XrealLinkgRPC::make_stub(target);

        // 这里i受限于images目录中的pgm图片数量
        for (int i = 0; i <= 20; ++i) {

            auto ack1 = XrealLinkgRPC::send_RawImuData(*stub, get_current_timestamp_us(), i,
                                        DumpSensorType::DUMP_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
                                        i * 0.5, i * 1.0, i * 1.5, 0, 0, 0);
            
            size_t len = 1024 * 100;
            uint8_t buf[1024 * 100];
            for (size_t j = 0; j < len; j++) buf[j] = static_cast<uint8_t>(j % 256);

            auto ack2 = XrealLinkgRPC::send_BinaryData(*stub, get_current_timestamp_us(), i, len, buf);

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

            auto ack3 = XrealLinkgRPC::send_ImageData(*stub, get_current_timestamp_us(), i, filename, buffer);

            // 间隔一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        std::cout << "Protobuf + gRPC send test completed!" << std::endl;
        
    } catch (const std::exception &e) {
        std::cerr << "Exception:  " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown  exception occurred" << std::endl;
        return -1;
    }
}