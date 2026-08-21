#include "common/chat_protocol.h"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
//一个客户端会话
struct ClientSession {
    SOCKET socket = INVALID_SOCKET;
    std::string username;
    std::string buffer;
    std::string remote;
    bool joined = false;
};
//服务器选项
struct ServerOptions {
    std::string host = "127.0.0.1";
    std::string name = "LabServer";
    unsigned short port = 5555;
};

std::mutex g_clients_mutex;
std::vector<std::shared_ptr<ClientSession>> g_clients;  //存储所有已连接的客户端会话
std::atomic<bool> g_running{true};  //标记服务器是否正在运行
SOCKET g_listener = INVALID_SOCKET;  //服务器监听的套接字
ServerOptions g_opts;

void CloseSocket(SOCKET socket) {  //关闭指定的套接字
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_BOTH);
        closesocket(socket);
    }
}

//发送一个数据帧
bool SendFrame(SOCKET socket, const std::string& header, const std::string& payload) {
    if (!chat::SendLine(socket, header)) {
        return false;
    }
    if (!payload.empty()) {
        return chat::SendAll(socket, payload);
    }
    return true;
}

//向所有已连接的客户端广播一个数据帧
void BroadcastFrame(const std::string& header, const std::string& payload) {
    std::lock_guard<std::mutex> lock(g_clients_mutex);
    for (auto it = g_clients.begin(); it != g_clients.end();) {
        if (!SendFrame((*it)->socket, header, payload)) {
            std::cerr << "[WARN] failed to deliver frame to " << (*it)->username << ", removing session\n";
            CloseSocket((*it)->socket);
            it = g_clients.erase(it);
        } else {
            ++it;
        }
    }
}

//广播一条系统消息（重命名避免与 Windows BroadcastSystemMessage 宏冲突）
void BroadcastSystemNotice(const std::string& text) {
    const auto timestamp = chat::CurrentIsoTimestamp();
    const std::string header = "SYS " + timestamp + " " + std::to_string(text.size());
    BroadcastFrame(header, text);
    std::cout << "[SYS] " << text << std::endl;
}

//广播一条聊天消息
void BroadcastChatMessage(const std::string& username, const std::string& text) {
    const auto timestamp = chat::CurrentIsoTimestamp();
    const std::string header = "MSG " + timestamp + " " + username + " " + std::to_string(text.size());
    BroadcastFrame(header, text);
    std::cout << "[" << timestamp << "] " << username << ": " << text << std::endl;
}

//向指定客户端发送一条错误消息
void SendError(SOCKET socket, const std::string& message) {
    const auto timestamp = chat::CurrentIsoTimestamp();
    const std::string header = "ERR " + timestamp + " " + std::to_string(message.size());
    SendFrame(socket, header, message);
}

//注册一个客户端会话
bool RegisterClient(const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(g_clients_mutex);
    const bool exists = std::any_of(g_clients.begin(), g_clients.end(),
                                    [&](const std::shared_ptr<ClientSession>& c) {
                                        return c->username == session->username;
                                    });
    if (exists) {
        return false;
    }
    g_clients.push_back(session);
    session->joined = true;
    return true;
}

//移除一个客户端会话
void RemoveClient(const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(g_clients_mutex);
    g_clients.erase(std::remove_if(g_clients.begin(), g_clients.end(),
                                   [&](const std::shared_ptr<ClientSession>& item) {
                                       return item.get() == session.get();
                                   }),
                    g_clients.end());
}

