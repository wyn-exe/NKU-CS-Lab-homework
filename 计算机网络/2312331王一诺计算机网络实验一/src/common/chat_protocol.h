#pragma once

#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace chat {

constexpr const char* kProtocolVersion = "CHAT/1.0";
constexpr std::size_t kMaxUsernameLength = 32;
constexpr std::size_t kMinUsernameLength = 2;
constexpr std::size_t kMaxPayloadLength = 4096;

void SetupConsoleUtf8();  //设置控制台的字符编码为 UTF-8
std::string CurrentIsoTimestamp();  //返回当前时间的 ISO 8601 格式字符串

bool SendAll(SOCKET socket, const char* data, std::size_t length);  //将指定长度的字节数据发送到指定的套接字
inline bool SendAll(SOCKET socket, const std::string& data) {  //发送字符串数据
    return SendAll(socket, data.data(), data.size());
}  

bool SendLine(SOCKET socket, const std::string& line);  //发送一行字符串数据
bool ReceiveLine(SOCKET socket, std::string& buffer, std::string& line); //从套接字接收一行数据，存储到 line 中
bool ReceiveBytes(SOCKET socket, std::string& buffer, std::size_t bytes, std::string& payload);  //从套接字接收指定字节数的数据，存储到 payload 中

std::vector<std::string> SplitTokens(const std::string& input, std::size_t max_tokens);  //将输入字符串按空格分割成多个token，最多 max_tokens 个
bool ParseLength(const std::string& token, std::size_t& value);  //将字符串token解析为长度值
std::string NormalizeUsername(const std::string& raw);  //规范化用户名

}  
