#define NOMINMAX
#include "rdt_common/rdt_protocol.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

namespace {

using Clock = std::chrono::steady_clock;

void SetupConsoleUtf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

struct ClientOptions {
    std::string host = "127.0.0.1";
    unsigned short port = 9000;
    std::string file_path;
    std::uint16_t window = rdt::kDefaultWindowSize;
    std::size_t mss = rdt::kMaxPayloadSize;
    int rto_ms = 800;  // 初始超时重传定时器
};

struct Segment {
    rdt::PacketType type = rdt::PacketType::kData;
    std::uint32_t seq = 0;
    std::vector<std::uint8_t> payload;
    bool acked = false;
    Clock::time_point last_sent{};
};

void PrintUsage() {
    std::cout << "用法: rdt_client --host <服务器IP> --port <端口> --file <文件路径> [--window 32] [--mss 1000]\n";
}

bool SendPacket(SOCKET sock, const sockaddr_in& addr, const rdt::Packet& packet) {
    const auto buffer = rdt::SerializePacket(packet);
    const int sent = sendto(sock, reinterpret_cast<const char*>(buffer.data()),
                           static_cast<int>(buffer.size()), 0,
                           reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    return sent == static_cast<int>(buffer.size());
}

bool ReceivePacket(SOCKET sock, sockaddr_in& from, rdt::Packet& packet) {
    std::uint8_t buffer[1500];
    int from_len = sizeof(from);
    const int ret = recvfrom(sock, reinterpret_cast<char*>(buffer), sizeof(buffer), 0,
                             reinterpret_cast<sockaddr*>(&from), &from_len);
    if (ret <= 0) {
        return false;
    }
    return rdt::ParsePacket(buffer, static_cast<std::size_t>(ret), packet);
}

// 进行三次握手，成功后返回 true
bool PerformHandshake(SOCKET sock, const sockaddr_in& server_addr, std::uint32_t syn_seq,
                      std::uint16_t window) {
    rdt::Packet syn{};
    syn.type = rdt::PacketType::kSyn;
    syn.seq = syn_seq;
    syn.window = window;
    syn.length = 0;

    int retries = 5;
    while (retries-- > 0) {
        SendPacket(sock, server_addr, syn);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeval tv{};
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        const int ready = select(0, &readfds, nullptr, nullptr, &tv);
        if (ready <= 0) {
            continue;
        }

        sockaddr_in from{};
        rdt::Packet response{};
        if (!ReceivePacket(sock, from, response)) {
            continue;
        }
        if (response.type == rdt::PacketType::kSynAck && response.ack == syn_seq + 1) {
            rdt::Packet ack{};
            ack.type = rdt::PacketType::kAck;
            ack.seq = syn_seq + 1;
            ack.ack = response.seq + 1;
            ack.window = window;
            SendPacket(sock, server_addr, ack);
            return true;
        }
    }
    return false;
}

// 根据文件切片生成所有发送分片，meta 序号为 1，数据从 2 开始，最后 FIN
std::map<std::uint32_t, Segment> BuildSegments(const ClientOptions& opts, std::uint64_t& fin_seq) {
    std::ifstream file(opts.file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开文件: " + opts.file_path);
    }
    const auto file_size = std::filesystem::file_size(opts.file_path);
    const std::string file_name = std::filesystem::path(opts.file_path).filename().string();
    if (file_name.size() > 255) {
        throw std::runtime_error("文件名过长，限制 255 字节以内");
    }

    std::map<std::uint32_t, Segment> segments;

    // META 报文
    Segment meta{};
    meta.type = rdt::PacketType::kMeta;
    meta.seq = 1;
    meta.payload.resize(8 + 2 + file_name.size());
    const std::uint64_t net_size = rdt::HostToNet64(file_size);
    std::memcpy(meta.payload.data(), &net_size, 8);
    const std::uint16_t name_len = htons(static_cast<std::uint16_t>(file_name.size()));
    std::memcpy(meta.payload.data() + 8, &name_len, 2);
    std::memcpy(meta.payload.data() + 10, file_name.data(), file_name.size());
    segments.emplace(meta.seq, std::move(meta));

    // 数据分片
    std::uint32_t seq = 2;
    std::vector<std::uint8_t> buffer(opts.mss);
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(opts.mss));
        const std::streamsize got = file.gcount();
        if (got <= 0) {
            break;
        }
        Segment seg{};
        seg.type = rdt::PacketType::kData;
        seg.seq = seq++;
        seg.payload.assign(buffer.begin(), buffer.begin() + got);
        segments.emplace(seg.seq, std::move(seg));
    }

