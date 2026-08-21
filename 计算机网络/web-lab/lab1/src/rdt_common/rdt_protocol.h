#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace rdt {

// Packet types
enum class PacketType : std::uint8_t {
    kSyn = 1,     // SYN
    kSynAck = 2,  // SYN-ACK
    kData = 3,    // Data
    kAck = 4,     // Pure ACK (with SACK bits)
    kFin = 5,     // FIN
    kFinAck = 6,  // FIN-ACK
    kReset = 7,   // Reset
    kMeta = 8     // File metadata
};

// Constants
constexpr std::size_t kMaxPayloadSize = 1000;
constexpr std::size_t kAckBitmapBits = 32;
constexpr std::uint16_t kDefaultWindowSize = 32;
constexpr std::size_t kHeaderSize = 20;

// Wire header (network byte order)
#pragma pack(push, 1)
struct WireHeader {
    std::uint8_t type;
    std::uint8_t flags;
    std::uint16_t length;
    std::uint32_t seq;
    std::uint32_t ack;
    std::uint32_t ack_bits;
    std::uint16_t window;
    std::uint16_t checksum;
};
#pragma pack(pop)

struct Packet {
    PacketType type = PacketType::kData;
    std::uint8_t flags = 0;
    std::uint16_t length = 0;
    std::uint32_t seq = 0;
    std::uint32_t ack = 0;
    std::uint32_t ack_bits = 0;
    std::uint16_t window = 0;
    std::uint16_t checksum = 0;
    std::vector<std::uint8_t> payload;
};

// Checksum (16-bit one's complement)
std::uint16_t ComputeChecksum(const std::uint8_t* data, std::size_t size);

// Serialize host-order packet to wire buffer
std::vector<std::uint8_t> SerializePacket(const Packet& packet);

// Parse wire buffer and verify checksum
bool ParsePacket(const std::uint8_t* data, std::size_t size, Packet& out_packet);

// Pretty-print type
std::string PacketTypeName(PacketType type);

// 64-bit host/network converters
std::uint64_t HostToNet64(std::uint64_t value);
std::uint64_t NetToHost64(std::uint64_t value);

}  // namespace rdt
