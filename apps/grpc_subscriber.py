#!/usr/bin/env python3
"""
Python 订阅者程序 - 订阅 grpc_receiver_send_by_topic 发布的数据

This script subscribes to ROS-like topics published by grpc_receiver_send_by_topic
and prints the received data to console.

Topics:
  - grpc/imu: IMU data (6D array of doubles)
  - grpc/latency: Latency data (6D array of uint64)
  - grpc/image: Image data (binary encoded as base64)
  - grpc/binary: Binary data (base64 encoded)
"""

import zmq
import json
import base64
import argparse
import sys
from typing import Optional, Dict, Any
import signal


class GRPCDataSubscriber:
    """订阅 gRPC 接收器发布的数据"""

    def __init__(self, master_addr: str = "tcp://127.0.0.1:5590", timeout: Optional[int] = None):
        """
        初始化订阅者

        Args:
            master_addr: Master 服务器地址 (default: tcp://127.0.0.1:5590)
            timeout: 接收超时时间（毫秒），None 表示无限等待
        """
        self.master_addr = master_addr
        self.timeout = timeout
        self.context = None
        self.sockets = {}
        self.running = True
        self.publisher_cache = {}

    def connect_to_master(self) -> Optional[Dict[str, int]]:
        """
        连接到 Master 获取发布者信息

        Returns:
            发布者信息字典 {topic: port}，连接失败时返回 None
        """
        try:
            ctx = zmq.Context()
            socket = ctx.socket(zmq.REQ)
            socket.setsockopt(zmq.RCVTIMEO, 3000)  # 3 秒超时
            socket.connect(self.master_addr)

            # 构建查询请求
            query = {
                "type": "query_publishers",
                "topic": "grpc/*"  # 查询所有 grpc 开头的 topic
            }

            # 发送请求
            socket.send_json(query)
            response = socket.recv_json()

            socket.close()
            ctx.term()

            print(f"✓ Connected to Master at {self.master_addr}")

            # 解析发布者信息
            publishers = {}
            if response.get("success"):
                for pub in response.get("publishers", []):
                    topic = pub.get("topic")
                    port = pub.get("port")
                    address = pub.get("address", "127.0.0.1")
                    if topic and port:
                        publishers[topic] = port

            print(f"✓ Found publishers: {publishers}")
            return publishers if publishers else None

        except zmq.error.Again:
            print(f"✗ Master query timeout at {self.master_addr}")
            print("  Make sure the gRPC receiver and Master are running")
            return None
        except Exception as e:
            print(f"✗ Failed to connect to Master: {e}")
            return None

    def subscribe_to_topic(self, topic: str, port: int):
        """
        订阅指定主题

        Args:
            topic: 主题名称
            port: 发布者端口
        """
        try:
            if self.context is None:
                self.context = zmq.Context()

            socket = self.context.socket(zmq.SUB)

            # 设置订阅过滤 - 仅订阅该 topic 的消息
            if isinstance(topic, str):
                socket.setsockopt_string(zmq.SUBSCRIBE, topic)
            else:
                socket.setsockopt(zmq.SUBSCRIBE, topic.encode())

            # 设置接收超时
            if self.timeout:
                socket.setsockopt(zmq.RCVTIMEO, self.timeout)

            # 连接到发布者
            addr = f"tcp://127.0.0.1:{port}"
            socket.connect(addr)

            self.sockets[topic] = socket
            print(f"✓ Subscribed to topic '{topic}' on port {port}")

        except Exception as e:
            print(f"✗ Failed to subscribe to topic '{topic}': {e}")

    def parse_message(self, msg_str: str) -> Optional[Dict[str, Any]]:
        """
        解析接收到的消息

        Args:
            msg_str: 消息字符串 (JSON 格式)

        Returns:
            解析后的消息字典，格式错误时返回 None
        """
        try:
            return json.loads(msg_str)
        except json.JSONDecodeError:
            return None

    def print_imu_data(self, data: Dict[str, Any]):
        """打印 IMU 数据"""
        print(f"\n📊 IMU Data Received")
        print(f"   Topic: {data.get('topic', 'N/A')}")
        print(f"   Timestamp (ns): {data.get('timestamp_ns', 'N/A')}")
        print(f"   Type: {data.get('data_type', 'N/A')}")
        imu_data = data.get('data', [])
        print(f"   Data: [{', '.join(f'{x:.6f}' for x in imu_data)}]")

    def print_latency_data(self, data: Dict[str, Any]):
        """打印延迟数据"""
        print(f"\n⏱️  Latency Data Received")
        print(f"   Topic: {data.get('topic', 'N/A')}")
        print(f"   Timestamp (ns): {data.get('timestamp_ns', 'N/A')}")
        print(f"   Type: {data.get('data_type', 'N/A')}")
        latency_data = data.get('data', [])
        print(f"   Data: {latency_data}")

    def print_image_data(self, data: Dict[str, Any]):
        """打印图像数据信息"""
        print(f"\n🖼️  Image Data Received")
        print(f"   Topic: {data.get('topic', 'N/A')}")
        print(f"   Filename: {data.get('filename', 'N/A')}")
        image_b64 = data.get('data', '')
        if image_b64:
            try:
                image_bytes = base64.b64decode(image_b64)
                print(f"   Size: {len(image_bytes)} bytes")
                # 尝试识别图像格式
                if image_bytes[:4] == b'\x89PNG':
                    print(f"   Format: PNG")
                elif image_bytes[:3] == b'\xff\xd8\xff':
                    print(f"   Format: JPEG")
                elif image_bytes[:2] == b'BM':
                    print(f"   Format: BMP")
                else:
                    print(f"   Format: Unknown (first 4 bytes: {image_bytes[:4].hex()})")
            except Exception as e:
                print(f"   Error decoding image: {e}")
        else:
            print(f"   Size: 0 bytes")

    def print_binary_data(self, data: Dict[str, Any]):
        """打印二进制数据信息"""
        print(f"\n💾 Binary Data Received")
        print(f"   Topic: {data.get('topic', 'N/A')}")
        binary_b64 = data.get('data', '')
        if binary_b64:
            try:
                binary_bytes = base64.b64decode(binary_b64)
                print(f"   Size: {len(binary_bytes)} bytes")
                print(f"   First 32 bytes (hex): {binary_bytes[:32].hex()}")
            except Exception as e:
                print(f"   Error decoding binary data: {e}")
        else:
            print(f"   Size: 0 bytes")

    def run(self):
        """
        运行订阅者 - 连接并接收数据
        """
        # 连接到 Master 获取发布者信息
        publishers = self.connect_to_master()
        if not publishers:
            print("\n⚠️  Using default ports (auto-discovery failed)")
            # 使用默认端口
            publishers = {
                "grpc/imu": 41207,
                "grpc/latency": 45345,
                "grpc/image": 44681,
                "grpc/binary": 38167,
            }

        # 订阅所有 topic
        for topic, port in publishers.items():
            self.subscribe_to_topic(topic, port)

        if not self.sockets:
            print("✗ No topics subscribed. Exiting.")
            return

        print("\n" + "=" * 60)
        print("Listening for messages... (Press Ctrl+C to exit)")
        print("=" * 60)

        try:
            while self.running:
                for topic, socket in self.sockets.items():
                    try:
                        # 接收消息
                        message = socket.recv_string(zmq.NOBLOCK)

                        # ZMQ PUB/SUB 格式: "topic message"
                        # 分离 topic 和 payload
                        parts = message.split(" ", 1)
                        if len(parts) == 2:
                            _, payload = parts
                            msg_data = self.parse_message(payload)

                            if msg_data:
                                msg_type = msg_data.get('type', 'Unknown')

                                # 根据消息类型打印不同信息
                                if msg_type == 'ImuMessage':
                                    self.print_imu_data(msg_data)
                                elif msg_type == 'LatencyMessage':
                                    self.print_latency_data(msg_data)
                                elif msg_type == 'ImageMessage':
                                    self.print_image_data(msg_data)
                                elif msg_type == 'BinaryMessage':
                                    self.print_binary_data(msg_data)
                                else:
                                    print(f"\n❓ Unknown message type: {msg_type}")
                                    print(f"   Data: {msg_data}")

                    except zmq.error.Again:
                        # No message available on this socket
                        pass
                    except Exception as e:
                        print(f"Error receiving from {topic}: {e}")

        except KeyboardInterrupt:
            print("\n\n" + "=" * 60)
            print("Shutting down...")
            print("=" * 60)
        finally:
            self.cleanup()

    def cleanup(self):
        """清理资源"""
        print("Cleaning up...")
        for socket in self.sockets.values():
            socket.close()
        if self.context:
            self.context.term()
        print("✓ Cleanup complete")


def main():
    parser = argparse.ArgumentParser(
        description="Subscribe to gRPC data published by grpc_receiver_send_by_topic",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Using default Master address (localhost:5590)
  %(prog)s

  # Using custom Master address
  %(prog)s -m tcp://192.168.1.100:5590

  # With receive timeout (5 seconds)
  %(prog)s -t 5000
        """
    )

    parser.add_argument(
        "-m", "--master",
        default="tcp://127.0.0.1:5590",
        help="Master server address (default: tcp://127.0.0.1:5590)"
    )

    parser.add_argument(
        "-t", "--timeout",
        type=int,
        help="Receive timeout in milliseconds (default: None = infinite)"
    )

    args = parser.parse_args()

    # 创建订阅者
    subscriber = GRPCDataSubscriber(
        master_addr=args.master,
        timeout=args.timeout
    )

    # 设置信号处理器
    def signal_handler(sig, frame):
        subscriber.running = False

    signal.signal(signal.SIGINT, signal_handler)

    # 运行订阅者
    subscriber.run()


if __name__ == "__main__":
    main()
