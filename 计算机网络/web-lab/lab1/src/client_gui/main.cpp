#include "common/chat_protocol.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace {

constexpr UINT WM_CHAT_LOG = WM_APP + 1;
constexpr UINT WM_CHAT_CONNECTED = WM_APP + 2;

enum ControlId : int {
    IDC_LOG = 1001,
    IDC_INPUT = 1002,
    IDC_SEND = IDOK
};

constexpr INT_PTR IDC_LOGIN_EDIT = 2001;

struct ClientOptions {
    std::string host = "127.0.0.1";
    unsigned short port = 5555;
    std::string username = "GuiUser";
};

struct GuiState {
    HWND hwnd = nullptr;
    HWND log_view = nullptr;
    HWND input = nullptr;
    HWND send_button = nullptr;
    std::atomic<bool> running{true};
    std::atomic<bool> connected{false};
    SOCKET socket = INVALID_SOCKET;
    std::mutex socket_mutex;
    HANDLE worker_handle = nullptr;
    ClientOptions options;
};

struct LoginDialogState {
    HWND hwnd = nullptr;
    HWND edit = nullptr;
    std::wstring value;
    bool accepted = false;
    bool finished = false;
};

const wchar_t kLoginWindowClass[] = L"ChatLoginWindow";

GuiState* GetState(HWND hwnd) {
    return reinterpret_cast<GuiState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return L"";
    }
    const int size =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0) {
        return L"";
    }
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), size,
                        nullptr, nullptr);
    return utf8;
}

