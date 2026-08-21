#include "reliable_udp.h"

#include <iostream>
#include <string>

// 发送端入口：rudp_sender <peer_ip> <peer_port> <file_path> [--local-ip IP] [--local-port PORT] [--wnd N] [--payload BYTES] [--rto MS]
int main(int argc, char* argv[]) {
    using namespace rudp;
    if (argc < 4) {
        std::cout << "用法: rudp_sender <peer_ip> <peer_port> <file_path> "
                     "[--local-ip IP] [--local-port PORT] [--wnd N] [--payload BYTES] [--rto MS]\n";
        return 1;
    }

    std::string peer_ip = argv[1];
    uint16_t peer_port = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string file_path = argv[3];

    std::string local_ip = "0.0.0.0";
    uint16_t local_port = 0;
    Config cfg;

    for (int i = 4; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--local-ip" && i + 1 < argc) {
            local_ip = argv[++i];
        } else if (arg == "--local-port" && i + 1 < argc) {
            local_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--wnd" && i + 1 < argc) {
            cfg.window_packets = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--payload" && i + 1 < argc) {
            cfg.payload_size = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--rto" && i + 1 < argc) {
            cfg.rto_ms = std::stoi(argv[++i]);
        }
    }

    TransferStats stats;
    bool ok = run_sender(local_ip, local_port, peer_ip, peer_port, file_path, cfg, stats);
    if (!ok) {
        std::cerr << "发送失败\n";
        return 1;
    }

    std::cout << "发送完成: 传输字节 " << stats.bytes
              << ", 耗时 " << stats.duration_ms << " ms"
              << ", 吞吐率 " << stats.throughput_mbps << " Mbps\n";
    return 0;
}
