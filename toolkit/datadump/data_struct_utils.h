#ifndef DATA_STRUCT_UTILS_H
#define DATA_STRUCT_UTILS_H

#include "data_structure_manager.h"
#include <sstream>

namespace xreal {
namespace toolkits {
namespace datadump {

// 数据结构工具类
class DataStructUtils {
public:
    // 从C++结构体自动生成JSON配置
    template<typename T>
    static std::string generateJsonFromStruct(const std::string& struct_name, 
                                             uint32_t group_id, 
                                             uint32_t msg_id,
                                             const std::vector<std::string>& field_definitions) {
        std::ostringstream json;
        json << "{\n";
        json << "  \"" << struct_name << "\": {\n";
        json << "    \"GROUP_ID\": " << group_id << ",\n";
        json << "    \"MSG_ID\": " << msg_id << ",\n";
        json << "    \"name\": \"" << struct_name << "\",\n";
        json << "    \"struct\": [\n";
        
        for (size_t i = 0; i < field_definitions.size(); ++i) {
            json << "      \"" << field_definitions[i] << "\"";
            if (i < field_definitions.size() - 1) {
                json << ",";
            }
            json << "\n";
        }
        
        json << "    ]\n";
        json << "  }\n";
        json << "}";
        
        return json.str();
    }
    
    // 验证数据大小是否匹配
    static bool validateDataSize(uint32_t group_id, uint32_t msg_id, 
                                size_t actual_size, DataStructureManager& manager) {
        auto structure = manager.getStructure(group_id, msg_id);
        if (!structure) {
            return false; // 没有找到对应的数据结构
        }
        
        return actual_size >= structure->total_size;
    }
    
    // 生成数据结构描述
    static std::string describeStructure(uint32_t group_id, uint32_t msg_id, 
                                        DataStructureManager& manager) {
        auto structure = manager.getStructure(group_id, msg_id);
        if (!structure) {
            return "Unknown structure";
        }
        
        std::ostringstream desc;
        desc << "Structure: " << structure->name << "\n";
        desc << "GROUP_ID: " << structure->group_id << ", MSG_ID: " << structure->msg_id << "\n";
        desc << "Total size: " << structure->total_size << " bytes\n";
        desc << "Fields:\n";
        
        size_t offset = 0;
        for (const auto& field : structure->fields) {
            desc << "  [" << offset << "] " << field.type << " " << field.name;
            if (field.array_size > 1) {
                desc << "[" << field.array_size << "]";
            }
            desc << " (size: " << field.getSize() << " bytes)";
            if (!field.unit.empty()) {
                desc << " unit: " << field.unit;
            }
            if (field.ignore) {
                desc << " [HIDDEN]";
            }
            desc << "\n";
            offset += field.getSize();
        }
        
        return desc.str();
    }
    
    // 打印数据的十六进制表示
    static std::string hexDump(const void* data, size_t size, size_t bytes_per_line = 16) {
        std::ostringstream hex;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        
        for (size_t i = 0; i < size; i += bytes_per_line) {
            // 偏移地址
            hex << std::hex << std::setfill('0') << std::setw(8) << i << ": ";
            
            // 十六进制数据
            for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
                hex << std::hex << std::setfill('0') << std::setw(2) << (int)bytes[i + j] << " ";
            }
            
            // 填充空格
            for (size_t j = size - i; j < bytes_per_line && j < size; ++j) {
                hex << "   ";
            }
            
            // ASCII表示
            hex << " |";
            for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
                char c = bytes[i + j];
                hex << (std::isprint(c) ? c : '.');
            }
            hex << "|\n";
        }
        
        return hex.str();
    }
    
    // 比较两个数据结构的差异
    static std::string compareData(const void* data1, const void* data2, 
                                  uint32_t group_id, uint32_t msg_id,
                                  DataStructureManager& manager) {
        auto structure = manager.getStructure(group_id, msg_id);
        if (!structure) {
            return "Cannot compare: structure not found";
        }
        
        std::ostringstream diff;
        size_t offset = 0;
        
        for (const auto& field : structure->fields) {
            std::string value1 = field.toString(data1, offset);
            std::string value2 = field.toString(data2, offset);
            
            if (value1 != value2) {
                diff << field.name << ": " << value1 << " -> " << value2 << "\n";
            }
            
            offset += field.getSize();
        }
        
        return diff.str();
    }
};

// 数据结构注册宏
#define REGISTER_DATA_STRUCT(struct_name, group_id, msg_id, ...) \
    do { \
        std::vector<std::string> fields = {__VA_ARGS__}; \
        std::string json = DataStructUtils::generateJsonFromStruct<struct_name>( \
            #struct_name, group_id, msg_id, fields); \
        XrealLinkCommon::loadDataStructuresFromJson(json); \
    } while(0)

// 简化的数据发送宏
#define SEND_STRUCT_DATA(group_id, msg_id, data) \
    XrealLinkOnlySaveFile::linkSendStatus(group_id, msg_id, \
        reinterpret_cast<const uint8_t*>(&data), sizeof(data))

#define SEND_STRUCT_DATA_TCP(group_id, msg_id, data) \
    XrealLinkTcp::linkSendStatus(group_id, msg_id, \
        reinterpret_cast<const uint8_t*>(&data), sizeof(data))

} // namespace datadump
} // namespace toolkits
} // namespace xreal

#endif // DATA_STRUCT_UTILS_H