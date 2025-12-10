#include "send_nviz_data_by_grpc.h"
#include "../util/logging.h"
#include "data_structure_manager.h"
#include <chrono>
#include <errno.h>
#include <iostream>

using xreal::toolkits::utils::Logger;

namespace xreal {
namespace toolkits {
namespace datadump {

XrealLinkgRPC::XrealLinkgRPC(const std::string &server_address)
    : server_address_(server_address) {
  log_prefix = "[xreal_link_grpc]";

  // 连接到 gRPC 服务器
  connectServer();
}

XrealLinkgRPC::~XrealLinkgRPC() { _Close(); }

void XrealLinkgRPC::_Close() {
  connected_ = false;
  stub_.reset();

  // 清理线程
  cleanupThreads();
}

void XrealLinkgRPC::Close() { XrealLinkgRPC::getInstance()->_Close(); }

bool XrealLinkgRPC::connectServer() {
  try {
    grpc::ChannelArguments args;
    // 设置大消息支持 (可选)
    // args.SetMaxSendMessageSize(64*1024*1024);
    // args.SetMaxReceiveMessageSize(-1);

    auto channel = grpc::CreateCustomChannel(
        server_address_, grpc::InsecureChannelCredentials(), args);

    stub_ = AlgService::NewStub(channel);
    connected_ = true;

    DLOG_INFO("{} Connected to gRPC server: {}", log_prefix, server_address_);
    return true;
  } catch (const std::exception &e) {
    DLOG_ERROR("{} Failed to connect to gRPC server {}: {}", log_prefix,
               server_address_, e.what());
    connected_ = false;
    return false;
  }
}

void XrealLinkgRPC::processGroupMsg(DataBuffer &group_msg, uint32_t freq_count,
                                    uint64_t timestamp_ns, uint64_t packet_id) {
  // gRPC 直接将分组后的消息发送到 send 队列
  concurrent_lock_free_queue_send_.enqueue(group_msg);
}

void XrealLinkgRPC::sendThread() {
  DataBuffer msg;

  while (send_thread_running) {
    while (concurrent_lock_free_queue_send_.try_dequeue(msg)) {
      if (!connected_) {
        DLOG_WARN("{} Not connected, attempting to reconnect...", log_prefix);
        connectServer();
      }

      if (connected_) {
        sendMsg(msg);
      }
    }

    // 如果队列为空，适当休眠避免忙等待
    if (concurrent_lock_free_queue_send_.size_approx() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

bool XrealLinkgRPC::sendMsg(DataBuffer msg) {
  size_t curLen = msg.size();
  uint8_t *curDataPtr = msg.data();

  while (curLen > 0) {
    if (curLen < sizeof(SimpleMessageHeader)) {
      break; // 不够一个消息头的大小
    }

    SimpleMessageHeader msg_header;
    uint8_t *payload_ptr = nullptr;
    size_t payload_size = 0;
    GetMsgPayload(curDataPtr, curLen, &msg_header, payload_ptr, payload_size);

    if (payload_size < static_cast<size_t>(msg_header.payload_length)) {
      break; // 数据不完整
    }

    bool success = false;

    // 根据 msg_id 选择不同的 gRPC 函数
    if (msg_header.msg_id == DUMP_MESSAGE_ID_RAW_IMU_DATA) {
      const size_t expected_size =
          structure_manager_.getStructure(msg_header.magic, msg_header.msg_id)
              ->total_size;
      if (msg_header.payload_length >= expected_size) {
        RawImuDataDumpStruct *data =
            reinterpret_cast<RawImuDataDumpStruct *>(payload_ptr);
        // onsensor timestamp 使用消息头中的 time_stamp
        success = sendRawImuData(msg_header.magic, msg_header.msg_id,
                                 msg_header.time_stamp, data, expected_size);
      } else {
        DLOG_WARN(
            "{} Invalid RawImuData size for msg_id={}, expected={}, got={}",
            log_prefix, msg_header.msg_id, expected_size,
            msg_header.payload_length);
      }
    } else if (msg_header.msg_id == DUMP_MESSAGE_ID_LATENCY_DATA) {
      const size_t expected_size =
          structure_manager_.getStructure(msg_header.magic, msg_header.msg_id)
              ->total_size;
      if (msg_header.payload_length >= expected_size) {
        RawLatencyDataDumpStruct *data =
            reinterpret_cast<RawLatencyDataDumpStruct *>(curDataPtr);
        // onsensor timestamp 使用消息头中的 time_stamp
        success = sendLatencyData(msg_header.magic, msg_header.msg_id,
                                  msg_header.time_stamp, data, expected_size);
      } else {
        DLOG_WARN(
            "{} Invalid LatencyData size for msg_id={}, expected={}, got={}",
            log_prefix, msg_header.msg_id, expected_size,
            msg_header.payload_length);
      }
    } else if (msg_header.msg_id == DUMP_MESSAGE_ID_IMAGE_DATA) {
      // 灰度图数据: 使用 ImageData
      success = sendImageData(msg_header.magic, msg_header.msg_id,
                              msg_header.time_stamp, curDataPtr,
                              msg_header.payload_length);
    } else {
      // 默认使用 BinaryData (包括 DUMP_MESSAGE_ID_BINARY_DATA)
      success = sendBinaryData(msg_header.magic, msg_header.msg_id,
                               msg_header.time_stamp, curDataPtr,
                               msg_header.payload_length);
    }

    if (!success) {
      DLOG_ERROR("{} Failed to send gRPC message, group_id={}, msg_id={}",
                 log_prefix, msg_header.magic, msg_header.msg_id);
    }

    // curDataPtr += msg_header.payload_length;
    // curLen -= msg_header.payload_length;
    curDataPtr += sizeof(SimpleMessageHeader) + msg_header.payload_length;
    curLen -= sizeof(SimpleMessageHeader) + msg_header.payload_length;
  }

  return true;
}

bool XrealLinkgRPC::sendRawImuData(uint32_t group_id, uint32_t msg_id,
                                   uint64_t onsensor_timestamp_us,
                                   RawImuDataDumpStruct *data,
                                   uint32_t data_len) {
  if (!stub_ || !connected_) {
    return false;
  }

  try {
    RawImuData req;

    // 设置 NvizHeader
    auto *header = req.mutable_header();
    header->set_group_id(group_id);
    header->set_msg_id(msg_id);
    header->set_onsensor_timestamp_us(onsensor_timestamp_us);

    // 设置数据
    req.set_timestamp_ns(data->timestamp_ns);
    req.set_type(data->type);
    req.set_data_1(data->data[0]);
    req.set_data_2(data->data[1]);
    req.set_data_3(data->data[2]);
    req.set_data_4(data->data[3]);
    req.set_data_5(data->data[4]);
    req.set_data_6(data->data[5]);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub_->SendRawImuData(&ctx, req, &resp);

    if (!status.ok()) {
      DLOG_ERROR("{} SendRawImuData failed: {}", log_prefix,
                 status.error_message());
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    DLOG_ERROR("{} SendRawImuData exception: {}", log_prefix, e.what());
    return false;
  }
}

bool XrealLinkgRPC::sendLatencyData(uint32_t group_id, uint32_t msg_id,
                                    uint64_t onsensor_timestamp_us,
                                    RawLatencyDataDumpStruct *data,
                                    uint32_t data_len) {
  if (!stub_ || !connected_) {
    return false;
  }

  try {
    LatencyData req;

    // 设置 NvizHeader
    auto *header = req.mutable_header();
    header->set_group_id(group_id);
    header->set_msg_id(msg_id);
    header->set_onsensor_timestamp_us(onsensor_timestamp_us);

    // 设置数据
    req.set_timestamp_ns(data->timestamp_ns);
    req.set_type(data->type);
    req.set_data_1(data->data[0]);
    req.set_data_2(data->data[1]);
    req.set_data_3(data->data[2]);
    req.set_data_4(data->data[3]);
    req.set_data_5(data->data[4]);
    req.set_data_6(data->data[5]);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub_->SendLatencyData(&ctx, req, &resp);

    if (!status.ok()) {
      DLOG_ERROR("{} SendLatencyData failed: {}", log_prefix,
                 status.error_message());
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    DLOG_ERROR("{} SendLatencyData exception: {}", log_prefix, e.what());
    return false;
  }
}

bool XrealLinkgRPC::sendBinaryData(uint32_t group_id, uint32_t msg_id,
                                   uint64_t onsensor_timestamp_us,
                                   const uint8_t *data, uint32_t data_len) {
  if (!stub_ || !connected_) {
    return false;
  }

  try {
    BinaryData req;

    // 设置 NvizHeader
    auto *header = req.mutable_header();
    header->set_group_id(group_id);
    header->set_msg_id(msg_id);
    header->set_onsensor_timestamp_us(onsensor_timestamp_us);

    // 设置数据
    req.set_data_len(data_len);
    req.set_payload(reinterpret_cast<const char *>(data), data_len);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub_->SendBinaryData(&ctx, req, &resp);

    if (!status.ok()) {
      DLOG_ERROR("{} SendBinaryData failed: {}", log_prefix,
                 status.error_message());
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    DLOG_ERROR("{} SendBinaryData exception: {}", log_prefix, e.what());
    return false;
  }
}

bool XrealLinkgRPC::sendImageData(uint32_t group_id, uint32_t msg_id,
                                  uint64_t onsensor_timestamp_us,
                                  const uint8_t *data, uint32_t data_len) {
  if (!stub_ || !connected_) {
    return false;
  }

  try {
    ImageData req;

    // 设置 NvizHeader
    auto *header = req.mutable_header();
    header->set_group_id(group_id);
    header->set_msg_id(msg_id);
    header->set_onsensor_timestamp_us(onsensor_timestamp_us);

    // 设置数据
    req.set_image_binary_data(reinterpret_cast<const char *>(data), data_len);

    grpc::ClientContext ctx;
    Ack resp;
    auto status = stub_->SendImageData(&ctx, req, &resp);

    if (!status.ok()) {
      DLOG_ERROR("{} SendImageData failed: {}", log_prefix,
                 status.error_message());
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    DLOG_ERROR("{} SendImageData exception: {}", log_prefix, e.what());
    return false;
  }
}

XrealLinkgRPC *XrealLinkgRPC::getInstance(const std::string &server_address) {
  static XrealLinkgRPC instance(server_address);
  return &instance;
}

void XrealLinkgRPC::linkSendStatus(int group_id, int msg_id,
                                   const uint8_t *data, uint64_t len) {
  DataBuffer raw_msg;
  raw_msg.resize(sizeof(XrealLinkCommon::RawMessageHeader) + len);
  XrealLinkCommon::RawMessageHeader *raw_header =
      (XrealLinkCommon::RawMessageHeader *)raw_msg.data();
  raw_header->group_id = group_id;
  raw_header->msg_id = msg_id;
  if (data && len > 0) {
    memcpy(raw_msg.data() + sizeof(XrealLinkCommon::RawMessageHeader), data,
           len);
  }
  XrealLinkgRPC::getInstance()->messageEnQueue(raw_msg);
}

} // namespace datadump
} // namespace toolkits
} // namespace xreal