// start-dsh-linux.cpp
// DeepSeek Harness (Linux) 启动器 —— 替代 start-dsh-linux.bat
//
// 本文件为 GBK 编码（为了在 Dev-C++ / VC6 等默认 GBK 的编辑器里中文注释不乱码）。
// 若使用 VSCode 打开，请在右下角把编码选为 GBK。
//
// 功能流程：
//   1. 新开一个控制台窗口，在其中启动 WSL 的 Ubuntu_AIagent，
//      执行: cd /home/dyy/deepseek-harness && pnpm dsh web --port 3081
//   2. 本程序自身完全无窗口（GUI 子系统编译），只在后台：
//      每 2 秒探测一次 127.0.0.1:3081，最多等 5 分钟
//   3. 端口可用后，用 Chrome 打开 http://127.0.0.1:3081/
//   4. 出错/超时用弹窗提示，全程记录日志到 %TEMP%\dsh-launcher.log
//
// 编译（MinGW-w64，-mwindows 表示无窗口 GUI 程序）:
//   g++ -O2 -s -static -std=c++17 -mwindows start-dsh-linux.cpp -o start-dsh-linux.exe -lws2_32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <shellapi.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>

// ---------- 配置区（需要修改时只改这里） ----------
static const char*    kWslDistro      = "Ubuntu_AIagent";          // WSL 发行版名
static const char*    kBashCommand    =                            // Linux 里执行的命令
    "cd /home/dyy/deepseek-harness && pnpm dsh web --port 3081 "
    "|| { echo [FAILED] Press Enter to close; read; }";
static const char*    kPortHost       = "127.0.0.1";
static const int      kPort           = 3081;
static const wchar_t* kUrl            = L"http://127.0.0.1:3081/";
static const wchar_t* kChromePath     = L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe";
static const int      kPollIntervalMs = 2000;   // 探测间隔：2 秒
static const int      kMaxWaitSeconds = 300;    // 最长等待：5 分钟
// -----------------------------------------------------

// 日志文件句柄（写 %TEMP%\dsh-launcher.log，便于排查问题）
static FILE* g_log = nullptr;

static void logf(const char* fmt, ...)
{
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}

// 打开日志文件，并写一行带时间的启动标记
static void openLog()
{
    char path[MAX_PATH];
    GetTempPathA(MAX_PATH, path);
    lstrcatA(path, "dsh-launcher.log");
    g_log = fopen(path, "a");
    if (!g_log) return;

    time_t t = time(nullptr);
    char* ts = ctime(&t);
    logf("---- launcher started: %s", ts ? ts : "(time unknown)\n");
}

// 1) 新开一个控制台窗口启动 WSL（等价于 bat 里的 start ... wsl.exe）
static bool launchWslWindow()
{
    // 把 bash 命令包进双引号，作为一个参数传给 wsl.exe
    std::string cmdLine = "wsl.exe -d ";
    cmdLine += kWslDistro;
    cmdLine += " -- bash -lic \"";
    cmdLine += kBashCommand;
    cmdLine += "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // CREATE_NEW_CONSOLE: 让 WSL 拥有自己的窗口（与原 bat 行为一致）
    if (!CreateProcessA(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
    {
        DWORD err = GetLastError();
        logf("ERROR: CreateProcessA(wsl.exe) failed, code %lu\n", err);
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    logf("WSL window launched: %s\n", cmdLine.c_str());
    return true;
}

// 2) 探测端口：非阻塞 connect + select，timeoutMs 毫秒内连上返回 true
static bool portOpen(int timeoutMs)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;

    u_long nonBlock = 1;
    ioctlsocket(s, FIONBIO, &nonBlock);

    sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((u_short)kPort);
    addr.sin_addr.s_addr = inet_addr(kPortHost);

    connect(s, (sockaddr*)&addr, sizeof(addr)); // 非阻塞：立即返回

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(s, &writeSet);
    timeval tv;
    tv.tv_sec  = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    bool ok = false;
    if (select(0, nullptr, &writeSet, nullptr, &tv) == 1)
    {
        int err = 0;
        int len = sizeof(err);
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
        ok = (err == 0); // SO_ERROR==0 说明真正连上了
    }
    closesocket(s);
    return ok;
}

// 3) 用 Chrome 打开网页（Chrome 不存在时退回系统默认浏览器）
static void openWebUi()
{
    if (GetFileAttributesW(kChromePath) != INVALID_FILE_ATTRIBUTES)
    {
        HINSTANCE r = ShellExecuteW(nullptr, L"open", kChromePath, kUrl, nullptr, SW_SHOWNORMAL);
        if ((INT_PTR)r > 32)
        {
            logf("Chrome launched for port %d\n", kPort);
            return;
        }
    }
    HINSTANCE r = ShellExecuteW(nullptr, L"open", kUrl, nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)r > 32)
        logf("Opened default browser for port %d\n", kPort);
    else
        logf("ERROR: could not open browser, ShellExecuteW returned %d\n", (int)(INT_PTR)r);
}

// 程序入口（GUI 子系统：运行时不产生任何控制台窗口）
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    openLog();

    if (!launchWslWindow())
    {
        MessageBoxW(nullptr,
                    L"Could not launch wsl.exe.\n"
                    L"The WSL window did not open.\n"
                    L"See %TEMP%\\dsh-launcher.log for details.",
                    L"DeepSeek Harness Launcher", MB_OK | MB_ICONERROR);
        if (g_log) fclose(g_log);
        return 1;
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    bool up = false;
    int waited = 0;
    while (waited < kMaxWaitSeconds)
    {
        if (portOpen(1000)) { up = true; break; }  // 每次探测给 1 秒超时
        Sleep(kPollIntervalMs);
        waited += kPollIntervalMs / 1000;
    }
    WSACleanup();

    if (up)
    {
        logf("Port %d is up after ~%d s, opening browser.\n", kPort, waited);
        openWebUi();
        if (g_log) fclose(g_log);
        return 0; // 成功：静默退出，无任何窗口
    }

    logf("TIMEOUT: port %d not open within %d s.\n", kPort, kMaxWaitSeconds);
    MessageBoxW(nullptr,
                L"Port 3081 did not open within 5 minutes.\n"
                L"Please check the WSL window for errors.\n"
                L"See %TEMP%\\dsh-launcher.log for details.",
                L"DeepSeek Harness Launcher", MB_OK | MB_ICONWARNING);
    if (g_log) fclose(g_log);
    return 1;
}