void HandleClient(std::shared_ptr<ClientSession> session) {
    std::cout << "[INFO] incoming connection from " << session->remote << std::endl;

    std::string line;
    if (!chat::ReceiveLine(session->socket, session->buffer, line)) {
        std::cerr << "[WARN] connection dropped during handshake\n";
        CloseSocket(session->socket);
        return;
    }

    const auto hello_tokens = chat::SplitTokens(line, 3);
    if (hello_tokens.empty() || hello_tokens[0] != "HELLO" || hello_tokens.size() < 2) {
        SendError(session->socket, "HELLO frame expected");
        CloseSocket(session->socket);
        return;
    }

    session->username = chat::NormalizeUsername(hello_tokens[1]);
    if (session->username.size() < chat::kMinUsernameLength) {
        SendError(session->socket, "username must be 2-32 characters (A-Z, a-z, 0-9, -, _)");
        CloseSocket(session->socket);
        return;
    }
    if (!RegisterClient(session)) {
        SendError(session->socket, "username already in use");
        CloseSocket(session->socket);
        return;
    }

    const std::string welcome =
        "WELCOME " + g_opts.name + " " + chat::CurrentIsoTimestamp() + " " + chat::kProtocolVersion;
    chat::SendLine(session->socket, welcome);

    BroadcastSystemNotice(session->username + " joined the chat");

    std::string final_notice;
    while (g_running) {
        if (!chat::ReceiveLine(session->socket, session->buffer, line)) {
            final_notice = session->username + " connection lost";
            break;
        }

        const auto tokens = chat::SplitTokens(line, 3);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "MSG") {
            if (tokens.size() < 2) {
                SendError(session->socket, "MSG frame missing length");
                continue;
            }
            std::size_t length = 0;
            if (!chat::ParseLength(tokens[1], length) || length > chat::kMaxPayloadLength) {
                SendError(session->socket, "invalid message length");
                continue;
            }
            std::string payload;
            if (!chat::ReceiveBytes(session->socket, session->buffer, length, payload)) {
                final_notice = session->username + " connection lost";
                break;
            }
            BroadcastChatMessage(session->username, payload);
        } else if (tokens[0] == "BYE") {
            std::size_t length = 0;
            std::string reason;
            if (tokens.size() >= 2 && chat::ParseLength(tokens[1], length) &&
                length <= chat::kMaxPayloadLength && length > 0) {
                if (!chat::ReceiveBytes(session->socket, session->buffer, length, reason)) {
                    final_notice = session->username + " connection lost";
                } else {
                    final_notice = session->username + " left: " + reason;
                }
            } else {
                final_notice = session->username + " left the chat";
            }
            break;
        } else {
            SendError(session->socket, "unknown command: " + tokens[0]);
        }
    }

    if (session->joined) {
        if (final_notice.empty()) {
            final_notice = session->username + " disconnected";
        }
        BroadcastSystemNotice(final_notice);
        RemoveClient(session);
    }

    CloseSocket(session->socket);
    std::cout << "[INFO] session for " << session->username << " closed\n";
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        g_running = false;
        if (g_listener != INVALID_SOCKET) {
            closesocket(g_listener);
        }
        return TRUE;
    }
    return FALSE;
}

void ShowUsage() {
    std::cout << "Usage: chat_server.exe [--host <127.0.0.1>] [--port <5555>] [--name <ServerName>]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    chat::SetupConsoleUtf8();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            g_opts.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            g_opts.port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == "--name" && i + 1 < argc) {
            g_opts.name = argv[++i];
        } else {
            ShowUsage();
            return 1;
        }
    }

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    g_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listener == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        WSACleanup();
        return 1;
    }

    BOOL reuse = TRUE;
    setsockopt(g_listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_opts.port);
    if (inet_pton(AF_INET, g_opts.host.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "invalid host: " << g_opts.host << "\n";
        CloseSocket(g_listener);
        WSACleanup();
        return 1;
    }

    if (bind(g_listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
        CloseSocket(g_listener);
        WSACleanup();
        return 1;
    }

    if (listen(g_listener, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen() failed\n";
        CloseSocket(g_listener);
        WSACleanup();
        return 1;
    }

    std::cout << "server listening on " << g_opts.host << ":" << g_opts.port << std::endl;

    while (g_running) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(g_listener, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_socket == INVALID_SOCKET) {
            if (!g_running) {
                break;
            }
            std::cerr << "accept() failed: " << WSAGetLastError() << "\n";
            continue;
        }

        char ip_str[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        const int port = ntohs(client_addr.sin_port);

        auto session = std::make_shared<ClientSession>();
        session->socket = client_socket;
        session->remote = std::string(ip_str) + ":" + std::to_string(port);

        std::thread(HandleClient, session).detach();
    }

    g_running = false;
    CloseSocket(g_listener);
    WSACleanup();
    std::cout << "server stopped\n";
    return 0;
}
