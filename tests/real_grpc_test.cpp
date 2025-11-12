#include "../toolkit/datadump/protobuf_grpc/send_nviz_data_by_protobuf_grpc.h"
#include <chrono>
#include <iostream>
#include <thread>

using namespace xreal::toolkits::datadump;

int main() {
    std::cout << "=== Protobuf + gRPC Send Test (Using Your Code) ===" << std::endl;

    try {
        std::string target = "localhost:50051";
        std::cout << "make_stub for " << target << std::endl;
        auto stub = make_stub(target);

        for (int i = 0; i < 1000000000; ++i) {

            RawImuDataDumpStruct gyro_values;

            float x = i * 0.5;
            float y = i * 1.0;
            float z = i * 1.5;

            gyro_values.onsensor_timestamp_us = get_current_timestamp_us();
            gyro_values.timestamp_ns = i * 1000000; // 模拟纳秒时间戳
            gyro_values.type =
                DumpSensorType::DUMP_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED; // 示例类型
            gyro_values.data[0] = x;
            gyro_values.data[1] = y;
            gyro_values.data[2] = z;
            gyro_values.data[3] = 0;
            gyro_values.data[4] = 0;
            gyro_values.data[5] = 0;

            auto ack1 = send_once(*stub, gyro_values);
            
            // RawImuDataDumpStruct accel_values;
            // accel_values.onsensor_timestamp_us = get_current_timestamp_us();
            // accel_values.timestamp_ns = i * 1000000; // 模拟纳秒时间戳
            // accel_values.type = DumpSensorType::
            //     DUMP_SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED; // 示例类型
            // accel_values.data[0] = x * 2;
            // accel_values.data[1] = y * 2;
            // accel_values.data[2] = z * 2;
            // accel_values.data[3] = 0;
            // accel_values.data[4] = 0;
            // accel_values.data[5] = 0;
            
            // std::vector<RawImuDataDumpStruct> blocks;
            // blocks.push_back(gyro_values);
            // blocks.push_back(accel_values);
            // auto ack2 = upload_stream(*stub, blocks);

            // 间隔一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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