#ifndef PROJECT_XREAL_LINK_H
#define PROJECT_XREAL_LINK_H

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define SERVER_PORT 7099
#ifndef LOCAL_ADDRESS
#define LOCAL_ADDRESS "127.0.0.1"
#endif

// #define NRLINK_MULTI_THREAD
#define NRLINK_WITH_EIGEN

#ifdef NRLINK_MULTI_THREAD
#include "xreal_link/nonLockBuffer.h"
#endif

#ifdef NRLINK_WITH_EIGEN
#include <Eigen/Eigen>
#endif

namespace xreal {
namespace toolkits {
namespace datadump {

#pragma pack(1)
struct XrealLinkMsgHeader {
  uint8_t magic;
  int32_t msg_id;
  int32_t payload_length;
  uint64_t time_stamp;
};
#pragma pack()


uint64_t get_current_timestamp_us() {
    struct timespec cur_time;
#ifdef _WIN32
    // Windows-specific implementation
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static const unsigned __int64 DELTA_EPOCH_IN_MICROSECS = 11644473600000000;

    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  // convert into microseconds
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    cur_time.tv_sec = (tmpres / 1000000UL);
    cur_time.tv_nsec = (tmpres % 1000000UL) * 1000;
#else
    // POSIX implementation
    clock_gettime(CLOCK_MONOTONIC, &cur_time);
#endif
    return (uint64_t)cur_time.tv_sec * 1000000 + (uint64_t)cur_time.tv_nsec / 1000;
}


uint64_t get_current_timestamp_ns() {
    struct timespec cur_time;
#ifdef _WIN32
    // Windows-specific implementation
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static const unsigned __int64 DELTA_EPOCH_IN_MICROSECS = 11644473600000000;

    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  // convert into microseconds
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    cur_time.tv_sec = (tmpres / 1000000UL);
    cur_time.tv_nsec = (tmpres % 1000000UL) * 1000;
#else
    // POSIX implementation
    clock_gettime(CLOCK_MONOTONIC, &cur_time);
#endif
    return (uint64_t)cur_time.tv_sec * 100000000 + (uint64_t)cur_time.tv_nsec;
}

class XrealLink {
 protected:
  int client_fd{-1};

  struct sockaddr_in server_addr;

  bool use_xreal_link_{0};
#ifdef NRLINK_MULTI_THREAD
  std::mutex mutex_;
  std::condition_variable cond_;
  std::thread send_thread_;
  bool send_thread_running = false;

  ConcurrentBufferNoLock<std::vector<uint8_t>> msg_buffer_{5000};
#endif
  XrealLink(const char *server_address = LOCAL_ADDRESS) {
#ifdef _WIN32
    /// Winsows uses wsasocket
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR) {
      printf("WSAStartup() fail\n");
      exit(0);
    }
#endif
    client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_fd == -1) {
      printf("socket() fail.\n");
      exit(0);
    }
    printf("socket() client fd is %d.\n", client_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_address);
    server_addr.sin_port = htons(SERVER_PORT);
  }

 public:
  ~XrealLink() {
    if (client_fd) {
#ifdef _WIN32
      closesocket(client_fd);
      WSACleanup();
#else
      close(client_fd);
#endif
    }
#ifdef NRLINK_MULTI_THREAD
    if (send_thread_running && send_thread_.joinable()) {
      send_thread_running = false;
      send_thread_.join();
    }
#endif
  }

  static void msgSetTime(XrealLinkMsgHeader &_msg, uint64_t _ts) {
    if (!_ts) {
      _msg.time_stamp = get_current_timestamp_us(); 
    }
  }

  static void msgInit(XrealLinkMsgHeader &_msg, int _group_id, int _msg_id, int32_t _len, uint64_t _ts) {
    _msg.magic = _group_id;
    _msg.msg_id = _msg_id;
    _msg.payload_length = _len;
    msgSetTime(_msg, _ts);
  }

  static void msgSetBuf(XrealLinkMsgHeader &_msg, std::vector<uint8_t> &_buffer, uint8_t *_payload) {
    _buffer.resize(sizeof(XrealLinkMsgHeader) + _msg.payload_length);
    memcpy(_buffer.data(), (uint8_t *)&_msg, sizeof(XrealLinkMsgHeader));
    memcpy(_buffer.data() + sizeof(XrealLinkMsgHeader), _payload, _msg.payload_length);
  }

