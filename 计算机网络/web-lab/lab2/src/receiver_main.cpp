#include "reliable_udp.h"

#include <iostream>
#include <string>

// 接收端入口：rudp_receiver <listen_ip> <listen_port> <output_dir> [--wnd N] [--payload BYTES]
int main(int argc, char* argv[]) {
    using namespace rudp;
    if (argc < 4) {
        std::cout << "用法: rudp_receiver <listen_ip> <listen_port> <output_dir> "
                     "[--wnd N] [--payload BYTES]\n";
        return 1;
    }
    std::string listen_ip = argv[1];
    uint16_t listen_port = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string output_dir = argv[3];

    Config cfg;
    for (int i = 4; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--wnd" && i + 1 < argc) {
            cfg.window_packets = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--payload" && i + 1 < argc) {
            cfg.payload_size = static_cast<std::size_t>(std::stoul(argv[++i]));
        }
    }

    TransferStats stats;
    bool ok = run_receiver(listen_ip, listen_port, output_dir, cfg, stats);
    if (!ok) {
        std::cerr << "接收失败\n";
        return 1;
    }

    std::cout << "接收完成: 传输字节 " << stats.bytes
              << ", 耗时 " << stats.duration_ms << " ms"
              << ", 吞吐率 " << stats.throughput_mbps << " Mbps\n";
    return 0;
}
