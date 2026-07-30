# Enterprise Silent Installation Script for Sentinel Desktop
Param(
    [string]$InstallerPath = ".\Sentinel-1.0.0-win64.exe",
    [string]$MsiPath = ".\Sentinel-1.0.0-win64.msi"
)

if (Test-Path $MsiPath) {
    Write-Host "Executing silent MSI installation..." -ForegroundColor Green
    Start-Process msiexec.exe -ArgumentList "/i `"$MsiPath`" /qn /norestart" -Wait -NoNewWindow
} elseif (Test-Path $InstallerPath) {
    Write-Host "Executing silent NSIS installation..." -ForegroundColor Green
    Start-Process $InstallerPath -ArgumentList "/S" -Wait -NoNewWindow
} else {
    Write-Error "No installer binary found at $InstallerPath or $MsiPath"
}
