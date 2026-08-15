# Launches the built DshShell.exe (does not wait for it to exit).
$ErrorActionPreference = 'Stop'
$exe = 'D:\wsl\dsh-shell\publish\DshShell.exe'

if (-not (Test-Path $exe)) {
    Write-Output "exe not found: $exe"
    exit 1
}

# remove old log so verification reads a fresh run
$log = 'D:\wsl\dsh-shell\publish\dshshell.log'
if (Test-Path $log) { Remove-Item $log -Force }

Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
Write-Output "launched: $exe"