void AppendText(HWND edit, const std::wstring& text) {
    const int len = GetWindowTextLengthW(edit);
    SendMessageW(edit, EM_SETSEL, len, len);
    SendMessageW(edit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
}

void PostLog(HWND hwnd, const std::wstring& message) {
    auto* payload = new std::wstring(message);
    PostMessageW(hwnd, WM_CHAT_LOG, 0, reinterpret_cast<LPARAM>(payload));
}

void NotifyConnection(HWND hwnd, bool connected) {
    PostMessageW(hwnd, WM_CHAT_CONNECTED, connected ? 1u : 0u, 0);
}

void RequestSocketShutdown(GuiState* state) {
    std::lock_guard<std::mutex> lock(state->socket_mutex);
    if (state->socket != INVALID_SOCKET) {
        shutdown(state->socket, SD_BOTH);
    }
}

bool SendPayload(GuiState* state, const std::string& payload) {
    SOCKET socket = INVALID_SOCKET;
    {
        std::lock_guard<std::mutex> lock(state->socket_mutex);
        socket = state->socket;
    }
    if (socket == INVALID_SOCKET) {
        return false;
    }
    const std::string header = "MSG " + std::to_string(payload.size());
    return chat::SendLine(socket, header) && chat::SendAll(socket, payload);
}

DWORD WINAPI NetworkThreadProc(LPVOID param) {
    auto* state = reinterpret_cast<GuiState*>(param);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        PostLog(state->hwnd, L"[ERROR] Failed to create socket.\r\n");
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(state->options.port);
    if (inet_pton(AF_INET, state->options.host.c_str(), &addr.sin_addr) != 1) {
        PostLog(state->hwnd, L"[ERROR] Invalid host address.\r\n");
        closesocket(sock);
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    PostLog(state->hwnd, L"[INFO] Connecting to server...\r\n");
    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        PostLog(state->hwnd, L"[ERROR] Unable to connect to server.\r\n");
        closesocket(sock);
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    const std::string hello = "HELLO " + state->options.username + " CLIENT/GUI";
    if (!chat::SendLine(sock, hello)) {
        PostLog(state->hwnd, L"[ERROR] Failed to send HELLO frame.\r\n");
        closesocket(sock);
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    std::string buffer;
    std::string line;
    if (!chat::ReceiveLine(sock, buffer, line)) {
        PostLog(state->hwnd, L"[ERROR] Connection closed during handshake.\r\n");
        closesocket(sock);
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    const auto welcome_tokens = chat::SplitTokens(line, 4);
    if (welcome_tokens.empty() || welcome_tokens[0] != "WELCOME") {
        PostLog(state->hwnd, L"[ERROR] Unexpected handshake response.\r\n");
        closesocket(sock);
        NotifyConnection(state->hwnd, false);
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(state->socket_mutex);
        state->socket = sock;
    }
    state->connected = true;
    const std::wstring server_name =
        welcome_tokens.size() >= 2 ? Utf8ToWide(welcome_tokens[1]) : L"Server";
    PostLog(state->hwnd, L"[INFO] Connected to " + server_name + L".\r\n");
    NotifyConnection(state->hwnd, true);

    while (state->running) {
        if (!chat::ReceiveLine(sock, buffer, line)) {
            PostLog(state->hwnd, L"[WARN] Disconnected from server.\r\n");
            break;
        }
        const auto tokens = chat::SplitTokens(line, 5);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "MSG" && tokens.size() >= 4) {
            std::size_t length = 0;
            if (!chat::ParseLength(tokens[3], length) || length > chat::kMaxPayloadLength) {
                continue;
            }
            std::string payload;
            if (!chat::ReceiveBytes(sock, buffer, length, payload)) {
                break;
            }
            const std::wstring log_line =
                L"[" + Utf8ToWide(tokens[1]) + L"] " + Utf8ToWide(tokens[2]) + L": " +
                Utf8ToWide(payload) + L"\r\n";
            PostLog(state->hwnd, log_line);
        } else if ((tokens[0] == "SYS" || tokens[0] == "ERR") && tokens.size() >= 3) {
            std::size_t length = 0;
            if (!chat::ParseLength(tokens[2], length) || length > chat::kMaxPayloadLength) {
                continue;
            }
            std::string payload;
            if (!chat::ReceiveBytes(sock, buffer, length, payload)) {
                break;
            }
            const std::wstring prefix = tokens[0] == "ERR" ? L"[ERROR] " : L"[SYS] ";
            PostLog(state->hwnd, prefix + Utf8ToWide(tokens[1]) + L" " + Utf8ToWide(payload) +
                                     L"\r\n");
        } else if (tokens[0] == "WELCOME") {
            PostLog(state->hwnd, Utf8ToWide(line) + L"\r\n");
        }
    }

    state->connected = false;
    NotifyConnection(state->hwnd, false);
    SOCKET owned_socket = INVALID_SOCKET;
    {
        std::lock_guard<std::mutex> lock(state->socket_mutex);
        owned_socket = state->socket;
        state->socket = INVALID_SOCKET;
    }
    if (owned_socket != INVALID_SOCKET) {
        shutdown(owned_socket, SD_BOTH);
        closesocket(owned_socket);
    } else {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
    }
    return 0;
}

void SendUserInput(GuiState* state) {
    if (!state->connected.load()) {
        PostLog(state->hwnd, L"[WARN] Not connected yet.\r\n");
        return;
    }
    const int length = GetWindowTextLengthW(state->input);
    if (length <= 0) {
        return;
    }
    std::wstring text(static_cast<std::size_t>(length), L'\0');
    GetWindowTextW(state->input, text.data(), length + 1);

    while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n')) {
        text.pop_back();
    }
    if (text.empty()) {
        SetWindowTextW(state->input, L"");
        return;
    }

    const std::string payload = WideToUtf8(text);
    if (payload.size() > chat::kMaxPayloadLength) {
        PostLog(state->hwnd, L"[WARN] Message too long.\r\n");
        return;
    }

    if (!SendPayload(state, payload)) {
        PostLog(state->hwnd, L"[ERROR] Failed to send message.\r\n");
        return;
    }
    SetWindowTextW(state->input, L"");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = GetState(hwnd);

    switch (message) {
        case WM_CREATE: {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = reinterpret_cast<GuiState*>(create->lpCreateParams);
            state->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            const int padding = 12;
            const int button_width = 90;
            const int input_height = 28;

            RECT rect{};
            GetClientRect(hwnd, &rect);
            const int width = rect.right - rect.left;
            const int height = rect.bottom - rect.top;

            state->log_view = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                    ES_READONLY,
                padding, padding, width - padding * 2,
                height - padding * 3 - input_height,
                hwnd, reinterpret_cast<HMENU>(IDC_LOG), create->hInstance, nullptr);

            state->input = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                padding, height - padding - input_height, width - padding * 3 - button_width,
                input_height, hwnd, reinterpret_cast<HMENU>(IDC_INPUT), create->hInstance, nullptr);

            state->send_button = CreateWindowExW(
                0, L"BUTTON", L"Send",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                width - padding - button_width, height - padding - input_height, button_width,
                input_height, hwnd, reinterpret_cast<HMENU>(IDC_SEND), create->hInstance, nullptr);

            EnableWindow(state->input, FALSE);
            EnableWindow(state->send_button, FALSE);
            return 0;
        }
        case WM_SIZE: {
            if (!state) {
                break;
            }
            RECT rect{};
            GetClientRect(hwnd, &rect);
            const int padding = 12;
            const int button_width = 90;
            const int input_height = 28;
            const int width = rect.right - rect.left;
            const int height = rect.bottom - rect.top;

            SetWindowPos(state->log_view, nullptr, padding, padding, width - padding * 2,
                         height - padding * 3 - input_height, SWP_NOZORDER);
            SetWindowPos(state->input, nullptr, padding, height - padding - input_height,
                         width - padding * 3 - button_width, input_height, SWP_NOZORDER);
            SetWindowPos(state->send_button, nullptr, width - padding - button_width,
                         height - padding - input_height, button_width, input_height,
                         SWP_NOZORDER);
            return 0;
        }
        case WM_COMMAND: {
            if (!state) {
                break;
            }
            const int control_id = LOWORD(wParam);
            const int notification = HIWORD(wParam);
            if (control_id == IDC_SEND && notification == BN_CLICKED) {
                SendUserInput(state);
                SetFocus(state->input);
            }
            return 0;
        }
        case WM_CHAT_LOG: {
            if (!state) {
                break;
            }
            auto* text = reinterpret_cast<std::wstring*>(lParam);
            if (text && state->log_view) {
                AppendText(state->log_view, *text);
                SendMessageW(state->log_view, EM_SCROLLCARET, 0, 0);
            }
            delete text;
            return 0;
        }
        case WM_CHAT_CONNECTED: {
            if (!state) {
                break;
            }
            const bool connected = wParam != 0;
            EnableWindow(state->input, connected);
            EnableWindow(state->send_button, connected);
            if (connected) {
                SetFocus(state->input);
            }
            return 0;
        }
        case WM_DESTROY: {
            if (state) {
                state->running = false;
                RequestSocketShutdown(state);
            }
            PostQuitMessage(0);
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

ClientOptions ParseOptions(bool& has_username_arg) {
    ClientOptions opts;
    has_username_arg = false;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return opts;
    }
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--host" && i + 1 < argc) {
            opts.host = WideToUtf8(argv[++i]);
        } else if (arg == L"--port" && i + 1 < argc) {
            opts.port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == L"--name" && i + 1 < argc) {
            opts.username = WideToUtf8(argv[++i]);
            has_username_arg = true;
        }
    }
    LocalFree(argv);
    return opts;
}

