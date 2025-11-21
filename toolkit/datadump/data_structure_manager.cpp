#include "data_structure_manager.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace xreal {
namespace toolkits {
namespace datadump {

// DataField implementation
size_t DataField::getSize() const {
  size_t base_size = 0;
  if (type == "u64" || type == "i64" || type == "f64") {
    base_size = 8;
  } else if (type == "u32" || type == "i32" || type == "f32") {
    base_size = 4;
  } else if (type == "u16" || type == "i16") {
    base_size = 2;
  } else if (type == "u8" || type == "i8") {
    base_size = 1;
  }
  return base_size * array_size;
}

std::string DataField::toString(const void *data, size_t offset) const {
  const uint8_t *ptr = static_cast<const uint8_t *>(data) + offset;

  if (type == "u64") {
    return std::to_string(*reinterpret_cast<const uint64_t *>(ptr));
  } else if (type == "u32") {
    return std::to_string(*reinterpret_cast<const uint32_t *>(ptr));
  } else if (type == "f32") {
    return std::to_string(*reinterpret_cast<const float *>(ptr));
  } else if (type == "f64") {
    return std::to_string(*reinterpret_cast<const double *>(ptr));
  } else if (type == "i64") {
    return std::to_string(*reinterpret_cast<const int64_t *>(ptr));
  } else if (type == "i32") {
    return std::to_string(*reinterpret_cast<const int32_t *>(ptr));
  } else if (type == "u16") {
    return std::to_string(*reinterpret_cast<const uint16_t *>(ptr));
  } else if (type == "i16") {
    return std::to_string(*reinterpret_cast<const int16_t *>(ptr));
  } else if (type == "u8") {
    return std::to_string(*reinterpret_cast<const uint8_t *>(ptr));
  } else if (type == "i8") {
    return std::to_string(*reinterpret_cast<const int8_t *>(ptr));
  }
  return "0";
}

// DataStructure implementation
void DataStructure::calculateSize() {
  total_size = 0;
  for (const auto &field : fields) {
    total_size += field.getSize();
  }
}

// DataStructureManager implementation
DataStructureManager::DataStructureManager() {
  std::shared_ptr<DataStructure> raw_imu_data_struct =
      std::make_shared<DataStructure>();
  raw_imu_data_struct->name = "RawImuDataDumpStruct";
  raw_imu_data_struct->fields = {
      {"u64", "timestamp_ns", "ns", 1, false}, {"u32", "type", "", 1, false},
      {"f32", "data0", "", 1, false},          {"f32", "data1", "", 1, false},
      {"f32", "data2", "", 1, false},          {"f32", "data3", "", 1, false},
      {"f32", "data4", "", 1, false},          {"f32", "data5", "", 1, false}};
  raw_imu_data_struct->calculateSize();
  structures_["RawImuDataDumpStruct"] = raw_imu_data_struct;

  std::shared_ptr<DataStructure> raw_latency_data_struct =
      std::make_shared<DataStructure>();
  raw_latency_data_struct->name = "RawLatencyDataDumpStruct";
  raw_latency_data_struct->fields = {{"u64", "timestamp_ns", "ns", 1, false},
                                     {"u32", "type", "", 1, false},
                                     {"u64", "data0", "ns", 1, false},
                                     {"u64", "data1", "ns", 1, false},
                                     {"u64", "data2", "ns", 1, false},
                                     {"u64", "data3", "ns", 1, false},
                                     {"u64", "data4", "ns", 1, false},
                                     {"u64", "data5", "ns", 1, false}};
  raw_latency_data_struct->calculateSize();
  structures_["RawLatencyDataDumpStruct"] = raw_latency_data_struct;

  // 初始化 msg_id 到数据结构的映射
  msg_id_to_field_definitions_[DUMP_MESSAGE_ID_RAW_IMU_DATA] =
      "RawImuDataDumpStruct";
  msg_id_to_field_definitions_[DUMP_MESSAGE_ID_LATENCY_DATA] =
      "RawLatencyDataDumpStruct";
}

DataStructureManager::DataStructureManager(
    const std::string &data_struct,
    const std::map<uint32_t, std::string> &msg_id_to_field_definitions) {

  std::shared_ptr<DataStructure> raw_imu_data_struct =
      std::make_shared<DataStructure>();
  raw_imu_data_struct->name = "RawImuDataDumpStruct";
  raw_imu_data_struct->fields = {{"u64", " timestamp_ns", "ns", 1, false},
                                 {"u32", " type", "", 1, false},
                                 {"f32", " data0", "", 1, false},
                                 {"f32", " data1", "", 1, false},
                                 {"f32", " data2", "", 1, false},
                                 {"f32", " data3", "", 1, false},
                                 {"f32", " data4", "", 1, false},
                                 {"f32", " data5", "", 1, false}};
  raw_imu_data_struct->calculateSize();
  structures_["RawImuDataDumpStruct"] = raw_imu_data_struct;

  std::shared_ptr<DataStructure> raw_latency_data_struct =
      std::make_shared<DataStructure>();
  raw_latency_data_struct->name = "RawLatencyDataDumpStruct";
  raw_latency_data_struct->fields = {{"u64", " timestamp_ns", "ns", 1, false},
                                     {"u32", " type", "", 1, false},
                                     {"u64", " data0", "ns", 1, false},
                                     {"u64", " data1", "ns", 1, false},
                                     {"u64", " data2", "ns", 1, false},
                                     {"u64", " data3", "ns", 1, false},
                                     {"u64", " data4", "ns", 1, false},
                                     {"u64", " data5", "ns", 1, false}};
  raw_latency_data_struct->calculateSize();
  structures_["RawLatencyDataDumpStruct"] = raw_latency_data_struct;

  // 初始化默认的 msg_id 到数据结构的映射
  msg_id_to_field_definitions_[DUMP_MESSAGE_ID_RAW_IMU_DATA] =
      "RawImuDataDumpStruct";
  msg_id_to_field_definitions_[DUMP_MESSAGE_ID_LATENCY_DATA] =
      "RawLatencyDataDumpStruct";

  // 解析数据结构定义
  loadFromJsonString(data_struct);
  // 合并传入的映射，传入的映射会覆盖默认映射
  for (const auto &pair : msg_id_to_field_definitions) {
    msg_id_to_field_definitions_[pair.first] = pair.second;
  }
}

std::string DataStructureManager::parseJsonString(const std::string &json,
                                                  const std::string &key) {
  size_t key_pos = json.find("\"" + key + "\"");
  if (key_pos == std::string::npos)
    return "";

  size_t colon_pos = json.find(":", key_pos);
  if (colon_pos == std::string::npos)
    return "";

  size_t value_start = json.find("\"", colon_pos);
  if (value_start == std::string::npos)
    return "";
  value_start++;

  size_t value_end = json.find("\"", value_start);
  if (value_end == std::string::npos)
    return "";

  return json.substr(value_start, value_end - value_start);
}

int DataStructureManager::parseJsonInt(const std::string &json,
                                       const std::string &key) {
  size_t key_pos = json.find("\"" + key + "\"");
  if (key_pos == std::string::npos)
    return -1;

  size_t colon_pos = json.find(":", key_pos);
  if (colon_pos == std::string::npos)
    return -1;

  size_t value_start = colon_pos + 1;
  while (value_start < json.length() &&
         (json[value_start] == ' ' || json[value_start] == '\t')) {
    value_start++;
  }

  size_t value_end = value_start;
  while (value_end < json.length() &&
         (json[value_end] >= '0' && json[value_end] <= '9')) {
    value_end++;
  }

  if (value_end == value_start)
    return -1;

  return std::stoi(json.substr(value_start, value_end - value_start));
}

std::vector<std::string>
DataStructureManager::parseJsonArray(const std::string &json,
                                     const std::string &key) {
  std::vector<std::string> result;

  size_t key_pos = json.find("\"" + key + "\"");
  if (key_pos == std::string::npos)
    return result;

  size_t colon_pos = json.find(":", key_pos);
  if (colon_pos == std::string::npos)
    return result;

  size_t array_start = json.find("[", colon_pos);
  if (array_start == std::string::npos)
    return result;

  size_t array_end = json.find("]", array_start);
  if (array_end == std::string::npos)
    return result;

  std::string array_content =
      json.substr(array_start + 1, array_end - array_start - 1);

  // 简单解析数组元素
  size_t pos = 0;
  while (pos < array_content.length()) {
    size_t quote_start = array_content.find("\"", pos);
    if (quote_start == std::string::npos)
      break;

    size_t quote_end = array_content.find("\"", quote_start + 1);
    if (quote_end == std::string::npos)
      break;

    result.push_back(
        array_content.substr(quote_start + 1, quote_end - quote_start - 1));
    pos = quote_end + 1;
  }

  return result;
}

bool DataStructureManager::loadFromJsonString(const std::string &json_config) {
  try {
    // 解析JSON中的各个数据结构定义
    size_t pos = 0;
    while (pos < json_config.length()) {
      size_t group_start = json_config.find("{", pos);
      if (group_start == std::string::npos)
        break;

      size_t group_end = json_config.find("}", group_start);
      if (group_end == std::string::npos)
        break;

      std::string group_json =
          json_config.substr(group_start, group_end - group_start + 1);

      // 解析单个数据结构
      auto data_struct = std::make_shared<DataStructure>();
      data_struct->name = parseJsonString(group_json, "name");

      // 解析字段列表
      std::vector<std::string> struct_fields =
          parseJsonArray(group_json, "struct");
      for (const auto &field_str : struct_fields) {
        DataField field;
        std::istringstream iss(field_str);
        std::string word;
        std::vector<std::string> words;

        while (iss >> word) {
          words.push_back(word);
        }

        if (words.size() >= 2) {
          field.type = words[0];
          field.name = words[1];

          // 处理数组类型 name[size]
          if (field.name.find("[") != std::string::npos) {
            size_t bracket_start = field.name.find("[");
            size_t bracket_end = field.name.find("]");
            if (bracket_end != std::string::npos) {
              std::string size_str = field.name.substr(
                  bracket_start + 1, bracket_end - bracket_start - 1);
              field.array_size = std::stoi(size_str);
              field.name = field.name.substr(0, bracket_start);
            }
          }

          data_struct->fields.push_back(field);
        }
      }

      data_struct->calculateSize();
      structures_[data_struct->getName()] = data_struct;

      pos = group_end + 1;
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    return false;
  }
}

bool DataStructureManager::loadFromJsonFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  std::string json_content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
  file.close();

  return loadFromJsonString(json_content);
}

std::shared_ptr<DataStructure>
DataStructureManager::getStructure(uint32_t group_id, uint32_t msg_id) {
  // 首先查找 msg_id_to_field_definitions_ 中的映射
  auto msg_it = msg_id_to_field_definitions_.find(msg_id);
  if (msg_it != msg_id_to_field_definitions_.end()) {
    auto struct_it = structures_.find(msg_it->second);
    if (struct_it != structures_.end()) {
      return struct_it->second;
    }
  }

  return nullptr;
}

std::string DataStructureManager::parseDataToCsv(uint32_t group_id,
                                                 uint32_t msg_id,
                                                 const void *data,
                                                 size_t data_size) {
  auto structure = getStructure(group_id, msg_id);
  if (!structure) {
    // 使用通用结构
    structure = structures_["RawImuDataDumpStruct"];
  }

  if (!structure || data_size < structure->total_size) {
    return "";
  }

  std::string csv_line;
  size_t offset = 0;
  bool first_field = true;

  for (size_t i = 0; i < structure->fields.size(); ++i) {
    const auto &field = structure->fields[i];

    // 跳过标记为 ignore 的字段
    if (!field.ignore) {
      if (!first_field) {
        csv_line += ",";
      }
      first_field = false;

      if (field.array_size == 1) {
        csv_line += field.toString(data, offset);
      } else {
        // 处理数组
        for (int j = 0; j < field.array_size; ++j) {
          csv_line += field.toString(
              data, offset + j * (field.getSize() / field.array_size));
          if (j < field.array_size - 1) {
            csv_line += ",";
          }
        }
      }
    }

    offset += field.getSize();
  }

  return csv_line;
}

std::string DataStructureManager::generateCsvHeader(uint32_t group_id,
                                                    uint32_t msg_id) {
  auto structure = getStructure(group_id, msg_id);
  if (!structure) {
    structure = structures_["RawImuDataDumpStruct"];
  }

  if (!structure) {
    return "";
  }

  std::string header;
  bool first_field = true;

  for (size_t i = 0; i < structure->fields.size(); ++i) {
    const auto &field = structure->fields[i];

    // 跳过标记为 ignore 的字段
    if (!first_field) {
      header += ", ";
    }
    first_field = false;

    if (field.array_size == 1) {
      header += field.name;
    } else {
      for (int j = 0; j < field.array_size; ++j) {
        header += field.name + "[" + std::to_string(j) + "]";
        if (j < field.array_size - 1) {
          header += ", ";
        }
      }
    }
  }

  return header;
}

} // namespace datadump
} // namespace toolkits
} // namespace xreal
