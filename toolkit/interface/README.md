# Interface Library - 动态库接口

此目录包含三个动态库的接口实现：

## 1. libsend_grpc.so
仅包含 gRPC 发送功能

**接口函数：**
- `void MakeStub(char* target_name, size_t target_size)` - 创建 gRPC stub
- `void SendRawImuData(...)` - 发送 IMU 原始数据
- `void SendBinaryData(...)` - 发送二进制数据
- `void SendImageData(...)` - 发送图像数据

## 2. libsave_file.so
仅包含文件保存功能

**接口函数：**
- `void InitSaveFile(const char* save_dir)` - 初始化文件保存（指定保存目录）
- `void SetFilenameMapping(uint64_t group_id, uint64_t msg_id, const char* filename)` - 设置消息ID到文件名的映射
- `void SaveData(int group_id, int msg_id, const uint8_t* data, uint64_t len)` - 保存数据到文件
- `void CloseSaveFile()` - 关闭文件保存

## 3. liball_in_one.so
包含文件保存和 gRPC 发送的所有功能

**接口函数：** 包含上述两个库的所有接口

## 编译

```bash
cd /path/to/toolkit
mkdir -p build && cd build
cmake .. -DDATADUMP_SRC_DIR=/path/to/toolkit/datadump \
         -DDATADUMP_BUILD_DIR=/path/to/build/toolkit/datadump
make
```

生成的动态库位于 `build/lib/` 目录下。