std::string DefaultUsername() {
    const unsigned value = GetTickCount() % 10000;
    return "GuiUser" + std::to_string(value);
}

void NormalizeOptions(ClientOptions& opts) {
    if (opts.username.empty()) {
        opts.username = DefaultUsername();
    }
    opts.username = chat::NormalizeUsername(opts.username);
    if (opts.username.size() < chat::kMinUsernameLength) {
        opts.username = DefaultUsername();
    }
}

LRESULT CALLBACK LoginWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

bool EnsureLoginWindowClass(HINSTANCE instance) {
    static ATOM login_class_atom = 0;
    if (login_class_atom != 0) {
        return true;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = LoginWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kLoginWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.style = CS_DBLCLKS;

    login_class_atom = RegisterClassExW(&wc);
    return login_class_atom != 0;
}

void CenterWindowOnScreen(HWND hwnd, int width, int height) {
    const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = GetSystemMetrics(SM_CYSCREEN);
    const int x = (screen_width - width) / 2;
    const int y = (screen_height - height) / 3;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK LoginWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<LoginDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
        case WM_NCCREATE: {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = reinterpret_cast<LoginDialogState*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            return TRUE;
        }
        case WM_CREATE: {
            if (!state) {
                return -1;
            }
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state->hwnd = hwnd;
            RECT rect{};
            GetClientRect(hwnd, &rect);
            const int width = rect.right - rect.left;
            const int padding = 16;
            const int label_height = 20;
            const int edit_height = 24;
            const int button_width = 90;
            const int button_height = 28;
            const int spacing = 12;
            const int buttons_top = rect.bottom - padding - button_height;

            HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

            HWND label = CreateWindowExW(
                0, L"STATIC", L"Enter username (2-32 chars: letters/digits/-/_)",
                WS_CHILD | WS_VISIBLE, padding, padding, width - padding * 2, label_height, hwnd,
                nullptr, create->hInstance, nullptr);
            SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

            state->edit = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", state->value.c_str(),
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                padding, padding + label_height + 6, width - padding * 2, edit_height, hwnd,
                reinterpret_cast<HMENU>(IDC_LOGIN_EDIT), create->hInstance, nullptr);
            SendMessageW(state->edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(state->edit, EM_LIMITTEXT, chat::kMaxUsernameLength, 0);

            const int ok_left = width / 2 - button_width - spacing / 2;
            const int cancel_left = width / 2 + spacing / 2;

            HWND ok_button = CreateWindowExW(
                0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                ok_left, buttons_top, button_width, button_height, hwnd,
                reinterpret_cast<HMENU>(IDOK), create->hInstance, nullptr);
            SendMessageW(ok_button, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

            HWND cancel_button = CreateWindowExW(
                0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, cancel_left, buttons_top, button_width,
                button_height, hwnd, reinterpret_cast<HMENU>(IDCANCEL), create->hInstance, nullptr);
            SendMessageW(cancel_button, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

            SetFocus(state->edit);
            SendMessageW(state->edit, EM_SETSEL, 0, -1);
            return 0;
        }
        case WM_COMMAND: {
            if (!state) {
                break;
            }
            const int control_id = LOWORD(wParam);
            if (control_id == IDOK) {
                if (!state->edit) {
                    break;
                }
                const int length = GetWindowTextLengthW(state->edit);
                if (length <= 0) {
                    MessageBoxW(hwnd, L"Please enter a username.", L"Chat Client GUI",
                                MB_ICONWARNING | MB_OK);
                    return 0;
                }
                std::wstring text(static_cast<std::size_t>(length + 1), L'\0');
                GetWindowTextW(state->edit, text.data(), length + 1);
                text.resize(static_cast<std::size_t>(length));
                const std::string normalized = chat::NormalizeUsername(WideToUtf8(text));
                if (normalized.size() < chat::kMinUsernameLength) {
                    MessageBoxW(hwnd,
                                L"Username must be 2-32 chars (letters/digits/-/_).",
                                L"Chat Client GUI", MB_ICONWARNING | MB_OK);
                    return 0;
                }
                state->value = Utf8ToWide(normalized);
                state->accepted = true;
                DestroyWindow(hwnd);
                return 0;
            } else if (control_id == IDCANCEL) {
                if (state) {
                    state->accepted = false;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY: {
            if (state) {
                state->hwnd = nullptr;
                state->finished = true;
            }
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool PromptForUsernameDialog(HINSTANCE instance, std::string& username) {
    if (!EnsureLoginWindowClass(instance)) {
        return false;
    }

    LoginDialogState dialog;
    if (!username.empty()) {
        dialog.value = Utf8ToWide(username);
    } else {
        dialog.value = Utf8ToWide(DefaultUsername());
    }

    const int width = 400;
    const int height = 180;
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, kLoginWindowClass, L"Enter Username",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr,
                                instance, &dialog);
    if (!hwnd) {
        return false;
    }

    dialog.hwnd = hwnd;
    CenterWindowOnScreen(hwnd, width, height);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (!dialog.finished && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        HWND target = dialog.hwnd;
        if (!target || !IsDialogMessageW(target, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (dialog.accepted) {
        username = WideToUtf8(dialog.value);
        return true;
    }
    return false;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show) {
    GuiState state;
    bool username_from_cli = false;
    state.options = ParseOptions(username_from_cli);
    if (!username_from_cli) {
        if (!PromptForUsernameDialog(instance, state.options.username)) {
            return 0;
        }
    }
    NormalizeOptions(state.options);

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        MessageBoxW(nullptr, L"WSAStartup failed.", L"Chat Client GUI", MB_ICONERROR);
        return 1;
    }

    const wchar_t kClassName[] = L"ChatGuiWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class.", L"Chat Client GUI", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    const int width = 640;
    const int height = 480;
    HWND hwnd = CreateWindowExW(0, kClassName, L"Chat Client (GUI)", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr,
                                instance, &state);
    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create window.", L"Chat Client GUI", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    state.worker_handle = CreateThread(nullptr, 0, NetworkThreadProc, &state, 0, nullptr);
    if (!state.worker_handle) {
        MessageBoxW(hwnd, L"Failed to start network thread.", L"Chat Client GUI", MB_ICONERROR);
        DestroyWindow(hwnd);
        WSACleanup();
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(state.hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    state.running = false;
    RequestSocketShutdown(&state);
    if (state.worker_handle) {
        WaitForSingleObject(state.worker_handle, INFINITE);
        CloseHandle(state.worker_handle);
    }

    WSACleanup();
    return static_cast<int>(msg.wParam);
}
