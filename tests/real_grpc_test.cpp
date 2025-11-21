#include "../../toolkit/datadump/send_nviz_data_by_grpc.h"
#include "../../toolkit/datadump/data_structure_manager.h"
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
    std::cout << "=== gRPC Send Test (Using New Interface) ===" << std::endl;

    try {
        std::string target = "localhost:50051";
        std::cout << "Connecting to gRPC server: " << target << std::endl;
        
        // 使用新接口初始化 gRPC 实例
        XrealLinkgRPC::getInstance(target);

        // 发送测试数据
        for (int i = 0; i <= 20; ++i) {
            // 准备 RawImuData
            RawImuDataDumpStruct imu_data;
            imu_data.onsensor_timestamp_us = get_current_timestamp_us();
            imu_data.timestamp_ns = get_current_timestamp_ns();
            imu_data.type = DumpSensorType::DUMP_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
            imu_data.data[0] = i * 0.5f;
            imu_data.data[1] = i * 1.0f;
            imu_data.data[2] = i * 1.5f;
            imu_data.data[3] = 0.0f;
            imu_data.data[4] = 0.0f;
            imu_data.data[5] = 0.0f;

            // 使用 linkSendStatus 发送 IMU 数据 (msg_id = 1)
            XrealLinkgRPC::linkSendStatus(
                1, // group_id
                1, // msg_id = 1 表示 IMU 数据
                reinterpret_cast<const uint8_t*>(&imu_data),
                sizeof(imu_data)
            );

            // 准备二进制数据
            size_t len = 1024 * 100;
            std::vector<uint8_t> binary_data(len);
            for (size_t j = 0; j < len; j++) {
                binary_data[j] = static_cast<uint8_t>(j % 256);
            }

            // 使用 linkSendStatus 发送二进制数据 (msg_id = 9999)
            XrealLinkgRPC::linkSendStatus(
                2, // group_id
                9999, // msg_id = 9999 表示通用二进制数据
                binary_data.data(),
                binary_data.size()
            );

            std::cout << "Sent message " << i << std::endl;

            // 间隔一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "gRPC send test completed!" << std::endl;
        
        // 关闭连接
        XrealLinkgRPC::Close();
        
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return -1;
    }
    
    return 0;
}