# Syncs the dsh-shell sources from WSL and publishes a win-x64 exe on D:.
$ErrorActionPreference = 'Stop'

$src = '\\wsl.localhost\Ubuntu_AIagent\home\dyy\project1\dsh-shell'
$dst = 'D:\wsl\dsh-shell'

Write-Output "syncing $src -> $dst"
New-Item -ItemType Directory -Force -Path $dst | Out-Null
Copy-Item "$src\*" $dst -Force -Recurse

Write-Output "publishing..."
& 'D:\dotnet\dotnet.exe' publish "$dst\DshShell.csproj" `
    -c Release -r win-x64 --self-contained false -o "$dst\publish" `
    --nologo -v minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Output "publish output:"
Get-ChildItem "$dst\publish" | Select-Object Name, Length | Format-Table -AutoSize
