using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace DshShell;

internal static class Program
{
    internal const string HarnessUrl = "http://127.0.0.1:3081";

    private static readonly string LogPath =
        Path.Combine(AppContext.BaseDirectory, "dshshell.log");

    internal static void Log(string message)
    {
        try
        {
            File.AppendAllText(LogPath,
                $"{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff} {message}{Environment.NewLine}");
        }
        catch
        {
            // logging must never crash the shell
        }
    }

    [STAThread]
    private static void Main()
    {
        try
        {
            // Must be the very first call: makes the process per-monitor DPI
            // aware (native 144dpi rendering on a 150% display) instead of
            // being bitmap-stretched from 96dpi by the system.
            Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
            Log($"start v{Application.ProductVersion}");
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
            Log("exit");
        }
        catch (Exception ex)
        {
            Log("fatal: " + ex);
            MessageBox.Show("启动失败：\n" + ex.Message,
                "DeepSeek Harness Shell", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}

internal sealed class MainForm : Form
{
    private readonly WebView2 _webView = new()
    {
        Dock = DockStyle.Fill,
    };

    public MainForm()
    {
        Text = "DeepSeek Harness";
        StartPosition = FormStartPosition.CenterScreen;
        WindowState = FormWindowState.Maximized;
        // Use the icon embedded in DshShell.exe (favicon.ico) for the
        // title bar and Windows taskbar, not the default WinForms icon.
        Icon = System.Drawing.Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        Controls.Add(_webView);
        Load += OnLoad;
    }

    private async void OnLoad(object? sender, EventArgs e)
    {
        try
        {
            Program.Log($"device dpi={DeviceDpi}");
            Program.Log("loading webview core...");
            await _webView.EnsureCoreWebView2Async(null);
            Program.Log("webview core ready, navigating to " + Program.HarnessUrl);
            _webView.CoreWebView2.NavigationStarting += (_, args) =>
                Program.Log("navigation starting: " + args.Uri);
            _webView.CoreWebView2.NavigationCompleted += (_, args) =>
            {
                Program.Log($"navigation completed: success={args.IsSuccess} " +
                            $"status={args.WebErrorStatus} " +
                            $"http={(int)args.HttpStatusCode}");
                if (!args.IsSuccess)
                {
                    MessageBox.Show(
                        "无法加载 " + Program.HarnessUrl +
                        "\n请确认 harness 已启动（pnpm dsh web --port 3081）。\n\n" +
                        $"错误：{args.WebErrorStatus}",
                        "DeepSeek Harness Shell",
                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            };
            _webView.CoreWebView2.ProcessFailed += (_, args) =>
                Program.Log($"process failed: {args.ProcessFailedKind}");
            _webView.CoreWebView2.Navigate(Program.HarnessUrl);
        }
        catch (Exception ex)
        {
            Program.Log("webview init failed: " + ex);
            MessageBox.Show("WebView2 初始化失败：\n" + ex.Message,
                "DeepSeek Harness Shell", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
