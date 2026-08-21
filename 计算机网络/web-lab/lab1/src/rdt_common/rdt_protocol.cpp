#include "rdt_common/rdt_protocol.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace rdt {

namespace {

// 将 16 位值按一补加法累加到 sum
void AddWord(std::uint32_t& sum, std::uint16_t value) {
    sum += value;
    if (sum & 0x10000) {
        sum = (sum & 0xFFFF) + 1;
    }
}

}  // namespace

std::uint16_t ComputeChecksum(const std::uint8_t* data, std::size_t size) {
    std::uint32_t sum = 0;
    const std::size_t pairs = size / 2;
    for (std::size_t i = 0; i < pairs; ++i) {
        const std::uint16_t word = static_cast<std::uint16_t>(data[2 * i] << 8 | data[2 * i + 1]);
        AddWord(sum, word);
    }
    if (size % 2 == 1) {
        const std::uint16_t last = static_cast<std::uint16_t>(data[size - 1] << 8);
        AddWord(sum, last);
    }
    return static_cast<std::uint16_t>(~sum);
}

std::vector<std::uint8_t> SerializePacket(const Packet& packet) {
    WireHeader header{};
    header.type = static_cast<std::uint8_t>(packet.type);
    header.flags = packet.flags;
    header.length = htons(packet.length);
    header.seq = htonl(packet.seq);
    header.ack = htonl(packet.ack);
    header.ack_bits = htonl(packet.ack_bits);
    header.window = htons(packet.window);
    header.checksum = 0;

    std::vector<std::uint8_t> buffer;
    buffer.resize(sizeof(WireHeader) + packet.payload.size());
    std::memcpy(buffer.data(), &header, sizeof(WireHeader));
    if (!packet.payload.empty()) {
        std::memcpy(buffer.data() + sizeof(WireHeader), packet.payload.data(), packet.payload.size());
    }

    const std::uint16_t checksum = ComputeChecksum(buffer.data(), buffer.size());
    const std::uint16_t checksum_net = htons(checksum);  // 校验和按网络序写入
    std::memcpy(buffer.data() + offsetof(WireHeader, checksum), &checksum_net, sizeof(checksum_net));
    return buffer;
}

bool ParsePacket(const std::uint8_t* data, std::size_t size, Packet& out_packet) {
    if (size < sizeof(WireHeader)) {
        return false;
    }
    // 校验和检测：直接对收到的网络序缓冲区求和，结果应为 0xFFFF
    const std::uint16_t checksum = ComputeChecksum(data, size);
    if (checksum != 0xFFFF) {
        return false;
    }

    WireHeader header{};
    std::memcpy(&header, data, sizeof(WireHeader));

    const std::size_t payload_len = ntohs(header.length);
    if (payload_len + sizeof(WireHeader) != size) {
        return false;
    }

    out_packet.type = static_cast<PacketType>(header.type);
    out_packet.flags = header.flags;
    out_packet.length = static_cast<std::uint16_t>(payload_len);
    out_packet.seq = ntohl(header.seq);
    out_packet.ack = ntohl(header.ack);
    out_packet.ack_bits = ntohl(header.ack_bits);
    out_packet.window = ntohs(header.window);
    out_packet.checksum = ntohs(header.checksum);
    out_packet.payload.assign(data + sizeof(WireHeader), data + size);
    return true;
}

std::string PacketTypeName(PacketType type) {
    switch (type) {
        case PacketType::kSyn:
            return "SYN";
        case PacketType::kSynAck:
            return "SYN-ACK";
        case PacketType::kData:
            return "DATA";
        case PacketType::kAck:
            return "ACK";
        case PacketType::kFin:
            return "FIN";
        case PacketType::kFinAck:
            return "FIN-ACK";
        case PacketType::kReset:
            return "RST";
        case PacketType::kMeta:
            return "META";
        default:
            return "UNKNOWN";
    }
}

std::uint64_t HostToNet64(std::uint64_t value) {
    const std::uint32_t high = htonl(static_cast<std::uint32_t>(value >> 32));
    const std::uint32_t low = htonl(static_cast<std::uint32_t>(value & 0xFFFFFFFFULL));
    return (static_cast<std::uint64_t>(low) << 32) | high;
}

std::uint64_t NetToHost64(std::uint64_t value) {
    const std::uint32_t low = ntohl(static_cast<std::uint32_t>(value >> 32));
    const std::uint32_t high = ntohl(static_cast<std::uint32_t>(value & 0xFFFFFFFFULL));
    return (static_cast<std::uint64_t>(high) << 32) | low;
}

}  // namespace rdt
