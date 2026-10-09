# Enterprise Silent Uninstallation Script for Sentinel
Param(
    [string]$MsiProductCode = ""
)

# "Sentinel Desktop" is the pre-rename registry key and is kept as a fallback.
$UninstallKey = @(
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Sentinel",
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Sentinel Desktop"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($UninstallKey) {
    $UninstallString = (Get-ItemProperty $UninstallKey).UninstallString
    if ($UninstallString) {
        Write-Host "Executing silent NSIS uninstallation..." -ForegroundColor Green
        Start-Process $UninstallString -ArgumentList "/S" -Wait -NoNewWindow
    }
} else {
    Write-Host "Executing silent MSI uninstallation..." -ForegroundColor Green
    Get-WmiObject -Class Win32_Product | Where-Object { $_.Name -like "*Sentinel*" } | ForEach-Object { $_.Uninstall() }
}
