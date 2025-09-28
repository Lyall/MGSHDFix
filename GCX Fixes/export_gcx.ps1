# export-gcx.ps1
# Scans the current directory for .gcx files and runs:
#   py <repoRoot>/external/mgs_gcx_editor/_gcx_export_mgs2.py <file.gcx>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

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
        throw "Python launcher not found. Install Python or ensure 'py' or 'python' is on PATH."
    }
}

# Resolve repo root
$repoRoot = git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0 -or -not $repoRoot)
{
    throw "Failed to determine git repository root. Are you inside a git repo?"
}

# Path to export script from repo root
$scriptPath = Join-Path $repoRoot "external\mgs_gcx_editor\_gcx_export_mgs2.py"
if (-not (Test-Path $scriptPath))
{
    throw "Could not find editor script at $scriptPath"
}

# Collect .gcx files in the current directory only.
$gcxFiles = @(Get-ChildItem -LiteralPath (Get-Location) -File -Filter "*.gcx" | Sort-Object Name)

if ($gcxFiles.Count -eq 0)
{
    Write-Host "No .gcx files found in $(Get-Location)." -ForegroundColor Yellow
    exit 0
}

$python = Get-PythonCmd

$fail = 0
$total = $gcxFiles.Count
$idx = 0

foreach ($f in $gcxFiles)
{
    $idx++
    Write-Host "[$idx/$total] Exporting '$($f.Name)'" 

    & $python $scriptPath $f.FullName
    $code = $LASTEXITCODE

    if ($code -ne 0)
    {
        Write-Host "    Failed with exit code $code" -ForegroundColor Red
        $fail++
    }
    else
    {
        Write-Host "    OK" -ForegroundColor Green
    }
}

if ($fail -gt 0)
{
    Write-Host "$fail of $total exports failed." -ForegroundColor Red
    exit 1
}
else
{
    Write-Host "All $total exports succeeded." -ForegroundColor Green
    exit 0
}
