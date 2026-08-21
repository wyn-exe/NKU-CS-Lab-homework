#include "reliable_udp.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using socket_len_t = int;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_len_t = socklen_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
using SOCKET = int;
#endif

namespace rudp {

using std::size_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

namespace {

// 数据包头部，保持紧凑布局便于校验和计算
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t seq;       //序列号，用于标识数据包的顺序
    uint32_t ack;       //确认号，用于确认接收到的数据
    uint16_t len;
    uint16_t wnd;       //窗口大小
    uint16_t flags;     //标志位
    uint32_t sack;      //选择确认窗口
    uint16_t checksum;  //校验和
};
#pragma pack(pop)

// 数据包结构
struct Packet {
    PacketHeader header{};
    std::vector<uint8_t> payload;
};

// 发送槽
struct SendSlot {
    Packet pkt;
    std::chrono::steady_clock::time_point last_send;
    bool acked = false;
    int retry = 0;
};

// 接收槽
struct RecvSlot {
    std::vector<uint8_t> data;
    uint32_t seq = 0;   // 接收到的数据包的序列号
};

// 网络地址信息
struct Endpoint {
    sockaddr_storage addr{};
    socket_len_t len = 0;
};

// 确保指定的目录存在
bool ensure_directory(const std::string& path) {
    try {
        std::filesystem::path p(path);
        if (std::filesystem::exists(p)) return true;
        return std::filesystem::create_directories(p);
    } catch (...) {
        return false;
    }
}

#ifdef _WIN32
bool set_console_utf8() {
    return SetConsoleOutputCP(CP_UTF8) && SetConsoleCP(CP_UTF8);
}
#endif

// 生成当前毫秒时间戳
int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 简易日志，便于调试
void log(const std::string& tag, const std::string& msg) {
    std::cout << "[" << tag << "] " << msg << std::endl;
}

// 初始化 socket 库
bool init_socket_lib() {
#ifdef _WIN32
    static bool inited = false;
    if (inited) return true;  // 避免重复初始化
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
    inited = true;
#endif
    return true;
}

void close_socket(SOCKET s) {
#ifdef _WIN32
    closesocket(s);     // Windows 下关闭 socket
#else
    close(s);        // Unix/Linux 下关闭 socket 
#endif
}

// 16 位互联网校验和
uint16_t checksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
    // 循环累加每 16 位数据
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    // 处理剩余的单字节
    if (len == 1) {
        sum += *reinterpret_cast<const uint8_t*>(ptr);
    }
    // 处理进位，将高 16 位加到低 16 位
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    // 取反得到校验和
    return static_cast<uint16_t>(~sum);
}

// 序列化数据包（含校验和）
std::vector<uint8_t> serialize_packet(const Packet& pkt) {
    PacketHeader hdr_net{};
    hdr_net.seq = htonl(pkt.header.seq);
    hdr_net.ack = htonl(pkt.header.ack);
    hdr_net.len = htons(pkt.header.len);
    hdr_net.wnd = htons(pkt.header.wnd);
    hdr_net.flags = htons(pkt.header.flags);
    hdr_net.sack = htonl(pkt.header.sack);
    hdr_net.checksum = 0;

    std::vector<uint8_t> buffer(sizeof(PacketHeader) + pkt.payload.size());
    std::memcpy(buffer.data(), &hdr_net, sizeof(PacketHeader));
    if (!pkt.payload.empty()) {
        std::memcpy(buffer.data() + sizeof(PacketHeader),
                    pkt.payload.data(), pkt.payload.size());
    }
    uint16_t sum = checksum16(buffer.data(), buffer.size());
    reinterpret_cast<PacketHeader*>(buffer.data())->checksum = htons(sum);
    return buffer;
}

// 解析数据包并做校验和检查
bool parse_packet(const uint8_t* buf, size_t len, Packet& out) {
    if (len < sizeof(PacketHeader)) return false;
    std::vector<uint8_t> tmp(buf, buf + len);
    PacketHeader hdr_net{};
    std::memcpy(&hdr_net, tmp.data(), sizeof(PacketHeader));
    uint16_t recv_sum = ntohs(hdr_net.checksum);
    reinterpret_cast<PacketHeader*>(tmp.data())->checksum = 0;
    uint16_t calc_sum = checksum16(tmp.data(), tmp.size());
    if (recv_sum != calc_sum) return false;     // 校验和不匹配，返回 false

    out.header.seq = ntohl(hdr_net.seq);
    out.header.ack = ntohl(hdr_net.ack);
    out.header.len = ntohs(hdr_net.len);
    out.header.wnd = ntohs(hdr_net.wnd);
    out.header.flags = ntohs(hdr_net.flags);
    out.header.sack = ntohl(hdr_net.sack);
    out.header.checksum = recv_sum;
    out.payload.resize(out.header.len);
    if (out.header.len > 0 && len >= sizeof(PacketHeader) + out.header.len) {
        std::memcpy(out.payload.data(), buf + sizeof(PacketHeader),
                    out.header.len);
    }
    return true;
}

