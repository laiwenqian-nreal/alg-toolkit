#ifndef DATA_STRUCTURE_MANAGER_H
#define DATA_STRUCTURE_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace xreal {
namespace toolkits {
namespace datadump {

enum DumpSensorType {
  DUMP_SENSOR_TYPE_INVALID = 0,
  DUMP_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED = 1,
  DUMP_SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED = 2,
  DUMP_SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED = 3,
  DUMP_SENSOR_TYPE_GYROSCOPE = 4,
  DUMP_SENSOR_TYPE_ACCELEROMETER = 5,
  DUMP_SENSOR_TYPE_MAGNETIC_FIELD = 6,
  DUMP_SENSOR_TYPE_ROTATION_VECTOR = 7,
  DUMP_SENSOR_TYPE_GAME_ROTATION_VECTOR = 8,
  DUMP_SENSOR_TYPE_GRAVITY = 9,
  DUMP_SENSOR_TYPE_LINEAR_ACCELERATION = 10,
  DUMP_SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR = 11,
  DUMP_SENSOR_TYPE_IMU_TEMPERATURE = 12,
  DUMP_SENSOR_TYPE_GYROSCOPE_CALIBRATION_DATA = 13,
  DUMP_SENSOR_TYPE_ACCELEROMETER_CALIBRATION_DATA = 14,
  DUMP_SENSOR_TYPE_MAGNETIC_CALIBRATION_DATA = 15,
  DUMP_SENSOR_TYPE_GNSS = 16,
  DUMP_SENSOR_TYPE_SIGNIFICANT_MOTION = 17,
  DUMP_SENSOR_TYPE_STEP_COUNTER = 18,
  DUMP_SENSOR_TYPE_STATIONARY_DETECT = 19,
  DUMP_SENSOR_TYPE_AIRPLANE_MODE = 20,
  DUMP_SENSOR_TYPE_ANGULAR_ACCELERATION = 21,
  DUMP_SENSOR_TYPE_ANGULAR_VELOCITY = 22,
  DUMP_SENSOR_TYPE_LINEAR_VELOCITY = 23,
  DUMP_SENSOR_TYPE_NREAL_LINK_COUNT =
      24, // data0 收集队列中的待处理数量， data1 发送队列中的待处理数量
  DUMP_SENSOR_TYPE_IMU_LATENCY = 25,
  DUMP_SENSOR_TYPE_MAG_LATENCY = 26,
  DUMP_SENSOR_TYPE_IMU_FRAME_ID = 27,
  DUMP_SENSOR_TYPE_MAG_FRAME_ID = 28,

  DUMP_SENSOR_TYPE_RESULT_P = 29,
  DUMP_SENSOR_TYPE_RESULT_Q = 30,
  DUMP_SENSOR_TYPE_RESULT_GYRO_BIAS = 31,
  DUMP_SENSOR_TYPE_UNDEFINE,
};

// 原始 IMU 数据结构
struct RawImuDataDumpStruct {
  uint64_t onsensor_timestamp_us;
  uint64_t timestamp_ns;
  uint32_t type;
  float data[6];
};

// 数据字段定义
struct DataField {
  std::string type;    // u64, u32, f32 等
  std::string name;    // 字段名
  std::string unit;    // 单位（可选）
  int array_size = 1;  // 数组大小，默认为1
  bool ignore = false; // 是否忽略显示

  // 获取字段大小
  size_t getSize() const;

  // 转换为字符串
  std::string toString(const void *data, size_t offset) const;
};

// 数据结构定义
struct DataStructure {
  std::string name;
  std::vector<DataField> fields;
  size_t total_size = 0;

  // 计算总大小
  void calculateSize();

  // 获取name
  std::string getName() const { return name; }
};

// 数据结构管理器
class DataStructureManager {
private:
  std::map<std::string, std::shared_ptr<DataStructure>> structures_;
  std::map<uint64_t, std::string> group_id_to_field_definitions_;

  // 简化的JSON解析
  std::string parseJsonString(const std::string &json, const std::string &key);
  int parseJsonInt(const std::string &json, const std::string &key);
  std::vector<std::string> parseJsonArray(const std::string &json,
                                          const std::string &key);

public:
  DataStructureManager(bool ignore_onsensor_timestamp = false);

  DataStructureManager(
      const std::string &data_struct,
      const std::map<uint64_t, std::string> &group_msg_id_to_field_definitions,
      bool ignore_onsensor_timestamp = false);

  // 从JSON字符串加载数据结构
  bool loadFromJsonString(const std::string &json_config);

  // 从JSON文件加载数据结构
  bool loadFromJsonFile(const std::string &filename);

  // 根据group_id和msg_id获取数据结构
  std::shared_ptr<DataStructure> getStructure(uint32_t group_id,
                                              uint32_t msg_id);

  // 解析数据并生成CSV行
  std::string parseDataToCsv(uint32_t group_id, uint32_t msg_id,
                             const void *data, size_t data_size);

  // 生成CSV头
  std::string generateCsvHeader(uint32_t group_id, uint32_t msg_id);
};

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif // DATA_STRUCTURE_MANAGER_H