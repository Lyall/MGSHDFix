# export-gcx.ps1
# Scans the current directory for .gcx files and runs:
#   py ../external/mgs_gcx_editor/_gcx_export_mgs2.py <file.gcx>
# for each one. Falls back to "python" if "py" is not available.

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

# Resolve the editor script path relative to the current directory.
$scriptRel = Join-Path -Path (Get-Location) -ChildPath "..\external\mgs_gcx_editor\_gcx_export_mgs2.py"
$scriptPath = Resolve-Path -LiteralPath $scriptRel -ErrorAction SilentlyContinue
if (-not $scriptPath)
{
    throw "Could not find editor script at ../external/mgs_gcx_editor/_gcx_export_mgs2.py relative to $(Get-Location)."
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

    # Call Python with fully qualified paths. Use call operator to preserve exit codes.
    & $python $scriptPath.Path $f.FullName
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