// 解析 IP 地址和端口，填充 Endpoint
bool resolve_address(const std::string& ip, uint16_t port, Endpoint& ep) {
    sockaddr_in addr4{};
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr4.sin_addr) != 1) {
        return false;
    }
    std::memcpy(&ep.addr, &addr4, sizeof(sockaddr_in));
    ep.len = sizeof(sockaddr_in);
    return true;
}

// 创建并绑定 UDP 套接字
SOCKET make_udp_socket(const std::string& bind_ip, uint16_t bind_port) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(bind_port);
    if (bind_ip.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, bind_ip.c_str(), &addr.sin_addr) != 1) {
            close_socket(s);
            return INVALID_SOCKET;
        }
    }
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        close_socket(s);
        return INVALID_SOCKET;
    }
    return s;
}

// 生成随机 ISN
uint32_t random_isn() {
    static std::mt19937 rng(
        static_cast<uint32_t>(std::chrono::high_resolution_clock::now()
                                  .time_since_epoch()
                                  .count()));
    return rng();
}

// 判断序列号 seq 是否在窗口内 [base, base + window*step)
bool in_window(uint32_t seq, uint32_t base, std::size_t window_packets,
               std::size_t payload_size) {
    uint32_t upper = base + static_cast<uint32_t>(window_packets * payload_size);
    return seq >= base && seq < upper;
}

}  // namespace

// ---------------- 发送端实现 ----------------

class Sender {
public:
    Sender(const Config& c,
           const std::string& local_ip,
           uint16_t local_port,
           const std::string& remote_ip,
           uint16_t remote_port)
        : cfg_(c),
          local_ip_(local_ip),
          local_port_(local_port),
          remote_ip_(remote_ip),
          remote_port_(remote_port) {}

    bool run(const std::string& file_path, TransferStats& stats);

private:
    bool handshake();
    bool send_meta(const std::string& name, uint64_t size);
    bool send_data(const std::vector<uint8_t>& data, TransferStats& stats);
    bool close_conn();

    bool recv_ack(Packet& out, int timeout_ms);
    void handle_ack(const Packet& ack_pkt, uint32_t& dup_ack);
    void retransmit_one(uint32_t seq);

    Config cfg_;
    std::string local_ip_;
    uint16_t local_port_;
    std::string remote_ip_;
    uint16_t remote_port_;
    SOCKET sock_{INVALID_SOCKET};
    Endpoint remote_{};

    uint32_t isn_local_{0};
    uint32_t isn_remote_{0};
    uint32_t send_base_{0};     // 最早未确认的序列号
    uint32_t next_seq_{0};      // 下一个可发送序列号
    uint32_t peer_expect_{0};   // 对端期望的 seq（用于 ACK 字段）
    uint32_t data_start_{0};    // 数据段起始序列号（剔除握手/元数据）
    uint16_t remote_wnd_{0};
    std::map<uint32_t, SendSlot> window_;
};

