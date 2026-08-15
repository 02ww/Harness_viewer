// start-dsh-dshshell.cpp
// DeepSeek Harness (Linux) 启动器（DshShell 版，WSL 后台隐藏运行）
//
// 本文件为 GBK 编码（为了在 Dev-C++ / VC6 等默认 GBK 的编辑器里中文注释不乱码）。
// 若使用 VSCode 打开，请在右下角把编码选为 GBK。
//
// 与原版 start-dsh-linux.cpp 的功能差异：
//   1. WSL 控制台窗口隐藏（CREATE_NO_WINDOW），不再弹出黑窗口；
//      WSL 内输出重定向到 /home/dyy/dsh-launcher-wsl.log。
//   2. 端口就绪后用轻量化外壳 DshShell.exe（WebView2）打开；
//      如果 DshShell.exe 不存在或启动失败，自动退回系统默认浏览器。
//
// 功能流程：
//   1. 在后台隐藏启动 WSL 的 Ubuntu_AIagent，
//      执行: cd /home/dyy/deepseek-harness && pnpm dsh web --port 3081
//   2. 本程序自身完全无窗口（GUI 子系统编译），只在后台：
//      每 2 秒探测一次 127.0.0.1:3081，最多等 5 分钟
//   3. 端口可用后，用 DshShell.exe 打开 http://127.0.0.1:3081/
//   4. 出错/超时用弹窗提示，全程记录日志到 %TEMP%\dsh-launcher.log
//
// 编译（MinGW-w64，-mwindows 表示无窗口 GUI 程序）:
//   g++ -O2 -s -static -std=c++17 -mwindows start-dsh-dshshell.cpp -o start-dsh-dshshell.exe -lws2_32

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
static const char*    kBashCommand    =                            // Linux 里执行的命令（输出写入 /home/dyy/dsh-launcher-wsl.log）
    "cd /home/dyy/deepseek-harness && "
    "exec > /home/dyy/dsh-launcher-wsl.log 2>&1; "
    "pnpm dsh web --port 3081; "
    "echo [dsh exited]";
static const char*    kPortHost       = "127.0.0.1";
static const int      kPort           = 3081;
static const wchar_t* kUrl            = L"http://127.0.0.1:3081/";
static const wchar_t* kDshShellDir    = L"D:\\wsl\\dsh-shell\\publish";
static const wchar_t* kDshShellPath   = L"D:\\wsl\\dsh-shell\\publish\\DshShell.exe";
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
    logf("---- launcher started (DshShell variant, hidden WSL): %s", ts ? ts : "(time unknown)\n");
}

// 1) 在后台隐藏启动 WSL（不再弹出控制台窗口）
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
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // CREATE_NO_WINDOW: wsl.exe 在后台运行，不显示任何控制台窗口
    if (!CreateProcessA(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        DWORD err = GetLastError();
        logf("ERROR: CreateProcessA(wsl.exe) failed, code %lu\n", err);
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    logf("WSL launched hidden: %s\n", cmdLine.c_str());
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

// 3) 用 DshShell 打开网页；DshShell 不存在或启动失败时退回系统默认浏览器
static void openWebUi()
{
    DWORD attr = GetFileAttributesW(kDshShellPath);
    bool dshAvailable = (attr != INVALID_FILE_ATTRIBUTES) &&
                        ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);

    if (dshAvailable)
    {
        HINSTANCE r = ShellExecuteW(nullptr, L"open", kDshShellPath, L"",
                                    kDshShellDir, SW_SHOWNORMAL);
        if ((INT_PTR)r > 32)
        {
            logf("DshShell launched for port %d\n", kPort);
            return;
        }
        logf("WARNING: DshShell launch failed (ShellExecuteW returned %d), "
             "falling back to default browser\n", (int)(INT_PTR)r);
    }
    else
    {
        logf("DshShell not found, falling back to default browser\n");
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
                    L"The WSL server was not started.\n"
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
                L"WSL runs hidden in the background.\n"
                L"Check /home/dyy/dsh-launcher-wsl.log for server errors.\n"
                L"See %TEMP%\\dsh-launcher.log for launcher details.",
                L"DeepSeek Harness Launcher", MB_OK | MB_ICONWARNING);
    if (g_log) fclose(g_log);
    return 1;
}
