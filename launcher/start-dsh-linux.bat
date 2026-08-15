@echo off
rem Start DeepSeek Harness (Linux) in a new WSL terminal window
rem Distro: Ubuntu_AIagent  |  Web UI: http://127.0.0.1:3081/

start "DeepSeek Harness Linux" wsl.exe -d Ubuntu_AIagent -- bash -lic "cd /home/dyy/deepseek-harness && pnpm dsh web --port 3081 || { echo [FAILED] Press Enter to close; read; }"

echo.
echo Waiting for the Linux server on port 3081 (this window closes by itself) ...
powershell.exe -NoProfile -Command "$t=0; while ($t -lt 300) { try { $c = New-Object Net.Sockets.TcpClient; $c.Connect('127.0.0.1', 3081); $c.Close(); Start-Process 'C:\Program Files\Google\Chrome\Application\chrome.exe' 'http://127.0.0.1:3081/'; exit 0 } catch { Start-Sleep -Seconds 2; $t += 2 } }; exit 1"
if errorlevel 1 (
  echo.
  echo [TIMEOUT] Port 3081 did not open within 5 minutes.
  echo Please check the WSL window for errors. Press any key to close this window.
  pause >nul
)