// 实现三次握手，建立可靠连接
bool Sender::handshake() {
    isn_local_ = random_isn(); //生成本地初始序列号ISN
    Packet syn{};              //构造 SYN 数据包
    syn.header.seq = isn_local_;
    syn.header.ack = 0;
    syn.header.flags = FLAG_SYN;
    syn.header.len = 0;
    syn.header.wnd = static_cast<uint16_t>(cfg_.window_packets);

    int retries = 0;
    // 循环发送 SYN 数据包，直到收到 SYN-ACK 或达到重试次数
    while (retries < cfg_.conn_retry) {
        auto buf = serialize_packet(syn);
        sendto(sock_, reinterpret_cast<const char*>(buf.data()),
               static_cast<int>(buf.size()), 0,
               reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
        log("HANDSHAKE", "发送 SYN");

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        timeval tv{1, 0};
        int r = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv); //等待远端响应
        if (r > 0 && FD_ISSET(sock_, &rfds)) {
            uint8_t buf_in[1500];
            sockaddr_storage from{};
            socket_len_t flen = sizeof(from);
            int n = recvfrom(sock_, reinterpret_cast<char*>(buf_in), sizeof(buf_in), 0,
                             reinterpret_cast<sockaddr*>(&from), &flen);
            if (n <= 0) {
                retries++;
                continue;
            }
            // 接收数据包并解析
            Packet pkt{};
            if (!parse_packet(buf_in, n, pkt)) continue;
            if ((pkt.header.flags & (FLAG_SYN | FLAG_ACK)) == (FLAG_SYN | FLAG_ACK) &&
                pkt.header.ack == isn_local_ + 1) {
                isn_remote_ = pkt.header.seq;
                remote_wnd_ = pkt.header.wnd;
                peer_expect_ = isn_remote_ + 1;
                Packet ack{};
                ack.header.seq = isn_local_ + 1;
                ack.header.ack = peer_expect_;
                ack.header.flags = FLAG_ACK;
                ack.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
                auto out = serialize_packet(ack);
                sendto(sock_, reinterpret_cast<const char*>(out.data()),
                       static_cast<int>(out.size()), 0,
                       reinterpret_cast<sockaddr*>(&from), flen);
                log("HANDSHAKE", "握手完成");
                send_base_ = next_seq_ = isn_local_ + 1;
                return true;
            }
        }
        retries++;
    }
    return false;
}

// 发送文件元数据
bool Sender::send_meta(const std::string& name, uint64_t size) {
    std::ostringstream oss;
    oss << name << "|" << size;
    std::string meta = oss.str();
    Packet pkt{};
    pkt.header.seq = next_seq_;
    pkt.header.ack = peer_expect_;
    pkt.header.flags = FLAG_ACK | FLAG_META;
    pkt.header.len = static_cast<uint16_t>(meta.size());
    pkt.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
    pkt.payload.assign(meta.begin(), meta.end());

    auto buf = serialize_packet(pkt);
    sendto(sock_, reinterpret_cast<const char*>(buf.data()),
           static_cast<int>(buf.size()), 0,
           reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
    log("META", "发送文件信息");

    // 等待确认
    for (int i = 0; i < cfg_.conn_retry; ++i) {
        Packet ack{};
        if (recv_ack(ack, cfg_.rto_ms)) {
            if ((ack.header.flags & FLAG_ACK) && ack.header.ack >= next_seq_ + meta.size()) {
                send_base_ = next_seq_ = ack.header.ack;
                data_start_ = send_base_;
                window_.clear();
                return true;
            }
        }
        // 超时重传元数据
        sendto(sock_, reinterpret_cast<const char*>(buf.data()),
               static_cast<int>(buf.size()), 0,
               reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
    }
    return false;
}

bool Sender::recv_ack(Packet& out, int timeout_ms) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock_, &rfds);
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv);
    if (r <= 0 || !FD_ISSET(sock_, &rfds)) return false;
    uint8_t buf[1500];
    sockaddr_storage from{};
    socket_len_t flen = sizeof(from);
    int n = recvfrom(sock_, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                     reinterpret_cast<sockaddr*>(&from), &flen);
    if (n <= 0) return false;
    return parse_packet(buf, n, out);
}

void Sender::handle_ack(const Packet& ack_pkt, uint32_t& dup_ack) {
    if (!(ack_pkt.header.flags & FLAG_ACK)) return;
    remote_wnd_ = ack_pkt.header.wnd;
    uint32_t ack_num = ack_pkt.header.ack;
    if (ack_num > send_base_) {
        // 新的累计确认，窗口前移
        std::vector<uint32_t> to_erase;
        for (auto& [seq, slot] : window_) {
            if (seq + slot.pkt.header.len <= ack_num) {
                slot.acked = true;
                to_erase.push_back(seq);
            }
        }
        for (auto s : to_erase) window_.erase(s);
        send_base_ = ack_num;
        dup_ack = 0;
    } else if (ack_num == send_base_) {
        dup_ack++;
    }

    // SACK 处理
    uint32_t base = ack_num;
    for (int i = 0; i < 32; ++i) {
        if (ack_pkt.header.sack & (1u << i)) {
            uint32_t seq = base + static_cast<uint32_t>((i + 1) * cfg_.payload_size);
            auto it = window_.find(seq);
            if (it != window_.end()) {
                it->second.acked = true;
            }
        }
    }
}

