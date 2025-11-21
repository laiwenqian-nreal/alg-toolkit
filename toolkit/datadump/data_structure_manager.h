#ifndef DATA_STRUCTURE_MANAGER_H
#define DATA_STRUCTURE_MANAGER_H

#include <dump_data.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
  std::map<uint32_t, std::string> msg_id_to_field_definitions_;

  // 简化的JSON解析
  std::string parseJsonString(const std::string &json, const std::string &key);
  int parseJsonInt(const std::string &json, const std::string &key);
  std::vector<std::string> parseJsonArray(const std::string &json,
                                          const std::string &key);

public:
  DataStructureManager();

  DataStructureManager(
      const std::string &data_struct,
      const std::map<uint32_t, std::string> &msg_id_to_field_definitions);

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