    // FIN
    Segment fin{};
    fin.type = rdt::PacketType::kFin;
    fin.seq = seq;
    segments.emplace(fin.seq, std::move(fin));
    fin_seq = seq;
    return segments;
}

}  // namespace

int main(int argc, char* argv[]) {
    SetupConsoleUtf8();  // 确保控制台使用 UTF-8，避免中文乱码

    ClientOptions opts;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            opts.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            opts.port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == "--file" && i + 1 < argc) {
            opts.file_path = argv[++i];
        } else if (arg == "--window" && i + 1 < argc) {
            opts.window = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--mss" && i + 1 < argc) {
            opts.mss = static_cast<std::size_t>(std::stoi(argv[++i]));
        }
    }

    if (opts.file_path.empty()) {
        PrintUsage();
        return 1;
    }
    if (opts.mss == 0 || opts.mss > rdt::kMaxPayloadSize) {
        std::cerr << "分片大小无效，必须在 1-" << rdt::kMaxPayloadSize << " 之间\n";
        return 1;
    }

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup 失败\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "创建 UDP 套接字失败\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(opts.port);
    if (inet_pton(AF_INET, opts.host.c_str(), &server_addr.sin_addr) != 1) {
        std::cerr << "服务器地址无效\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::random_device rd;
    std::mt19937 rng(static_cast<unsigned>(
        rd() ^ static_cast<unsigned>(Clock::now().time_since_epoch().count())));
    const std::uint32_t syn_seq = rng();

    std::cout << "[INFO] 开始握手..." << std::endl;
    if (!PerformHandshake(sock, server_addr, syn_seq, opts.window)) {
        std::cerr << "握手失败\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << "[INFO] 握手成功，准备发送文件\n";

    std::uint64_t fin_seq = 0;
    std::map<std::uint32_t, Segment> segments;
    try {
        segments = BuildSegments(opts, fin_seq);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const std::uint32_t final_ack_target = static_cast<std::uint32_t>(fin_seq + 1);
    std::uint32_t send_base = 1;
    std::uint32_t next_seq = 1;
    std::uint32_t latest_ack_value = 0;
    int dup_ack_count = 0;
    double cwnd = 1.0;        // 拥塞窗口（报文个数）
    double ssthresh = 16.0;   // 慢启动阈值
    const auto start_time = Clock::now();
    const std::uint64_t data_bytes = std::filesystem::file_size(opts.file_path);

    auto send_segment = [&](Segment& seg) {
        rdt::Packet pkt{};
        pkt.type = seg.type;
        pkt.seq = seg.seq;
        pkt.window = opts.window;
        pkt.length = static_cast<std::uint16_t>(seg.payload.size());
        pkt.payload = seg.payload;
        SendPacket(sock, server_addr, pkt);
        seg.last_sent = Clock::now();
    };

    // 主循环：流水线发送 + 处理 ACK + 超时重传
    while (send_base <= fin_seq) {
        // 计算可发送窗口：流量控制窗口与拥塞窗口取较小值
        const std::size_t congestion_cap = static_cast<std::size_t>(std::max(1.0, std::floor(cwnd)));
        const std::size_t window_cap = std::min<std::size_t>(opts.window, congestion_cap);

        // 尝试填满窗口
        while (next_seq <= fin_seq) {
            std::size_t inflight = 0;
            for (auto& kv : segments) {
                if (!kv.second.acked && kv.second.last_sent.time_since_epoch().count() != 0) {
                    if (kv.first >= send_base && kv.first < send_base + opts.window) {
                        ++inflight;
                    }
                }
            }
            if (inflight >= window_cap) {
                break;
            }
            auto it = segments.find(next_seq);
            if (it == segments.end()) {
                break;
            }
            send_segment(it->second);
            ++next_seq;
        }

        // 等待 ACK 或超时检查
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms
        const int ready = select(0, &readfds, nullptr, nullptr, &tv);

        if (ready > 0) {
            sockaddr_in from{};
            rdt::Packet pkt{};
            if (ReceivePacket(sock, from, pkt)) {
                if (pkt.type == rdt::PacketType::kAck || pkt.type == rdt::PacketType::kFinAck) {
                    const std::uint32_t ack_value = pkt.ack;
                    int newly_acked = 0;

                    // 累积确认
                    for (auto it = segments.begin(); it != segments.end(); ++it) {
                        if (!it->second.acked && it->first < ack_value) {
                            it->second.acked = true;
                            ++newly_acked;
                        }
                    }
                    // SACK 位图确认
                    for (std::size_t i = 0; i < rdt::kAckBitmapBits; ++i) {
                        if (pkt.ack_bits & (1u << i)) {
                            const std::uint32_t seq = ack_value + static_cast<std::uint32_t>(i) + 1;
                            auto it = segments.find(seq);
                            if (it != segments.end() && !it->second.acked) {
                                it->second.acked = true;
                                ++newly_acked;
                            }
                        }
                    }

                    // 拥塞控制：根据是否新确认调整 cwnd
                    if (ack_value > latest_ack_value || newly_acked > 0) {
                        latest_ack_value = std::max(latest_ack_value, ack_value);
                        dup_ack_count = 0;
                        const int total_new = newly_acked + static_cast<int>(ack_value > send_base ? (ack_value - send_base) : 0);
                        for (int i = 0; i < total_new; ++i) {
                            if (cwnd < ssthresh) {
                                cwnd += 1.0;  // 慢启动
                            } else {
                                cwnd += 1.0 / cwnd;  // 拥塞避免
                            }
                        }
                    } else {
                        ++dup_ack_count;
                        // 三次重复 ACK 触发快速重传
                        if (dup_ack_count >= 3 && segments.count(ack_value)) {
                            auto& seg = segments[ack_value];
                            send_segment(seg);
                            ssthresh = std::max(1.0, cwnd / 2.0);
                            cwnd = ssthresh;
                            dup_ack_count = 0;
                        }
                    }

                    // 滑动窗口
                    while (segments.count(send_base) && segments[send_base].acked) {
                        ++send_base;
                    }
                }
            }
        }

        // 超时重传检查
        const auto now = Clock::now();
        for (auto& kv : segments) {
            if (kv.first < send_base) {
                continue;
            }
            if (kv.second.acked) {
                continue;
            }
            if (kv.second.last_sent.time_since_epoch().count() == 0) {
                continue;
            }
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - kv.second.last_sent).count();
            if (elapsed >= opts.rto_ms) {
                send_segment(kv.second);
                ssthresh = std::max(1.0, cwnd / 2.0);
                cwnd = 1.0;
                dup_ack_count = 0;
            }
        }
    }

    const auto end_time = Clock::now();
    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    const double seconds = duration_ms / 1000.0;
    const double throughput = seconds > 0 ? data_bytes * 8.0 / seconds / 1e6 : 0.0;  // Mbps

    std::cout << "[INFO] 传输完成，用时 " << duration_ms << " ms，平均吞吐率 "
              << throughput << " Mbps" << std::endl;

    closesocket(sock);
    WSACleanup();
    return 0;
}
