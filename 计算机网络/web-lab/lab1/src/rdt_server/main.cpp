#define NOMINMAX
#include "rdt_common/rdt_protocol.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
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

struct ServerOptions {
    unsigned short port = 9000;
    std::uint16_t window = rdt::kDefaultWindowSize;
    std::string out_dir = "received";
};

struct ReceiverState {
    bool handshake_done = false;
    bool fin_received = false;
    std::uint32_t expected_seq = 1;  // 期望的下一个分片序号（含 META）
    std::uint32_t fin_seq = 0;
    std::map<std::uint32_t, std::vector<std::uint8_t>> buffer;
    std::uint64_t file_size = 0;
    std::uint64_t bytes_written = 0;
    std::ofstream file;
    std::string file_name;
    Clock::time_point start_time{};
    Clock::time_point last_ack_sent{};
};

void PrintUsage() {
    std::cout << "用法: rdt_server [--port 9000] [--window 32] [--outdir received]\n";
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

void EnsureOutDir(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
}

// 将已有的有序缓冲写入文件，并推进 expected_seq
void FlushInOrder(ReceiverState& state) {
    while (true) {
        auto it = state.buffer.find(state.expected_seq);
        if (it == state.buffer.end()) {
            break;
        }
        if (state.file.is_open() && !it->second.empty()) {
            state.file.write(reinterpret_cast<const char*>(it->second.data()),
                             static_cast<std::streamsize>(it->second.size()));
            state.bytes_written += it->second.size();
        }
        state.buffer.erase(it);
        ++state.expected_seq;
    }
}

std::uint32_t BuildAckBits(const ReceiverState& state) {
    std::uint32_t bits = 0;
    for (std::size_t i = 0; i < rdt::kAckBitmapBits; ++i) {
        const std::uint32_t seq = state.expected_seq + static_cast<std::uint32_t>(i) + 1;
        if (state.buffer.count(seq)) {
            bits |= (1u << i);
        }
    }
    return bits;
}

}  // namespace

