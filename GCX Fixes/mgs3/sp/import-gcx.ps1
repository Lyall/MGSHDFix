# import-gcx.ps1
# For all .csv files in this script's folder whose CRC32 does not match mgs3-gcx-hashes.txt,
# run the import script on the corresponding .gcx file.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-PythonCmd {
    if (Get-Command py -ErrorAction SilentlyContinue) { return "py" }
    elseif (Get-Command python -ErrorAction SilentlyContinue) { return "python" }
    else { throw "Python not found on PATH." }
}

function Get-Crc32-7z([string]$path) {
    $sevenZip = Get-Command 7z -ErrorAction SilentlyContinue
    if (-not $sevenZip) {
        throw "7z not found on PATH. Install 7-Zip and ensure '7z' is accessible."
    }

    $out = & $sevenZip h -scrcCRC32 -- $path 2>&1

    # Grab the first line that starts with 8 hex chars AND ends with the filename
    $fileName = [System.IO.Path]::GetFileName($path)
    $line = $out | Where-Object { $_ -match "^[0-9A-Fa-f]{8}\s+\d+\s+$fileName$" }

    if (-not $line) {
        throw "Failed to parse CRC32 for $path. 7z output:`n$out"
    }

    return ($line -split '\s+')[0].ToUpper()
}

$python   = Get-PythonCmd
$repoRoot = git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0 -or -not $repoRoot) { throw "Failed to determine git repository root." }

$importScript = Join-Path $repoRoot "external\mgs_gcx_editor\_gcx_import_mgs3.py"
if (-not (Test-Path $importScript)) { throw "Import script not found at $importScript" }

# Paths anchored to script location
$scriptDir = $PSScriptRoot
$hashFile  = Join-Path $scriptDir "mgs3-gcx-hashes.txt"
if (-not (Test-Path $hashFile)) { throw "Missing hash file: $hashFile" }

# Load expected hashes
$expected = @{}
Get-Content $hashFile | ForEach-Object {
    if ($_ -match "^\s*([^,]+),([0-9A-Fa-f]{8})\s*$") {
        $expected[$matches[1]] = $matches[2].ToUpper()
    }
}
Write-Host "Checking hashes with 7z" -ForegroundColor Yellow
# Find changed CSVs
$targetFiles = @()
Get-ChildItem -LiteralPath $scriptDir -File -Filter "*.csv" | ForEach-Object {
    $name = $_.Name
    if ($expected.ContainsKey($name)) {
        $crc = Get-Crc32-7z $_.FullName
        if ($crc -ne $expected[$name]) {
            $gcxPath = [System.IO.Path]::ChangeExtension($_.FullName, ".gcx")
            if (Test-Path $gcxPath) {
                Write-Host "Detected change: $name (expected $($expected[$name]), got $crc)" -ForegroundColor Yellow
                $targetFiles += $gcxPath
            } else {
                Write-Host "Warning: no matching .gcx for $name" -ForegroundColor Yellow
            }
        }
    } else {
        Write-Host "Warning: no expected hash for $name" -ForegroundColor Yellow
    }
}

if (-not $targetFiles) {
    Write-Host "No changed CSVs with matching GCX found." -ForegroundColor Green
    exit 0
}

# Run imports
$idx = 0
$total = $targetFiles.Count
foreach ($file in $targetFiles) {
    $idx++
    Write-Host "[$idx/$total] Importing $([System.IO.Path]::GetFileName($file))..."
    & $python $importScript $file
    if ($LASTEXITCODE -ne 0) {
        Write-Host "    Failed with exit code $LASTEXITCODE" -ForegroundColor Red
    } else {
        Write-Host "    OK" -ForegroundColor Green
    }
}

# CI move logic
$usFiles = @(
    "scenerio_stage_boss.gcx","scenerio_stage_museum.gcx","scenerio_stage_r_plt0.gcx",
    "scenerio_stage_r_plt10.gcx","scenerio_stage_r_sna_b.gcx","scenerio_stage_r_title.gcx",
    "scenerio_stage_r_tnk0.gcx","scenerio_stage_r_vr_1.gcx","scenerio_stage_r_vr_rp.gcx"
)

if ($env:CI -eq "true") {
    $euDest = Join-Path $repoRoot "dist\assets\gcx\sp\_bp"
    $usDest = Join-Path $repoRoot "dist\assets\gcx\sp\_bp"
    foreach ($d in @($euDest,$usDest)) {
        if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }
    }
    Write-Host "CI mode detected. Moving finalized GCX files..."
    foreach ($file in $targetFiles) {
        $name = [System.IO.Path]::GetFileName($file)
        if ($usFiles -contains $name) {
            $dest = Join-Path $usDest $name
            Write-Host "    SP: $name -> $dest"
        } else {
            $dest = Join-Path $euDest $name
            Write-Host "    SP: $name -> $dest"
        }
        Move-Item -Path $file -Destination $dest -Force
    }
} else {
    $destDir = Split-Path $scriptDir -Parent
    Write-Host "Moving finalized GCX files to $destDir..."
    foreach ($file in $targetFiles) {
        $dest = Join-Path $destDir (Split-Path $file -Leaf)
        Move-Item -Path $file -Destination $dest -Force
        Write-Host "    Moved $file -> $dest"
    }
}
