#!/usr/bin/env pwsh
# =========================================================================
# Auric Release Script for Windows
# =========================================================================
# Creates a distributable ZIP package with binaries and required assets.
#
# Usage:
#   .\scripts\create-release.ps1 [-Version <version>]
#
# Examples:
#   .\scripts\create-release.ps1                    # Creates auric-windows.zip
#   .\scripts\create-release.ps1 -Version "1.0.0"   # Creates auric-1.0.0-windows.zip
# =========================================================================

param(
    [string]$Version = "",
    [string]$BuildDir = "build-vs2022",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

# Determine output filename
if ($Version) {
    $ArchiveName = "auric-$Version-windows.zip"
} else {
    $ArchiveName = "auric-windows.zip"
}

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Auric Windows Release Packager" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check if build exists
$ReleaseDir = Join-Path $BuildDir "src\$Config"
if (-not (Test-Path $ReleaseDir)) {
    Write-Error "Build directory not found: $ReleaseDir"
    Write-Host "Please build first with: cmake --build --preset windows-vs2022-$($Config.ToLower())"
    exit 1
}

$ExePath = Join-Path $ReleaseDir "auric.exe"
if (-not (Test-Path $ExePath)) {
    Write-Error "auric.exe not found in $ReleaseDir"
    Write-Host "Please build the project first."
    exit 1
}

# Create temporary staging directory
$StagingDir = Join-Path $env:TEMP "auric-release-$(Get-Random)"
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

Write-Host "Staging directory: $StagingDir" -ForegroundColor Gray
Write-Host ""

# Define files to copy
$FilesToCopy = @(
    # Main executable
    @{ Source = $ExePath; Dest = "auric.exe" }
    
    # Required DLLs
    @{ Source = Join-Path $ReleaseDir "SDL3.dll"; Dest = "SDL3.dll" }
    @{ Source = Join-Path $ReleaseDir "yaml-cpp.dll"; Dest = "yaml-cpp.dll" }
    @{ Source = Join-Path $ReleaseDir "boost_log-vc143-mt-x64-1_90.dll"; Dest = "boost_log-vc143-mt-x64-1_90.dll" }
    @{ Source = Join-Path $ReleaseDir "boost_program_options-vc143-mt-x64-1_90.dll"; Dest = "boost_program_options-vc143-mt-x64-1_90.dll" }
    @{ Source = Join-Path $ReleaseDir "boost_thread-vc143-mt-x64-1_90.dll"; Dest = "boost_thread-vc143-mt-x64-1_90.dll" }
    @{ Source = Join-Path $ReleaseDir "boost_filesystem-vc143-mt-x64-1_90.dll"; Dest = "boost_filesystem-vc143-mt-x64-1_90.dll" }
)

# Copy binaries
Write-Host "Copying binaries..." -ForegroundColor Yellow
foreach ($file in $FilesToCopy) {
    if (Test-Path $file.Source) {
        Copy-Item $file.Source -Destination (Join-Path $StagingDir $file.Dest) -Force
        Write-Host "  + $($file.Dest)" -ForegroundColor Green
    } else {
        Write-Warning "Missing: $($file.Source)"
    }
}

# Copy config file
Write-Host ""
Write-Host "Copying configuration..." -ForegroundColor Yellow
if (Test-Path "auric.yaml") {
    Copy-Item "auric.yaml" -Destination $StagingDir -Force
    Write-Host "  + auric.yaml" -ForegroundColor Green
}

# Copy assets
Write-Host ""
Write-Host "Copying assets..." -ForegroundColor Yellow

$AssetDirs = @("images", "fonts")
foreach ($dir in $AssetDirs) {
    if (Test-Path $dir) {
        Copy-Item -Path $dir -Destination $StagingDir -Recurse -Force
        Write-Host "  + $dir/" -ForegroundColor Green
    }
}

# Create ROMS directory with README
Write-Host ""
Write-Host "Creating ROMS directory..." -ForegroundColor Yellow
$RomsDir = Join-Path $StagingDir "ROMS"
New-Item -ItemType Directory -Path $RomsDir -Force | Out-Null

if (Test-Path "ROMS\README.md") {
    Copy-Item "ROMS\README.md" -Destination $RomsDir -Force
    Write-Host "  + ROMS/README.md" -ForegroundColor Green
}

# Create README.txt for the release
Write-Host ""
Write-Host "Creating README.txt..." -ForegroundColor Yellow
$ReadmeContent = @"
Auric - Oric Computer Emulator
==============================

QUICK START
-----------

1. OBTAIN ROM FILES (REQUIRED)
   You need these ROM files to run the emulator:
   - basic10.rom    (Oric 1 ROM)
   - basic11b.rom   (Oric Atmos ROM)
   - microdis.rom   (Microdisc controller ROM)
   
   Place them in the ROMS/ folder.
   
   See ROMS/README.md for information on obtaining these legally.

2. RUN THE EMULATOR
   Double-click auric.exe or run from command line:
   
     auric.exe
     
   Command line options:
     auric.exe --help          Show all options
     auric.exe --zoom 4        Set window zoom (1-10)
     auric.exe --oric1         Use Oric 1 mode (default is Atmos)
     auric.exe --tape file.tap Load tape image
     auric.exe --disk file.dsk Load disk image

3. CONTROLS
   F1          Toggle main menu
   F2          Save snapshot
   F3          Load snapshot
   Ctrl+V      Paste clipboard text
   Ctrl+W      Toggle warp mode
   Ctrl+R      Reset (NMI)
   Ctrl+B      Break to debugger

DOCUMENTATION
-------------
- paste.md          Clipboard paste feature documentation
- README.md         Full project documentation

LICENSE
-------
This software is licensed under GPL v3.
See LICENSE file for details.

ROM files are copyrighted and must be obtained separately.
"@

$ReadmeContent | Out-File -FilePath (Join-Path $StagingDir "README.txt") -Encoding UTF8
Write-Host "  + README.txt" -ForegroundColor Green

# Create the ZIP archive
Write-Host ""
Write-Host "Creating archive..." -ForegroundColor Yellow
$ArchivePath = Join-Path (Get-Location) $ArchiveName

if (Test-Path $ArchivePath) {
    Remove-Item $ArchivePath -Force
}

Compress-Archive -Path "$StagingDir\*" -DestinationPath $ArchivePath -Force

# Clean up staging directory
Remove-Item $StagingDir -Recurse -Force

# Summary
$ArchiveSize = (Get-Item $ArchivePath).Length / 1MB
Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Release package created successfully!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "File: $ArchivePath" -ForegroundColor White
Write-Host "Size: $([math]::Round($ArchiveSize, 2)) MB" -ForegroundColor Gray
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Test the package: Expand-Archive $ArchiveName -DestinationPath test\" -ForegroundColor White
Write-Host "  2. Upload to GitHub Releases" -ForegroundColor White
Write-Host "     URL: https://github.com/dagfinndybvig/Auric/releases/new" -ForegroundColor White
Write-Host ""
