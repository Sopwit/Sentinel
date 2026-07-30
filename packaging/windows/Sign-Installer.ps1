# Sign-Installer.ps1 - Utility to generate a local self-signed certificate and sign Sentinel installers
Param(
    [string]$InstallerPath,
    [switch]$GenerateCertOnly
)

$ErrorActionPreference = "Stop"

# Define cert subject/names
$Subject = "CN=Sentinel Local Development, O=Sentinel, C=US"
$CertStorePath = "Cert:\CurrentUser\My"
$RootStorePath = "Cert:\LocalMachine\Root"

# 1. Generate certificate if not already present
$Cert = Get-ChildItem -Path $CertStorePath | Where-Object { $_.Subject -like "*CN=Sentinel Local Development*" } | Select-Object -First 1

if ($null -eq $Cert) {
    Write-Host "Generating local self-signed code signing certificate..." -ForegroundColor Cyan
    $Cert = New-SelfSignedCertificate -Subject $Subject -Type CodeSigningCert -KeySpec Signature -KeyLength 2048 -CertStoreLocation $CertStorePath
    Write-Host "Certificate generated successfully with Thumbprint: $($Cert.Thumbprint)" -ForegroundColor Green
} else {
    Write-Host "Using existing certificate with Thumbprint: $($Cert.Thumbprint)" -ForegroundColor Green
}

# Export certificate to trusted root store to make it trusted locally on this PC (requires elevation)
Write-Host "To make Windows trust this certificate locally and prevent SmartScreen warnings," -ForegroundColor Yellow
Write-Host "the certificate must be installed into the Trusted Root Certification Authorities store." -ForegroundColor Yellow
Write-Host "This requires running PowerShell as Administrator." -ForegroundColor Yellow

$IsAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($IsAdmin) {
    Write-Host "PowerShell is elevated. Installing certificate to Trusted Root store..." -ForegroundColor Cyan
    # Export and import
    $tempPath = [System.IO.Path]::GetTempFileName()
    Export-Certificate -Cert $Cert -FilePath $tempPath -Type CERT | Out-Null
    Import-Certificate -FilePath $tempPath -CertStoreLocation $RootStorePath | Out-Null
    Remove-Item $tempPath
    Write-Host "Certificate installed to Trusted Root Certification Authorities successfully!" -ForegroundColor Green
} else {
    Write-Host "PowerShell is NOT elevated. Skipping installation to Trusted Root store." -ForegroundColor Yellow
    Write-Host "Please re-run this script in an Elevated Administrator PowerShell window to install trust." -ForegroundColor Yellow
}

if ($GenerateCertOnly) {
    exit 0
}

# 2. Find signtool
if (-not $InstallerPath) {
    Write-Error "Please specify the path to the installer package using -InstallerPath."
}

if (-not (Test-Path $InstallerPath)) {
    Write-Error "Installer file not found at: $InstallerPath"
}

$SignTool = Get-Command signtool -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if ($null -eq $SignTool) {
    # Check common Windows SDK paths
    $sdkPaths = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\x64\signtool.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.19041.0\x64\signtool.exe"
    )
    foreach ($path in $sdkPaths) {
        if (Test-Path $path) {
            $SignTool = $path
            break
        }
    }
}

if ($null -eq $SignTool) {
    Write-Error "signtool.exe was not found. Please install the Windows SDK."
}

Write-Host "Using Signtool at: $SignTool" -ForegroundColor Cyan
Write-Host "Signing: $InstallerPath" -ForegroundColor Cyan

# Sign installer
& $SignTool sign /fd SHA256 /sha1 $Cert.Thumbprint /tr "http://timestamp.digicert.com" /td SHA256 $InstallerPath

Write-Host "Installer signed successfully! SmartScreen 'Publisher Unknown' warnings should be gone when installing on this system." -ForegroundColor Green
