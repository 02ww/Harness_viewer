# DshShell — DeepSeek Harness 轻量外壳

一个显示 harness Web GUI（http://127.0.0.1:3081）的最小 Windows 程序：
WinForms 窗口 + WebView2（复用系统 Edge 内核），exe 本体仅 148 KB，
不捆绑浏览器，不安装 Chrome。

## 构成

| 文件 | 作用 |
|---|---|
| `Program.cs` | 整个程序：创建最大化窗口、初始化 WebView2、导航到 harness、写日志 |
| `DshShell.csproj` | net8.0-windows + WinForms + Microsoft.Web.WebView2 1.0.2592.51 |

WebView2 NuGet 版本与本机已装 Runtime（126.0.2592.113）同系列，零额外安装。

## 使用

1. 先启动 harness：`cd /home/dyy/deepseek-harness && pnpm dsh web --port 3081`
2. 双击桌面快捷方式「DeepSeek Harness」或 `D:\wsl\dsh-shell\publish\DshShell.exe`

## 重新构建（源码在 WSL，编译用 Windows 侧 SDK）

```sh
powershell.exe -NoProfile -ExecutionPolicy Bypass -File \
  "$(wslpath -w /home/dyy/project1/tools/sync-build.ps1)"
```

脚本会把源码同步到 `D:\wsl\dsh-shell`，用 `D:\dotnet\dotnet.exe`（便携 SDK 8.0.424）
发布到 `D:\wsl\dsh-shell\publish`。发布为 framework-dependent，
依赖本机已有的 .NET 8 Desktop Runtime 和 WebView2 Runtime。

## 日志与诊断

程序在 exe 旁写 `dshshell.log`，记录内核就绪、导航开始/完成（HTTP 状态码）等信息。
harness 未启动时窗口会弹出提示，日志显示 `success=False`。

## 后续可选增强

- 自定义图标：生成 `.ico` 后在 csproj 加 `<ApplicationIcon>app.ico</ApplicationIcon>`
- 单实例互斥：避免重复打开多个窗口
- 自动启动 harness：exe 启动时若 3081 无响应，可自动拉起 `pnpm dsh web`
