# import-gcx.ps1
# For all changed .csv files in *this* folder since the baseline tag,
# run the import script on the corresponding .gcx file.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-PythonCmd
{
    if (Get-Command py -ErrorAction SilentlyContinue)
    {
        return "py"
    }
    elseif (Get-Command python -ErrorAction SilentlyContinue)
    {
        return "python"
    }
    else
    {
        throw "Python not found on PATH."
    }
}

$python = Get-PythonCmd

# Locate repo root
$repoRoot = git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0 -or -not $repoRoot)
{
    throw "Failed to determine git repository root."
}

# Path to import script from repo root
$importScript = Join-Path $repoRoot "external\mgs_gcx_editor\_gcx_import_mgs2.py"
if (-not (Test-Path $importScript))
{
    throw "Import script not found at $importScript"
}

# Ensure baseline tag exists
Write-Host "Fetching baseline tag 'gcx-baseline'..."
git fetch origin tag gcx-baseline --no-tags --prune --depth=1 2>$null

# Ask git for changed files since the baseline tag
$changedFiles = git diff --name-only gcx-baseline HEAD
if ($LASTEXITCODE -ne 0)
{
    throw "Failed to run git diff. Ensure this is a git repo and the tag 'gcx-baseline' exists."
}

# Current folder path
$cwd = (Get-Location).ProviderPath

# Only keep CSVs whose parent is the current folder
$targetFiles = @()
foreach ($f in $changedFiles)
{
    if ($f -like "*.csv")
    {
        $fullCsv = Join-Path $repoRoot $f
        if ((Split-Path $fullCsv -Parent) -eq $cwd)
        {
            # Swap extension to .gcx
            $gcxPath = [System.IO.Path]::ChangeExtension($fullCsv, ".gcx")
            if (Test-Path $gcxPath)
            {
                $targetFiles += $gcxPath
            }
            else
            {
                Write-Host "Warning: no matching .gcx for $fullCsv" -ForegroundColor Yellow
            }
        }
    }
}


if (-not $targetFiles)
{
    Write-Host "No changed CSVs with matching GCX found since 'gcx-baseline'."
    exit 0
}

# Run the import script on each target
$idx = 0
$total = $targetFiles.Count
foreach ($file in $targetFiles)
{
    $idx++
    Write-Host "[$idx/$total] Importing $([System.IO.Path]::GetFileName($file))..."
    & $python $importScript $file
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "    Failed with exit code $LASTEXITCODE" -ForegroundColor Red
    }
    else
    {
        Write-Host "    OK" -ForegroundColor Green
    }
}

# Define special "US" filenames (for CI only)
$usFiles = @(
    "scenerio_stage_boss.gcx",
    "scenerio_stage_museum.gcx",
    "scenerio_stage_r_plt0.gcx",
    "scenerio_stage_r_plt10.gcx",
    "scenerio_stage_r_sna_b.gcx",
    "scenerio_stage_r_title.gcx",
    "scenerio_stage_r_tnk0.gcx",
    "scenerio_stage_r_vr_1.gcx",
    "scenerio_stage_r_vr_rp.gcx"
)

# Move finalized GCX files
if ($env:CI -eq "true")
{
    $euDest = Join-Path $repoRoot "dist\assets\gcx\eu\_bp"
    $usDest = Join-Path $repoRoot "dist\assets\gcx\us\_bp"

    foreach ($d in @($euDest, $usDest))
    {
        if (-not (Test-Path $d))
        {
            New-Item -ItemType Directory -Path $d -Force | Out-Null
        }
    }

    Write-Host "CI mode detected. Moving finalized GCX files..."
    foreach ($file in $targetFiles)
    {
        $name = [System.IO.Path]::GetFileName($file)
        if ($usFiles -contains $name)
        {
            $dest = Join-Path $usDest $name
            Write-Host "    US: $name -> $dest"
        }
        else
        {
            $dest = Join-Path $euDest $name
            Write-Host "    EU: $name -> $dest"
        }
        Move-Item -Path $file -Destination $dest -Force
    }
}
else
{
    $destDir = Split-Path $cwd -Parent
    Write-Host "Moving finalized GCX files to $destDir..."
    foreach ($file in $targetFiles)
    {
        $dest = Join-Path $destDir (Split-Path $file -Leaf)
        Move-Item -Path $file -Destination $dest -Force
        Write-Host "    Moved $file -> $dest"
    }
}