void Sender::retransmit_one(uint32_t seq) {
    auto it = window_.find(seq);
    if (it == window_.end()) return;
    it->second.retry++;
    it->second.last_send = std::chrono::steady_clock::now();
    auto buf = serialize_packet(it->second.pkt);
    sendto(sock_, reinterpret_cast<const char*>(buf.data()),
           static_cast<int>(buf.size()), 0,
           reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
}

// 发送文件数据（拥塞控制和超时重传）
bool Sender::send_data(const std::vector<uint8_t>& data, TransferStats& stats) {
    const std::size_t total = data.size();
    const uint32_t data_end = data_start_ + static_cast<uint32_t>(total);
    // 初始化拥塞窗口和慢启动阈值
    double cwnd = 1.0;
    double ssthresh = static_cast<double>(cfg_.window_packets);
    uint32_t dup_ack = 0;
    bool in_fast_recovery = false;  // 标记是否处于快速恢复阶段
    auto start_time = std::chrono::steady_clock::now();

    // 循环发送数据，直到所有数据被确认
    while (send_base_ < data_end || !window_.empty()) {
        // 计算允许发送的分片数量
        std::size_t inflight = window_.size();
        std::size_t allowed_remote = remote_wnd_ == 0 ? 1 : remote_wnd_;
        std::size_t allowed = std::max<std::size_t>(
            1, std::min<std::size_t>(
                   {cfg_.window_packets,
                    static_cast<std::size_t>(std::floor(cwnd)),
                    allowed_remote}));
        while (inflight < allowed && next_seq_ < data_end) {
            std::size_t offset = static_cast<std::size_t>(next_seq_ - data_start_);
            std::size_t len = std::min(cfg_.payload_size, total - offset);
            Packet pkt{};
            pkt.header.seq = next_seq_;
            pkt.header.ack = peer_expect_;
            pkt.header.flags = FLAG_ACK;
            pkt.header.len = static_cast<uint16_t>(len);
            pkt.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
            pkt.payload.insert(pkt.payload.end(),
                               data.begin() + offset,
                               data.begin() + offset + len);
            auto buf = serialize_packet(pkt);
            sendto(sock_, reinterpret_cast<const char*>(buf.data()),
                   static_cast<int>(buf.size()), 0,
                   reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);

            SendSlot slot{pkt, std::chrono::steady_clock::now()};
            window_[pkt.header.seq] = slot;
            next_seq_ += static_cast<uint32_t>(len);
            inflight++;
        }

        // 监听 ACK 并更新拥塞窗口
        Packet ack{};
        if (recv_ack(ack, cfg_.rto_ms)) {
            handle_ack(ack, dup_ack);
            // 拥塞控制：Reno
            if (ack.header.ack > send_base_) {
                // 收到新的 ACK，退出快速恢复
                if (in_fast_recovery) {
                    cwnd = ssthresh;  // 恢复到慢启动阈值
                    in_fast_recovery = false;
                }
                if (cwnd < ssthresh) {
                    cwnd++;  // 慢启动
                } else {
                    cwnd += 1.0 / cwnd;  // 拥塞避免
                }
            } else if (dup_ack >= 3) {
                if (!in_fast_recovery) {
                    // 进入快速恢复
                    ssthresh = std::max(2.0, cwnd / 2.0);
                    cwnd = ssthresh + 3.0;
                    retransmit_one(send_base_);  // 快速重传
                    in_fast_recovery = true;
                } else {
                    // 快速恢复阶段，增加拥塞窗口
                    cwnd += 1.0;
                }
            }
        }

        // 超时处理
        auto now = std::chrono::steady_clock::now();
        for (auto& [seq, slot] : window_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - slot.last_send).count();
            if (elapsed > cfg_.rto_ms && !slot.acked &&
                slot.retry < cfg_.max_retransmit) {
                ssthresh = std::max(2.0, cwnd / 2.0);
                cwnd = 1.0;
                dup_ack = 0;
                in_fast_recovery = false;
                retransmit_one(seq);
            }
        }

        // 若所有数据已被 ACK，退出
        if (window_.empty() && send_base_ >= data_end) {
            break;
        }
    }

    auto end = std::chrono::steady_clock::now();
    stats.bytes = total;
    stats.duration_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time).count());
    stats.throughput_mbps = (total * 8.0 / 1'000'000) /
                            (stats.duration_ms / 1000.0 + 1e-6);
    return true;
}

