#include "common/chat_protocol.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ClientOptions {
    std::string host = "127.0.0.1";
    unsigned short port = 5555;
    std::string username;
};

std::mutex g_io_mutex;

bool PromptInteractiveOptions(ClientOptions& opts) {
    std::cout << "==== Chat Client ====\n";
    std::cout << "Server host [" << opts.host << "]: " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return false;
    }
    if (!line.empty()) {
        opts.host = line;
    }

    while (true) {
        std::cout << "Server port [" << opts.port << "]: " << std::flush;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        if (line.empty()) {
            break;
        }
        try {
            const int value = std::stoi(line);
            if (value > 0 && value <= 65535) {
                opts.port = static_cast<unsigned short>(value);
                break;
            }
        } catch (...) {
        }
        std::cout << "Please enter a port between 1 and 65535.\n";
    }

    while (true) {
        std::cout << "Username (letters/digits/-/_ length 2-32): " << std::flush;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        const std::string normalized = chat::NormalizeUsername(line);
        if (normalized.size() >= chat::kMinUsernameLength) {
            opts.username = normalized;
            break;
        }
        std::cout << "Invalid username, please try again.\n";
    }
    return true;
}

void PrintUsage() {
    std::cout << "Usage: chat_client.exe --host <server> --port <port> --name <username>\n";
}

bool SendClientMessage(SOCKET socket, const std::string& payload) {
    const std::string header = "MSG " + std::to_string(payload.size());
    return chat::SendLine(socket, header) && chat::SendAll(socket, payload);
}

bool SendClientBye(SOCKET socket, const std::string& reason) {
    const std::string header = "BYE " + std::to_string(reason.size());
    return chat::SendLine(socket, header) && chat::SendAll(socket, reason);
}

void ReceiverLoop(SOCKET socket, std::atomic<bool>& running) {
    std::string buffer;
    while (running.load()) {
        std::string line;
        if (!chat::ReceiveLine(socket, buffer, line)) {
            std::lock_guard<std::mutex> lock(g_io_mutex);
            std::cout << "\n[INFO] connection closed by server\n";
            running = false;
            break;
        }

        const auto tokens = chat::SplitTokens(line, 5);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "MSG") {
            if (tokens.size() < 4) {
                continue;
            }
            std::size_t length = 0;
            if (!chat::ParseLength(tokens[3], length)) {
                continue;
            }
            std::string payload;
            if (!chat::ReceiveBytes(socket, buffer, length, payload)) {
                running = false;
                break;
            }
            std::lock_guard<std::mutex> lock(g_io_mutex);
            std::cout << "\n[" << tokens[1] << "] " << tokens[2] << ": " << payload << std::endl << "> " << std::flush;
        } else if (tokens[0] == "SYS" || tokens[0] == "ERR") {
            if (tokens.size() < 3) {
                continue;
            }
            std::size_t length = 0;
            if (!chat::ParseLength(tokens[2], length)) {
                continue;
            }
            std::string payload;
            if (!chat::ReceiveBytes(socket, buffer, length, payload)) {
                running = false;
                break;
            }
            std::lock_guard<std::mutex> lock(g_io_mutex);
            const std::string prefix = tokens[0] == "ERR" ? "[ERROR]" : "[SYS]";
            std::cout << "\n" << prefix << " " << tokens[1] << " " << payload << std::endl << "> " << std::flush;
        } else if (tokens[0] == "WELCOME") {
            std::lock_guard<std::mutex> lock(g_io_mutex);
            std::cout << "\n[INFO] connected to " << line << std::endl << "> " << std::flush;
        } else {
            std::lock_guard<std::mutex> lock(g_io_mutex);
            std::cout << "\n[DEBUG] unhandled frame: " << line << std::endl << "> " << std::flush;
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    chat::SetupConsoleUtf8();

    ClientOptions opts;
    const bool interactive_mode = argc <= 1;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            opts.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            opts.port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == "--name" && i + 1 < argc) {
            opts.username = argv[++i];
        } else {
            PrintUsage();
            return 1;
        }
    }

    if (opts.username.empty()) {
        if (!interactive_mode || !PromptInteractiveOptions(opts)) {
            PrintUsage();
            return 1;
        }
    }

    opts.username = chat::NormalizeUsername(opts.username);
    if (opts.username.size() < chat::kMinUsernameLength) {
        std::cerr << "Invalid username. Use --name <letters/digits/-/_ length 2-32>\n";
        return 1;
    }

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(opts.port);
    if (inet_pton(AF_INET, opts.host.c_str(), &server_addr.sin_addr) != 1) {
        std::cerr << "invalid host address\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "connect() failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const std::string hello = "HELLO " + opts.username + " CLIENT/1.0";
    if (!chat::SendLine(sock, hello)) {
        std::cerr << "failed to send HELLO frame\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string buffer;
    std::string line;
    if (!chat::ReceiveLine(sock, buffer, line)) {
        std::cerr << "server closed connection during handshake\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const auto welcome_tokens = chat::SplitTokens(line, 4);
    if (welcome_tokens.empty() || welcome_tokens[0] != "WELCOME") {
        std::cerr << "unexpected handshake response: " << line << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const std::string server_name = welcome_tokens.size() >= 2 ? welcome_tokens[1] : "server";
    std::cout << "Connected to " << server_name << " as " << opts.username << std::endl;
    std::cout << "Type messages and press Enter to send. Use /quit to exit.\n> " << std::flush;

    std::atomic<bool> running{true};
    std::thread receiver(ReceiverLoop, sock, std::ref(running));

    std::string input;
    while (running.load() && std::getline(std::cin, input)) {
        if (!running.load()) {
            break;
        }
        if (input == "/quit") {
            SendClientBye(sock, "client quit");
            running = false;
            break;
        }
        if (input.empty()) {
            std::cout << "> " << std::flush;
            continue;
        }
        if (input.size() > chat::kMaxPayloadLength) {
            std::cout << "[WARN] message too long (max " << chat::kMaxPayloadLength << " bytes)\n> " << std::flush;
            continue;
        }
        if (!SendClientMessage(sock, input)) {
            std::cout << "\n[ERROR] failed to send message\n";
            running = false;
            break;
        }
        std::cout << "> " << std::flush;
    }

    running = false;
    shutdown(sock, SD_BOTH);
    if (receiver.joinable()) {
        receiver.join();
    }

    closesocket(sock);
    WSACleanup();
    std::cout << "\nclient exited\n";
    return 0;
}
