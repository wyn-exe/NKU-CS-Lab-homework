#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace chat {

// 协议常量
constexpr const char* kProtocolVersion = "CHAT/1.0";
constexpr std::size_t kMaxUsernameLength = 32;
constexpr std::size_t kMinUsernameLength = 2;
constexpr std::size_t kMaxPayloadLength = 4096;

// 基础工具
void SetupConsoleUtf8();
std::string CurrentIsoTimestamp();

// 发送/接收原语
bool SendAll(SOCKET socket, const char* data, std::size_t length);
inline bool SendAll(SOCKET socket, const std::string& data) {
    return SendAll(socket, data.data(), data.size());
}
bool SendLine(SOCKET socket, const std::string& line);
bool ReceiveLine(SOCKET socket, std::string& buffer, std::string& line);
bool ReceiveBytes(SOCKET socket, std::string& buffer, std::size_t bytes,
                  std::string& payload);

// 辅助解析工具
std::vector<std::string> SplitTokens(const std::string& input, std::size_t max_tokens);
bool ParseLength(const std::string& token, std::size_t& value);
std::string NormalizeUsername(const std::string& raw);

}  // namespace chat
