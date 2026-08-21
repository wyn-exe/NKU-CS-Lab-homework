#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace rudp {

// 标志位定义，组合使用
constexpr std::uint16_t FLAG_SYN  = 0x0001;
constexpr std::uint16_t FLAG_ACK  = 0x0002;
constexpr std::uint16_t FLAG_FIN  = 0x0004;
constexpr std::uint16_t FLAG_RST  = 0x0008;
constexpr std::uint16_t FLAG_META = 0x0010;  // 传送文件元数据（文件名、大小）

// 发送/接收的基础配置
struct Config {
    std::size_t payload_size = 1000;        // 单个数据分片大小（字节）
    std::size_t window_packets = 32;        // 固定发送/接收窗口（以分片数计）
    int rto_ms = 300;                       // 超时重传定时器（毫秒）
    int conn_retry = 5;                     // 握手/终止重试次数
    int max_retransmit = 20;                // 数据分片最大重传次数
};

//记录传输的统计信息
struct TransferStats {
    std::uint64_t bytes = 0;
    double duration_ms = 0.0;
    double throughput_mbps = 0.0;
};

// 发送端入口
bool run_sender(const std::string& local_ip,
                std::uint16_t local_port,
                const std::string& remote_ip,
                std::uint16_t remote_port,
                const std::string& file_path,
                const Config& cfg,
                TransferStats& stats);

// 接收端入口
bool run_receiver(const std::string& listen_ip,
                  std::uint16_t listen_port,
                  const std::string& output_dir,
                  const Config& cfg,
                  TransferStats& stats);

}  // namespace rudp
