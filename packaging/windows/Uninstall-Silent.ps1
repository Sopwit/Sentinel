# Enterprise Silent Uninstallation Script for Sentinel Desktop
Param(
    [string]$MsiProductCode = ""
)

$UninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Sentinel Desktop"
if (Test-Path $UninstallKey) {
    $UninstallString = (Get-ItemProperty $UninstallKey).UninstallString
    if ($UninstallString) {
        Write-Host "Executing silent NSIS uninstallation..." -ForegroundColor Green
        Start-Process $UninstallString -ArgumentList "/S" -Wait -NoNewWindow
    }
} else {
    Write-Host "Executing silent MSI uninstallation..." -ForegroundColor Green
    Get-WmiObject -Class Win32_Product | Where-Object { $_.Name -like "*Sentinel Desktop*" } | ForEach-Object { $_.Uninstall() }
}