  template <typename T>
  static void linkSendStatus(int _msg_id, const std::vector<T> &_vec, uint64_t _time_stamp_us = 0) {
    linkSendStatus(0xfd, _msg_id, _vec, _time_stamp_us);
  }

#ifdef NRLINK_WITH_EIGEN
  static void linkSendStatus(int _msg_id, const Eigen::VectorXd &_status, uint64_t _time_stamp_us = 0) {
    std::vector<uint8_t> buffer;
    buffer.resize(_status.cols() * _status.rows() * sizeof(double));
    memcpy(buffer.data(), (uint8_t *)_status.data(), buffer.size());

    XrealLink::linkSendStatus(_msg_id, buffer);
  }
#endif
  static void linkSendStatus(int _group_id, int _msg_id, const uint8_t *data, uint64_t len,
                             uint64_t _time_stamp_us = 0) {
    XrealLinkMsgHeader msg;
    msgInit(msg, _group_id, _msg_id, len, _time_stamp_us);

    std::vector<uint8_t> buffer;
    msgSetBuf(msg, buffer, (uint8_t *)data);
#ifdef NRLINK_MULTI_THREAD
    if (!XrealLink::getInstance()->send_thread_running) {
      std::cout << "Nreak link create send thread" << std::endl;
      XrealLink::getInstance()->send_thread_running = true;
      XrealLink::getInstance()->send_thread_ = std::thread(&XrealLink::udpSendThread, XrealLink::getInstance());
    }
    XrealLink::getInstance()->udpSendMessageNonBlocking(buffer);
#else
    XrealLink::getInstance()->udpSendMessage(buffer.data(), buffer.size());
#endif
  }

  template <typename T>
  static void linkSendStatus(int _group_id, int _msg_id, const std::vector<T> &_vec, uint64_t _time_stamp_us = 0) {
    XrealLinkMsgHeader msg;
    msgInit(msg, _group_id, _msg_id, _vec.size() * sizeof(T), _time_stamp_us);

    std::vector<uint8_t> buffer;
    msgSetBuf(msg, buffer, (uint8_t *)_vec.data());
#ifdef NRLINK_MULTI_THREAD
    if (!XrealLink::getInstance()->send_thread_running) {
      std::cout << "Nreak link create send thread" << std::endl;
      XrealLink::getInstance()->send_thread_running = true;
      XrealLink::getInstance()->send_thread_ = std::thread(&XrealLink::udpSendThread, XrealLink::getInstance());
    }
    XrealLink::getInstance()->udpSendMessageNonBlocking(buffer);
#else
    XrealLink::getInstance()->udpSendMessage(buffer.data(), buffer.size());
#endif
  }

  void udpSendMessage(void *_buf, size_t _n) {
    if (client_fd) {
      if (sendto(client_fd, (const char *)_buf, _n, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "Xreal link Send Message Failed!!!" << std::endl;
      }
    }
  }
#ifdef NRLINK_MULTI_THREAD
  void udpSendMessageNonBlocking(std::vector<uint8_t> &buffer) {
    if (client_fd) {
      auto writable_buffer = msg_buffer_.getWritableBuffer();
      *writable_buffer = buffer;
      msg_buffer_.setValid(true);
      msg_buffer_.doneWriteBuffer();
      cond_.notify_all();
    }
  }

  void udpSendThread() {
    int send_idx = 0;
    send_thread_running = true;
    std::unique_lock<std::mutex> lk(mutex_);
    pthread_setname_np(pthread_self(), std::string("NRLINK").substr(0, 15).c_str());
    while (send_thread_running) {
      int write_idx = msg_buffer_.getReadableARBufferIndex();
      if (!msg_buffer_.isValid() || send_idx == write_idx) {
        cond_.wait_for(lk, std::chrono::milliseconds(100));
        continue;
      }
      send_idx++;
      if (send_idx >= msg_buffer_.getSize()) {
        send_idx = 0;
      }
      auto buffer = msg_buffer_.getBuffer(send_idx);
      udpSendMessage(buffer->data(), buffer->size());
    }
  }
#endif
  static XrealLink *getInstance(const char *addr = LOCAL_ADDRESS) {
    static XrealLink instance(addr);
    return &instance;
  }

  static int setServerAddr(const char *addr) {
    auto instance = getInstance(addr);
    if (addr != LOCAL_ADDRESS) {
      if (instance->server_addr.sin_addr.s_addr != inet_addr(addr)) {
        sockaddr_in new_addr;
        memset(&new_addr, 0, sizeof(new_addr));
        new_addr.sin_family = AF_INET;
        new_addr.sin_addr.s_addr = inet_addr(addr);
        new_addr.sin_port = htons(SERVER_PORT);
        instance->server_addr = new_addr;
      }
    }
    return 0;
  }

  static bool &use_xreal_link() { return getInstance()->use_xreal_link_; }
};
}  // namespace utils
}  // namespace imu_tracking
}  // namespace xreal

#endif
