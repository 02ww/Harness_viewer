# Archives this session's artifacts to D:\DSH_workspace\Linux\project1
$ErrorActionPreference = 'Stop'

$wslSrc = '\\wsl.localhost\Ubuntu_AIagent\home\dyy\project1'
$dst = 'D:\DSH_workspace\Linux\project1'

New-Item -ItemType Directory -Force -Path $dst | Out-Null

Write-Output "copying dsh-shell sources..."
Copy-Item "$wslSrc\dsh-shell" $dst -Recurse -Force

Write-Output "copying top-level README..."
Copy-Item "$wslSrc\README.md" $dst -Force

Write-Output "copying tools..."
Copy-Item "$wslSrc\tools" $dst -Recurse -Force

Write-Output "copying publish output (exe + runtime dlls, excluding WebView2 profile data)..."
if (Test-Path "$dst\publish") { Remove-Item "$dst\publish" -Recurse -Force }
New-Item -ItemType Directory -Force -Path "$dst\publish" | Out-Null
Get-ChildItem 'D:\wsl\dsh-shell\publish' -Force |
    Where-Object { $_.Name -notlike '*.WebView2' } |
    Copy-Item -Destination "$dst\publish" -Recurse -Force

$files = Get-ChildItem $dst -Recurse -File
$totalMb = [math]::Round((($files | Measure-Object Length -Sum).Sum / 1MB), 2)
Write-Output ("`n" + $files.Count + " files, " + $totalMb + " MB")
Write-Output ("top-level of " + $dst + ":")
Get-ChildItem $dst | Select-Object Name | Format-Table -AutoSize
