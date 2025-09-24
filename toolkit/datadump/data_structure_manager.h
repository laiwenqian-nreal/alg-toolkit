#ifndef DATA_STRUCTURE_MANAGER_H
#define DATA_STRUCTURE_MANAGER_H

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// 简化的JSON解析 - 如果项目中有其他JSON库可以替换
#include <algorithm>

namespace xreal {
namespace toolkits {
namespace datadump {

// 数据字段定义
struct DataField {
  std::string type;    // u64, u32, f32 等
  std::string name;    // 字段名
  std::string unit;    // 单位（可选）
  int array_size = 1;  // 数组大小，默认为1
  bool ignore = false; // 是否忽略显示

  // 获取字段大小
  size_t getSize() const {
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

  // 转换为字符串
  std::string toString(const void *data, size_t offset) const {
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
};

// 数据结构定义
struct DataStructure {
  uint32_t group_id;
  uint32_t msg_id;
  std::string name;
  std::vector<DataField> fields;
  size_t total_size = 0;

  // 计算总大小
  void calculateSize() {
    total_size = 0;
    for (const auto &field : fields) {
      total_size += field.getSize();
    }
  }

  // 获取唯一ID
  uint64_t getUniqueId() const {
    return (static_cast<uint64_t>(group_id) << 32) | msg_id;
  }
};

// 通用数据结构（固定字段）
struct CommonDataStructure {
  static std::vector<DataField> getCommonFields() {
    return {{"u64", "onsensor_timestamp_us", "us", 1, false},
            {"u64", "timestamp_ns", "ns", 1, false},
            {"u32", "type", "", 1, false},
            {"f32", "data0", "", 1, false},
            {"f32", "data1", "", 1, false},
            {"f32", "data2", "", 1, false},
            {"f32", "data3", "", 1, false},
            {"f32", "data4", "", 1, false},
            {"f32", "data5", "", 1, false}};
  }

  static size_t getCommonSize() {
    return 8 + 8 + 4 + 6 * 4; // 64bit onsensor_timestamp_us + 64bit
                              // timestamp_ns + 32bit type + 6x 32bit float
  }
};

// 数据结构管理器
class DataStructureManager {
private:
  std::map<uint64_t, std::shared_ptr<DataStructure>> structures_;
  std::map<std::string, uint32_t> name_to_group_id_;
  std::map<uint32_t, std::string> group_id_to_name_;

  // 简化的JSON解析
  std::string parseJsonString(const std::string &json, const std::string &key) {
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

  int parseJsonInt(const std::string &json, const std::string &key) {
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

  std::vector<std::string> parseJsonArray(const std::string &json,
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

public:
  DataStructureManager() {
    // 添加默认的通用数据结构
    registerCommonStructure();
  }

  // 注册通用数据结构
  void registerCommonStructure() {
    auto common_struct = std::make_shared<DataStructure>();
    common_struct->group_id = 0xFD; // 默认group_id
    common_struct->msg_id = 0x01;   // 默认msg_id
    common_struct->name = "CommonData";
    common_struct->fields = CommonDataStructure::getCommonFields();
    common_struct->calculateSize();

    structures_[common_struct->getUniqueId()] = common_struct;
  }

  // 从JSON字符串加载数据结构
  bool loadFromJsonString(const std::string &json_config) {
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
        data_struct->group_id = parseJsonInt(group_json, "GROUP_ID");
        data_struct->msg_id = parseJsonInt(group_json, "MSG_ID");
        data_struct->name = parseJsonString(group_json, "name");

        if (data_struct->group_id == -1 || data_struct->msg_id == -1) {
          pos = group_end + 1;
          continue;
        }

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

            // 检查隐藏字段
            if (field.name.find("HIDE") != std::string::npos) {
              field.ignore = true;
            }

            data_struct->fields.push_back(field);
          }
        }

        data_struct->calculateSize();
        structures_[data_struct->getUniqueId()] = data_struct;

        // 更新名称映射
        name_to_group_id_[data_struct->name] = data_struct->group_id;
        group_id_to_name_[data_struct->group_id] = data_struct->name;

        pos = group_end + 1;
      }

      return true;
    } catch (const std::exception &e) {
      std::cerr << "Error parsing JSON: " << e.what() << std::endl;
      return false;
    }
  }

  // 从JSON文件加载数据结构
  bool loadFromJsonFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    std::string json_content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();

    return loadFromJsonString(json_content);
  }

  // 根据group_id和msg_id获取数据结构
  std::shared_ptr<DataStructure> getStructure(uint32_t group_id,
                                              uint32_t msg_id) {
    uint64_t key = (static_cast<uint64_t>(group_id) << 32) | msg_id;
    auto it = structures_.find(key);
    if (it != structures_.end()) {
      return it->second;
    }
    return nullptr;
  }

  // 解析数据并生成CSV行
  std::string parseDataToCsv(uint32_t group_id, uint32_t msg_id,
                             const void *data, size_t data_size) {
    auto structure = getStructure(group_id, msg_id);
    if (!structure) {
      // 使用通用结构
      structure = getStructure(0xFD, 0x01);
    }

    if (!structure || data_size < structure->total_size) {
      return "";
    }

    std::string csv_line;
    size_t offset = 0;

    for (size_t i = 0; i < structure->fields.size(); ++i) {
      const auto &field = structure->fields[i];

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

      if (i < structure->fields.size() - 1) {
        csv_line += ",";
      }

      offset += field.getSize();
    }

    return csv_line;
  }

  // 生成CSV头
  std::string generateCsvHeader(uint32_t group_id, uint32_t msg_id) {
    auto structure = getStructure(group_id, msg_id);
    if (!structure) {
      structure = getStructure(0xFD, 0x01);
    }

    if (!structure) {
      return "";
    }

    std::string header;
    for (size_t i = 0; i < structure->fields.size(); ++i) {
      const auto &field = structure->fields[i];

      if (field.array_size == 1) {
        header += field.name;
      } else {
        for (int j = 0; j < field.array_size; ++j) {
          header += field.name + "[" + std::to_string(j) + "]";
          if (j < field.array_size - 1) {
            header += ",";
          }
        }
      }

      if (i < structure->fields.size() - 1) {
        header += ",";
      }
    }

    return header;
  }

  // 获取所有已注册的数据结构
  std::vector<std::shared_ptr<DataStructure>> getAllStructures() {
    std::vector<std::shared_ptr<DataStructure>> result;
    for (const auto &pair : structures_) {
      result.push_back(pair.second);
    }
    return result;
  }
};

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif // DATA_STRUCTURE_MANAGER_H