int main(int argc, char* argv[]) {
    SetupConsoleUtf8();  // 确保控制台使用 UTF-8，避免中文提示乱码
    ServerOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            opts.port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == "--window" && i + 1 < argc) {
            opts.window = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--outdir" && i + 1 < argc) {
            opts.out_dir = argv[++i];
        }
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

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(opts.port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind 失败\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[INFO] 服务器监听 UDP 端口 " << opts.port << std::endl;

    sockaddr_in client_addr{};
    ReceiverState state;
    std::uint32_t server_isn = 12345;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms

        const int ready = select(0, &readfds, nullptr, nullptr, &tv);
        if (ready <= 0) {
            // 周期性发送 ACK，防止对端长时间等待
            if (state.handshake_done) {
                rdt::Packet ack{};
                ack.type = state.fin_received && state.expected_seq == state.fin_seq + 1
                               ? rdt::PacketType::kFinAck
                               : rdt::PacketType::kAck;
                ack.ack = state.expected_seq;
                ack.window = static_cast<std::uint16_t>(
                    state.buffer.size() >= opts.window ? 0 : opts.window - state.buffer.size());
                ack.ack_bits = BuildAckBits(state);
                SendPacket(sock, client_addr, ack);
                state.last_ack_sent = Clock::now();
            }
            continue;
        }

        sockaddr_in from{};
        rdt::Packet pkt{};
        if (!ReceivePacket(sock, from, pkt)) {
            continue;
        }

        // 握手阶段：等待 SYN
        if (!state.handshake_done) {
            if (pkt.type == rdt::PacketType::kSyn) {
                client_addr = from;
                server_isn = static_cast<std::uint32_t>(Clock::now().time_since_epoch().count());
                rdt::Packet syn_ack{};
                syn_ack.type = rdt::PacketType::kSynAck;
                syn_ack.seq = server_isn;
                syn_ack.ack = pkt.seq + 1;
                syn_ack.window = opts.window;
                SendPacket(sock, client_addr, syn_ack);
                continue;
            }
            if (pkt.type == rdt::PacketType::kAck && pkt.ack == server_isn + 1) {
                state.handshake_done = true;
                state.start_time = Clock::now();
                std::cout << "[INFO] 握手完成，开始接收文件\n";
            }
            continue;
        }

        // 仅处理来自握手时记录的客户端
        if (from.sin_addr.s_addr != client_addr.sin_addr.s_addr ||
            from.sin_port != client_addr.sin_port) {
            continue;
        }

        if (pkt.type == rdt::PacketType::kMeta) {
            // META 必须按序到达
            if (pkt.seq == state.expected_seq && pkt.payload.size() >= 10) {
                std::uint64_t net_size = 0;
                std::memcpy(&net_size, pkt.payload.data(), 8);
                state.file_size = rdt::NetToHost64(net_size);
                std::uint16_t name_len = 0;
                std::memcpy(&name_len, pkt.payload.data() + 8, 2);
                name_len = ntohs(name_len);
                if (pkt.payload.size() == 10 + name_len) {
                    state.file_name.assign(reinterpret_cast<const char*>(pkt.payload.data() + 10),
                                           name_len);
                    EnsureOutDir(opts.out_dir);
                    const auto out_path = std::filesystem::path(opts.out_dir) / state.file_name;
                    state.file.open(out_path, std::ios::binary);
                    if (!state.file) {
                        std::cerr << "[ERROR] 无法创建输出文件: " << out_path.string() << "\n";
                        break;
                    }
                    std::cout << "[INFO] 接收文件 " << state.file_name << " 大小 "
                              << state.file_size << " 字节\n";
                    ++state.expected_seq;  // META 确认
                }
            }
        } else if (pkt.type == rdt::PacketType::kData) {
            // 判断是否在接收窗口内
            if (pkt.seq >= state.expected_seq &&
                pkt.seq < state.expected_seq + opts.window) {
                if (!state.buffer.count(pkt.seq)) {
                    state.buffer.emplace(pkt.seq, pkt.payload);
                    if (pkt.seq == state.expected_seq) {
                        FlushInOrder(state);
                    }
                }
            }
        } else if (pkt.type == rdt::PacketType::kFin) {
            state.fin_received = true;
            state.fin_seq = pkt.seq;
            if (pkt.seq == state.expected_seq) {
                ++state.expected_seq;
            }
            FlushInOrder(state);
        } else if (pkt.type == rdt::PacketType::kReset) {
            std::cerr << "[WARN] 收到对端复位，退出\n";
            break;
        }

        // 发送 ACK / FIN-ACK
        rdt::Packet ack{};
        ack.type = state.fin_received && state.expected_seq == state.fin_seq + 1
                       ? rdt::PacketType::kFinAck
                       : rdt::PacketType::kAck;
        ack.ack = state.expected_seq;
        ack.window = static_cast<std::uint16_t>(
            state.buffer.size() >= opts.window ? 0 : opts.window - state.buffer.size());
        ack.ack_bits = BuildAckBits(state);
        SendPacket(sock, client_addr, ack);
        state.last_ack_sent = Clock::now();

        if (state.fin_received && state.expected_seq == state.fin_seq + 1) {
            const auto end_time = Clock::now();
            const auto duration_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - state.start_time)
                    .count();
            const double seconds = duration_ms / 1000.0;
            const double throughput =
                seconds > 0 ? state.bytes_written * 8.0 / seconds / 1e6 : 0.0;
            std::cout << "[INFO] 接收完成，用时 " << duration_ms << " ms，平均吞吐率 "
                      << throughput << " Mbps" << std::endl;
            break;
        }
    }

    if (state.file.is_open()) {
        state.file.close();
    }
    closesocket(sock);
    WSACleanup();
    return 0;
}