// 实现四次挥手关闭连接
bool Sender::close_conn() {
    Packet fin{};
    fin.header.seq = next_seq_;
    fin.header.ack = peer_expect_;
    fin.header.flags = FLAG_FIN | FLAG_ACK;
    fin.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
    auto buf = serialize_packet(fin);
    sendto(sock_, reinterpret_cast<const char*>(buf.data()),
           static_cast<int>(buf.size()), 0,
           reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
    log("FIN", "发送 FIN");

    int timeout_ms = std::max(cfg_.rto_ms, 800);
    int max_retry = cfg_.conn_retry * 2 + 2;
    for (int i = 0; i < max_retry; ++i) {
        Packet ack{};
        if (recv_ack(ack, timeout_ms)) {
            if ((ack.header.flags & FLAG_ACK) && ack.header.ack >= fin.header.seq + 1) {
                // 对端 FIN
                if (ack.header.flags & FLAG_FIN) {
                    Packet last_ack{};
                    last_ack.header.seq = fin.header.seq + 1;
                    last_ack.header.ack = ack.header.seq + 1;
                    last_ack.header.flags = FLAG_ACK;
                    last_ack.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
                    auto out = serialize_packet(last_ack);
                    sendto(sock_, reinterpret_cast<const char*>(out.data()),
                           static_cast<int>(out.size()), 0,
                           reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
                }
                return true;
            }
        }
        sendto(sock_, reinterpret_cast<const char*>(buf.data()),
               static_cast<int>(buf.size()), 0,
               reinterpret_cast<sockaddr*>(&remote_.addr), remote_.len);
    }
    log("FIN", "FIN 确认，关闭连接");
    return true;
}

// 运行发送端的主逻辑，包括初始化、握手、发送文件元数据和数据、关闭连接
bool Sender::run(const std::string& file_path, TransferStats& stats) {
    if (!init_socket_lib()) {
        log("ERROR", "初始化 socket 失败");
        return false;
    }
    Endpoint ep{};
    if (!resolve_address(remote_ip_, remote_port_, ep)) {
        log("ERROR", "远端地址解析失败");
        return false;
    }
    remote_ = ep;
    remote_wnd_ = static_cast<uint16_t>(cfg_.window_packets);
    sock_ = make_udp_socket(local_ip_, local_port_);
    if (sock_ == INVALID_SOCKET) {
        log("ERROR", "创建/绑定 socket 失败");
        return false;
    }

    if (!handshake()) {
        log("ERROR", "三次握手失败");
        close_socket(sock_);
        return false;
    }

    // 读取文件
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        log("ERROR", "无法打开文件");
        return false;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());

    // 发送元数据
    std::string filename = file_path;
    auto pos = file_path.find_last_of("/\\");
    if (pos != std::string::npos) filename = file_path.substr(pos + 1);
    if (!send_meta(filename, data.size())) {
        log("ERROR", "元数据传输失败");
        return false;
    }

    if (!send_data(data, stats)) {
        log("ERROR", "数据传输失败");
        return false;
    }

    if (!close_conn()) {
        log("WARN", "连接关闭未确认");
    }
    close_socket(sock_);
    return true;
}

// ---------------- 接收端实现 ----------------

class Receiver {
public:
    Receiver(const Config& c,
             const std::string& ip,
             uint16_t port,
             const std::string& out_dir)
        : cfg_(c), listen_ip_(ip), listen_port_(port), out_dir_(out_dir) {}

    bool run(TransferStats& stats);

private:
    bool handshake();
    void send_ack(uint32_t ack_num, uint32_t sack_mask, uint16_t flags);
    bool handle_packet(const Packet& pkt, TransferStats& stats, bool& fin_seen);
    void reset_state();
    bool close_conn();

    Config cfg_;
    std::string listen_ip_;
    uint16_t listen_port_;
    std::string out_dir_;
    SOCKET sock_{INVALID_SOCKET};
    Endpoint peer_{};

    uint32_t isn_local_{0};
    uint32_t peer_isn_{0};
    uint32_t expect_seq_{0};
    uint32_t data_start_seq_{0};
    uint64_t received_data_{0};
    bool meta_received_{false};
    bool fin_seen_{false};
    uint32_t fin_ack_num_{0};
    std::string filename_;
    uint64_t expected_size_{0};
    std::ofstream out_;
    std::map<uint32_t, RecvSlot> buffer_;
    std::string out_dir_real_;
};

// 实现三次握手，建立可靠连接
bool Receiver::handshake() {
    fd_set rfds;
    while (true) {
        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        timeval tv{1, 0};
        int r = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv); // 等待数据到达
        // 接收数据包并解析
        if (r > 0 && FD_ISSET(sock_, &rfds)) {
            uint8_t buf[1500];
            sockaddr_storage from{};
            socket_len_t flen = sizeof(from);
            int n = recvfrom(sock_, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                             reinterpret_cast<sockaddr*>(&from), &flen);
            if (n <= 0) continue;
            Packet syn{};
            if (!parse_packet(buf, n, syn)) continue;
            // 如果收到 SYN 包，记录对端地址和初始序列号
            if (syn.header.flags & FLAG_SYN) {
                peer_ = {from, flen};
                peer_isn_ = syn.header.seq;
                expect_seq_ = peer_isn_ + 1;
                isn_local_ = random_isn();
                // 生成本地初始序列号，构造 SYN-ACK 包并发送
                Packet synack{};
                synack.header.seq = isn_local_;
                synack.header.ack = expect_seq_;
                synack.header.flags = FLAG_SYN | FLAG_ACK;
                synack.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
                auto out = serialize_packet(synack);
                sendto(sock_, reinterpret_cast<const char*>(out.data()),
                       static_cast<int>(out.size()), 0,
                       reinterpret_cast<sockaddr*>(&peer_.addr), peer_.len);
                log("HANDSHAKE", "发送 SYN-ACK");
                // 记录输出目录，确保存在
                out_dir_real_ = out_dir_;
                if (!out_dir_real_.empty()) {
                    ensure_directory(out_dir_real_);
                }

                // 等待最终 ACK，完成握手
                for (int i = 0; i < cfg_.conn_retry; ++i) {
                    FD_ZERO(&rfds);
                    FD_SET(sock_, &rfds);
                    timeval tv2{1, 0};
                    int rr = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv2);
                    if (rr > 0 && FD_ISSET(sock_, &rfds)) {
                        int m = recvfrom(sock_, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                                         reinterpret_cast<sockaddr*>(&from), &flen);
                        if (m <= 0) continue;
                        Packet ack{};
                        if (!parse_packet(buf, m, ack)) continue;
                        if ((ack.header.flags & FLAG_ACK) &&
                            ack.header.ack == isn_local_ + 1) {
                            log("HANDSHAKE", "握手完成");
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

// 重置接收端的状态
void Receiver::reset_state() {
    peer_isn_ = 0;
    expect_seq_ = 0;
    data_start_seq_ = 0;
    received_data_ = 0;
    meta_received_ = false;
    fin_seen_ = false;
    fin_ack_num_ = 0;
    filename_.clear();
    expected_size_ = 0;
    buffer_.clear();
    if (out_.is_open()) out_.close();
}

// 发送 ACK 确认包
void Receiver::send_ack(uint32_t ack_num, uint32_t sack_mask, uint16_t flags) {
    Packet ack{};
    ack.header.seq = isn_local_ + 1;  // 单向数据，保持固定
    ack.header.ack = ack_num;
    ack.header.flags = static_cast<uint16_t>(FLAG_ACK | flags);
    ack.header.sack = sack_mask;
    // 计算接收窗口剩余空间
    int free_slots =
        static_cast<int>(cfg_.window_packets) - static_cast<int>(buffer_.size());
    if (free_slots < 0) free_slots = 0;
    ack.header.wnd = static_cast<uint16_t>(free_slots); //将剩余空间设置为wnd字段
    // 序列化数据包并发送
    auto out = serialize_packet(ack);
    sendto(sock_, reinterpret_cast<const char*>(out.data()),
           static_cast<int>(out.size()), 0,
           reinterpret_cast<sockaddr*>(&peer_.addr), peer_.len);
}

// 处理接收到的数据包，包括元数据、数据和 FIN
bool Receiver::handle_packet(const Packet& pkt, TransferStats& stats, bool& fin_seen) {
    // 元数据:解析文件名和大小，创建输出文件
    if (pkt.header.flags & FLAG_META) {
        if (meta_received_) return true;
        std::string meta(pkt.payload.begin(), pkt.payload.end());
        auto pos = meta.find('|');
        if (pos == std::string::npos) return false;
        filename_ = meta.substr(0, pos);
        expected_size_ = std::stoull(meta.substr(pos + 1));
        std::filesystem::path p = std::filesystem::path(out_dir_real_) / filename_;
        ensure_directory(std::filesystem::path(out_dir_real_).string());
        out_.open(p, std::ios::binary);
        if (!out_) {
            log("ERROR", "无法创建输出文件");
            return false;
        }
        meta_received_ = true;
        data_start_seq_ = pkt.header.seq + pkt.header.len;
        expect_seq_ = data_start_seq_;
        received_data_ = 0;
        send_ack(expect_seq_, 0, 0);
        return true;
    }

    // FIN 处理:记录 FIN 确认号并发送 FIN-ACK
    if (pkt.header.flags & FLAG_FIN) {
        fin_ack_num_ = pkt.header.seq + 1;
        fin_seen_ = true;

        send_ack(fin_ack_num_, 0, FLAG_FIN);
        stats.bytes = received_data_;
        fin_seen = true;
        return false;
    }

    // 数据接收:检查序列号是否在窗口内
    if (pkt.header.len > 0) {
        if (!in_window(pkt.header.seq, expect_seq_, cfg_.window_packets,
                       cfg_.payload_size)) {
            send_ack(expect_seq_, 0, 0);
            return true;
        }
        if (pkt.header.seq == expect_seq_) {  // 如果是期望的序列号，写入文件并更新状态
            out_.write(reinterpret_cast<const char*>(pkt.payload.data()),
                       pkt.payload.size());
            received_data_ += pkt.header.len;
            expect_seq_ += pkt.header.len;
            // 处理 buffer 中后续连续块
            bool advanced = true;
            while (advanced) {
                advanced = false;
                auto it = buffer_.find(expect_seq_);
                if (it != buffer_.end()) {
                    out_.write(reinterpret_cast<const char*>(it->second.data.data()),
                               it->second.data.size());
                    received_data_ += static_cast<uint32_t>(it->second.data.size());
                    expect_seq_ += static_cast<uint32_t>(it->second.data.size());
                    buffer_.erase(it);
                    advanced = true;
                }
            }
        } else if (pkt.header.seq > expect_seq_) { // 如果是乱序包，存入缓冲区
            if (buffer_.size() < cfg_.window_packets &&
                buffer_.find(pkt.header.seq) == buffer_.end()) {
                buffer_[pkt.header.seq] = RecvSlot{pkt.payload, pkt.header.seq};
            }
        }

        // 构造 SACK 掩码并发送 ACK
        uint32_t sack = 0;
        for (const auto& [seq, slot] : buffer_) {
            if (seq > expect_seq_) {
                uint32_t delta = seq - expect_seq_;
                if (delta >= cfg_.payload_size) {
                    uint32_t idx = delta / static_cast<uint32_t>(cfg_.payload_size);
                    if (idx > 0 && idx <= 32) {
                        sack |= (1u << (idx - 1));
                    }
                }
            }
        }
        send_ack(expect_seq_, sack, 0);

        if (expected_size_ > 0 && received_data_ >= expected_size_) {
            stats.bytes = expected_size_;
            return true;
        }
    }
    return true;
}

// 实现挥手关闭连接
bool Receiver::close_conn() {
    log("FIN", "收到 FIN，开始关闭连接");

    // 发送 ACK 确认 FIN
    send_ack(fin_ack_num_, 0, FLAG_ACK);
    log("FIN", "发送 ACK 确认 FIN");

    // 构造 FIN 数据包
    Packet fin{};
    fin.header.seq = expect_seq_;
    fin.header.ack = fin_ack_num_;
    fin.header.flags = FLAG_FIN;
    fin.header.wnd = static_cast<uint16_t>(cfg_.window_packets);
    auto buf = serialize_packet(fin);
    sendto(sock_, reinterpret_cast<const char*>(buf.data()),
           static_cast<int>(buf.size()), 0,
           reinterpret_cast<sockaddr*>(&peer_.addr), peer_.len);
    log("FIN", "发送 FIN");

    // 等待发送端的最后一个 ACK
    fd_set rfds;
    for (int i = 0; i < cfg_.conn_retry; ++i) {
        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        timeval tv{1, 0};
        int r = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv);
        if (r > 0 && FD_ISSET(sock_, &rfds)) {
            uint8_t buf_in[1500];
            sockaddr_storage from{};
            socket_len_t flen = sizeof(from);
            int n = recvfrom(sock_, reinterpret_cast<char*>(buf_in), sizeof(buf_in), 0,
                             reinterpret_cast<sockaddr*>(&from), &flen);
            if (n <= 0) continue;

            Packet ack{};
            if (!parse_packet(buf_in, n, ack)) continue;

            // 检查是否是最后的 ACK
            if ((ack.header.flags & FLAG_ACK) && ack.header.ack == fin.header.seq + 1) {
                log("FIN", "收到最后的 ACK，连接关闭完成");
                return true;
            }
        }
    }

    log("FIN", "未收到最后的 ACK，连接关闭未确认");
    return false;
}

// 运行接收端的主逻辑，包括初始化、握手、接收数据和关闭连接
bool Receiver::run(TransferStats& stats) {
    if (!init_socket_lib()) {
        log("ERROR", "初始化 socket 失败");
        return false;
    }
    sock_ = make_udp_socket(listen_ip_, listen_port_);
    if (sock_ == INVALID_SOCKET) {
        log("ERROR", "绑定端口失败");
        return false;
    }
    // 循环等待新连接
    while (true) {
        reset_state();
        // 握手
        if (!handshake()) {
            log("ERROR", "握手失败");
            break;
        }

        auto start = std::chrono::steady_clock::now();
        fd_set rfds;
        bool fin_seen = false;
        // 接收数据包并调用 handle_packet 方法处理
        while (true) {
            FD_ZERO(&rfds);
            FD_SET(sock_, &rfds);
            timeval tv{1, 0};
            int r = select(static_cast<int>(sock_ + 1), &rfds, nullptr, nullptr, &tv);
            if (r > 0 && FD_ISSET(sock_, &rfds)) {
                uint8_t buf[2000];
                sockaddr_storage from{};
                socket_len_t flen = sizeof(from);
                int n = recvfrom(sock_, reinterpret_cast<char*>(buf), sizeof(buf), 0,
                                 reinterpret_cast<sockaddr*>(&from), &flen);
                if (n <= 0) continue;
                Packet pkt{};
                if (!parse_packet(buf, n, pkt)) continue;
                if (!handle_packet(pkt, stats, fin_seen)) break;
                if (expected_size_ > 0 && received_data_ >= expected_size_) {
                    fin_seen = true;
                    break;
                }
            }
            if (fin_seen) break;
        }
        auto end = std::chrono::steady_clock::now();
        // 记录传输统计信息
        if (stats.bytes == 0) {
            stats.bytes = received_data_;
        }
        stats.duration_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        stats.throughput_mbps = (stats.bytes * 8.0 / 1'000'000) /
                                (stats.duration_ms / 1000.0 + 1e-6);

        // 确保发送端收到最终 ACK
        if (fin_seen_ && fin_ack_num_ > 0) {
            for (int i = 0; i < 3; ++i) {
                send_ack(fin_ack_num_, 0, 0);
                sleep_ms(50);
            }
        }

        // 在接收数据包的循环结束后，调用 close_conn
        if (fin_seen_) {
            if (!close_conn()) {
                log("WARN", "连接关闭未确认");
            }
        }

        // 进入下一轮等待新连接
    }
    close_socket(sock_);
    return true;
}

// ---------------- 公共接口 ----------------

// 发送端入口函数，初始化发送端并启动文件传输
bool run_sender(const std::string& local_ip,
                uint16_t local_port,
                const std::string& remote_ip,
                uint16_t remote_port,
                const std::string& file_path,
                const Config& cfg,
                TransferStats& stats) {
#ifdef _WIN32
    set_console_utf8();
#endif
    Sender sender(cfg, local_ip, local_port, remote_ip, remote_port);
    return sender.run(file_path, stats);
}

// 接收端入口函数，初始化接收端并启动文件接收
bool run_receiver(const std::string& listen_ip,
                  uint16_t listen_port,
                  const std::string& output_dir,
                  const Config& cfg,
                  TransferStats& stats) {
#ifdef _WIN32
    set_console_utf8();
#endif
    Receiver receiver(cfg, listen_ip, listen_port, output_dir);
    return receiver.run(stats);
}

}  // namespace rudp
