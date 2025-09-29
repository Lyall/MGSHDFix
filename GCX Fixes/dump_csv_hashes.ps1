# dump-7z-crc32.ps1
# Uses 7-Zip's CRC32 calculation to dump all CSV file hashes in the current folder.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$sevenZip = (Get-Command 7z -ErrorAction SilentlyContinue)?.Source
if (-not $sevenZip) {
    throw "7z not found on PATH. Install 7-Zip and ensure '7z' is accessible."
}

$outFile = Join-Path (Get-Location) "actual-hashes.txt"
Remove-Item $outFile -ErrorAction SilentlyContinue

Get-ChildItem -Filter "*.csv" | Sort-Object Name | ForEach-Object {
    $out = & $sevenZip h -scrcCRC32 -- $_.FullName 2>&1

    # Find the data row: it starts with 8 hex chars (the CRC)
    $dataLine = $out | Where-Object { $_ -match '^[0-9A-Fa-f]{8}\s+' }
    if ($dataLine) {
        $parts = $dataLine -split '\s+'
        $crc   = $parts[0].ToUpper()
        "$($_.Name),$crc" | Out-File -FilePath $outFile -Append -Encoding ascii
        Write-Host "$($_.Name) => $crc"
    }
    else {
        Write-Host "Failed to parse CRC32 for $($_.Name)" -ForegroundColor Red
        Write-Host $out
    }
}

Write-Host "7-Zip CRC32 hashes written to $outFile"
