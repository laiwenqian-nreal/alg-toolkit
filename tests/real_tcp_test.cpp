#include "../toolkit/datadump/send_nviz_data_by_tcp.h"
#include <chrono>
#include <iostream>
#include <thread>

using namespace xreal::toolkits::datadump;

int main() {
  std::cout << "=== TCP Send Test (Using Your Code) ===" << std::endl;

  try {
    std::cout << "Using static interface to send TCP data..." << std::endl;

    // 创建测试数据并通过静态接口发送
    std::cout << "Creating and sending test data..." << std::endl;

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

      RawImuDataDumpStruct accel_values;
      accel_values.onsensor_timestamp_us = get_current_timestamp_us();
      accel_values.timestamp_ns = i * 1000000; // 模拟纳秒时间戳
      accel_values.type = DumpSensorType::
          DUMP_SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED; // 示例类型
      accel_values.data[0] = x * 2;
      accel_values.data[1] = y * 2;
      accel_values.data[2] = z * 2;
      accel_values.data[3] = 0;
      accel_values.data[4] = 0;
      accel_values.data[5] = 0;

      // 使用静态接口发送数据
      XrealLinkTcp::linkSendStatus(
          127, // group_id
          1,   // msg_id (使用5个不同的msg_id)
          reinterpret_cast<const uint8_t *>(&gyro_values), sizeof(gyro_values));

      XrealLinkTcp::linkSendStatus(
          127, // group_id
          1,   // msg_id (使用5个不同的msg_id)
          reinterpret_cast<const uint8_t *>(&accel_values),
          sizeof(accel_values));

      // 间隔一段时间
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "All data sent to TCP queue" << std::endl;
    std::cout << "Waiting for transmission to complete..." << std::endl;

    // 等待发送完成
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Closing TCP connection..." << std::endl;
    XrealLinkTcp::Close();

    std::cout << "TCP send test completed!" << std::endl;
    std::cout << "Note: Start a TCP server on port 8099 to receive the data"
              << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -1;
  } catch (...) {
    std::cerr << "Unknown exception occurred" << std::endl;
    return -1;
  }

  return 0;
}
