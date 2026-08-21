#include "common/chat_protocol.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace chat {

void SetupConsoleUtf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

std::string CurrentIsoTimestamp() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto seconds = clock::to_time_t(now);
    std::tm tm_snapshot{};
    gmtime_s(&tm_snapshot, &seconds);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_snapshot);
    return std::string(buffer);
}

bool SendAll(SOCKET socket, const char* data, std::size_t length) {
    std::size_t sent_total = 0;
    while (sent_total < length) {
        const int chunk = send(socket, data + sent_total, static_cast<int>(length - sent_total), 0);
        if (chunk == SOCKET_ERROR || chunk == 0) {
            return false;
        }
        sent_total += static_cast<std::size_t>(chunk);
    }
    return true;
}

bool SendLine(SOCKET socket, const std::string& line) {
    std::string payload = line;
    payload.push_back('\n');
    return SendAll(socket, payload);
}

bool ReceiveLine(SOCKET socket, std::string& buffer, std::string& line) {
    while (true) {
        const auto pos = buffer.find('\n');
        if (pos != std::string::npos) {
            line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }

        char chunk[1024];
        const int received = recv(socket, chunk, static_cast<int>(sizeof(chunk)), 0);
        if (received <= 0) {
            return false;
        }
        buffer.append(chunk, received);
    }
}

bool ReceiveBytes(SOCKET socket, std::string& buffer, std::size_t bytes, std::string& payload) {
    while (buffer.size() < bytes) {
        char chunk[1024];
        const int received = recv(socket, chunk, static_cast<int>(sizeof(chunk)), 0);
        if (received <= 0) {
            return false;
        }
        buffer.append(chunk, received);
    }
    payload.assign(buffer.data(), bytes);
    buffer.erase(0, bytes);
    return true;
}

std::vector<std::string> SplitTokens(const std::string& input, std::size_t max_tokens) {
    std::vector<std::string> tokens;
    std::size_t i = 0;
    while (i < input.size() && tokens.size() + 1 < max_tokens) {
        while (i < input.size() && std::isspace(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
        const std::size_t start = i;
        while (i < input.size() && !std::isspace(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
        if (i > start) {
            tokens.emplace_back(input.substr(start, i - start));
        }
    }
    if (i < input.size()) {
        tokens.emplace_back(input.substr(i));
    }
    return tokens;
}

bool ParseLength(const std::string& token, std::size_t& value) {
    if (token.empty()) {
        return false;
    }
    try {
        const std::size_t pos = token.find_first_not_of("0123456789");
        if (pos != std::string::npos) {
            return false;
        }
        value = static_cast<std::size_t>(std::stoull(token));
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string NormalizeUsername(const std::string& raw) {
    std::string cleaned;
    cleaned.reserve(raw.size());
    for (const char ch : raw) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || ch == '-' || ch == '_') {
            cleaned.push_back(static_cast<char>(c));
        }
    }
    if (cleaned.size() > kMaxUsernameLength) {
        cleaned.resize(kMaxUsernameLength);
    }
    return cleaned;
}

}  // namespace chat
