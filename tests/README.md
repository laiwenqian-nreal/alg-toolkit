# Datadump Simple Tests

这个目录包含了datadump模块的简单测试程序，分别测试TCP发数据和文件保存功能。

## 测试文件说明

### 简单测试 (推荐)
- `simple_tcp_test.cpp` - 简单的TCP发送测试程序
- `simple_file_test.cpp` - 简单的文件保存测试程序  
- `build_simple_tests.sh` - 编译简单测试的脚本
- `run_simple_tests.sh` - 运行简单测试的脚本

### 单元测试 (可选)
- `test_tcp_send.cpp` - TCP发送功能的单元测试
- `test_file_save.cpp` - 文件保存功能的单元测试
- `CMakeLists.txt` - CMake构建配置
- `run_tests.sh` - 测试运行脚本

## 测试内容

### TCP发送测试 (test_tcp_send.cpp)
- 测试TCP连接功能
- 测试消息时间戳设置
- 测试数据包头设置
- 测试数据包数据设置
- 测试消息入队列
- 测试组消息处理
- 测试服务器地址设置
- 测试单例模式
- 性能测试

### 文件保存测试 (test_file_save.cpp)
- 测试文件保存器初始化
- 测试单例模式
- 测试文件输出流获取
- 测试CSV格式数据写入
- 测试批量写入功能
- 测试保存目录设置
- 测试大数据量处理
- 测试错误处理

## 快速开始 (简单测试)

### 编译和运行简单测试

```bash
cd toolkit/datadump/test

# 编译测试程序
chmod +x build_simple_tests.sh
./build_simple_tests.sh

# 运行所有测试
chmod +x run_simple_tests.sh
./run_simple_tests.sh

# 或者单独运行
./simple_file_test    # 文件保存测试
./simple_tcp_test     # TCP发送测试（需要先启动TCP服务器）
```

### TCP测试说明

TCP测试会发送数据到 `127.0.0.1:8099`，你可以用以下方式启动一个简单的TCP服务器来接收数据：

```bash
# 使用netcat监听
nc -l 8099

# 或者使用python
python3 -c "
import socket
s = socket.socket()
s.bind(('127.0.0.1', 8099))
s.listen(1)
print('Listening on port 8099...')
while True:
    conn, addr = s.accept()
    print(f'Connection from {addr}')
    data = conn.recv(1024)
    print(f'Received: {len(data)} bytes')
    conn.send(b'ok')
    conn.close()
"
```

### 文件测试说明

文件测试会在 `./test_output/` 目录下创建CSV文件，测试完成后可以查看生成的文件。

## 单元测试 (需要Google Test)

如果需要运行完整的单元测试，需要安装Google Test框架：

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev libgmock-dev
```

### 运行单元测试

```bash
cd toolkit/datadump/test
chmod +x run_tests.sh
./run_tests.sh
```

### 方法2：使用CMake

```bash
cd toolkit/datadump/test
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

### 方法3：手动编译运行

```bash
cd toolkit/datadump/test

# 编译TCP测试
g++ -std=c++17 -I.. -I../../ -I/usr/include/gtest -I/usr/include/gmock \
    test_tcp_send.cpp ../send_nviz_data_by_tcp.cpp ../xreal_link_common.cpp \
    -lgtest -lgtest_main -lgmock -pthread -o test_tcp_send

# 编译文件保存测试
g++ -std=c++17 -I.. -I../../ -I/usr/include/gtest -I/usr/include/gmock \
    test_file_save.cpp ../dump_nviz_data_to_file.cpp ../xreal_link_common.cpp \
    -lgtest -lgtest_main -lgmock -pthread -lstdc++fs -o test_file_save

# 运行测试
./test_tcp_send
./test_file_save
```

## 注意事项

1. **TCP测试注意事项**：
   - TCP连接测试需要在8099端口有服务器运行才能完全通过
   - 没有服务器时连接会失败，这是正常的预期行为
   - 其他功能测试不依赖外部服务器

2. **文件保存测试注意事项**：
   - 测试会在`/tmp/`目录下创建临时文件和目录
   - 测试完成后会自动清理临时文件
   - 需要有文件系统写权限

3. **性能测试**：
   - 包含了性能基准测试
   - 可以用来检测性能回归

## 测试覆盖的功能

- ✅ 基本功能测试
- ✅ 错误处理测试  
- ✅ 性能测试
- ✅ 单例模式测试
- ✅ 多线程安全测试（部分）
- ✅ 文件I/O测试
- ✅ 网络连接测试
