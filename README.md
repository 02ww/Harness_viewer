# DeepSeek Harness Windows Launcher

在 Windows 上一键启动 WSL 中的 DeepSeek Harness，并用轻量 WebView2 外壳打开 Web UI。

## 组件

| 目录 | 说明 |
|---|---|
| `launcher/` | Windows 启动器源码（C++ / MinGW），启动 WSL、探测端口、打开浏览器 |
| `dsh-shell/` | DshShell 轻量外壳源码（.NET 8 WinForms + WebView2），带自定义图标 |
| `tools/` | 构建/同步脚本 |

### launcher

- `start-dsh-linux.cpp`：原版，用 Chrome 打开。
- `start-dsh-dshshell.cpp`：DshShell 版；WSL 在后台隐藏运行，优先用 DshShell 打开，失败时退回默认浏览器。
- `*.bat`：等价的批处理版本（早期方案，保留备用）。

编译（MinGW-w64）：

```bat
g++ -O2 -s -static -std=c++17 -mwindows start-dsh-dshshell.cpp -o start-dsh-dshshell.exe -lws2_32
```

### dsh-shell

- `Program.cs`：WinForms + WebView2，导航到 `http://127.0.0.1:3081`。
- `DshShell.csproj`：net8.0-windows，嵌入 `favicon.ico` 作为窗口/任务栏图标。
- 发布：

```bat
dotnet publish DshShell.csproj -c Release -o publish
```

## 运行依赖

- Windows 10/11 + WSL2（发行版 `Ubuntu_AIagent`）
- .NET 8 Desktop Runtime
- WebView2 Runtime
- WSL 内已安装 `pnpm`，且存在 `/home/dyy/deepseek-harness`

## 安全说明

- 仓库只包含源码，不包含编译产物、运行日志或 WebView2 用户数据目录。
- WebView2 用户数据默认生成在 exe 旁的 `DshShell.exe.WebView2` 目录，可能缓存网页内容，**不要提交该目录**。
