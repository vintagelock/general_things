param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath
)

# Check if file exists
if (-not (Test-Path $FilePath)) {
    Write-Error "File not found: $FilePath"
    exit 1
}

# Check if file has .js extension
if (-not $FilePath.EndsWith(".js")) {
    Write-Error "File must have .js extension"
    exit 1
}

try {
    # Read the file content
    Write-Host "Reading file: $FilePath"
    $content = Get-Content -Path $FilePath -Raw -Encoding UTF8
    
    # Define emoji regex pattern using .NET regex class for Unicode support
    $emojiPattern = @(
        # Emoticons (U+1F600-U+1F64F)
        '\uD83D[\uDE00-\uDE4F]',
        # Miscellaneous Symbols and Pictographs (U+1F300-U+1F5FF)
        '\uD83C[\uDF00-\uDFFF]|\uD83D[\uDC00-\uDDFF]',
        # Transport and Map Symbols (U+1F680-U+1F6FF)
        '\uD83D[\uDE80-\uDEFF]',
        # Supplemental Symbols and Pictographs (U+1F900-U+1F9FF)
        '\uD83E[\uDD00-\uDDFF]',
        # Additional emoji ranges
        '[\u2600-\u26FF]',  # Miscellaneous Symbols
        '[\u2700-\u27BF]',  # Dingbats
        '[\uFE00-\uFE0F]',  # Variation Selectors
        '\uD83C[\uDDE0-\uDDFF]', # Regional Indicator Symbols
        # Common single-character emojis
        '[\u203C\u2049\u20E3\u2122\u2139\u2194-\u2199\u21A9-\u21AA\u231A-\u231B\u2328\u23CF\u23E9-\u23F3\u23F8-\u23FA\u24C2\u25AA-\u25AB\u25B6\u25C0\u25FB-\u25FE\u2934-\u2935\u2B05-\u2B07\u2B1B-\u2B1C\u2B50\u2B55\u3030\u303D\u3297\u3299]'
    ) -join '|'
    
    # Remove emojis from content
    Write-Host "Removing emoji characters..."
    $cleanContent = $content -replace $emojiPattern, ''
    
    # Generate output filename
    $directory = Split-Path $FilePath -Parent
    $filename = Split-Path $FilePath -LeafBase
    $outputPath = Join-Path $directory "${filename}_noemoji.js"
    
    # Save the cleaned content
    Write-Host "Saving cleaned file: $outputPath"
    $cleanContent | Out-File -FilePath $outputPath -Encoding UTF8 -NoNewline
    
    # Report results
    $originalSize = (Get-Item $FilePath).Length
    $cleanSize = (Get-Item $outputPath).Length
    $removedBytes = $originalSize - $cleanSize
    
    Write-Host "Successfully processed file!" -ForegroundColor Green
    Write-Host "Original size: $originalSize bytes"
    Write-Host "Cleaned size: $cleanSize bytes"
    Write-Host "Removed: $removedBytes bytes"
    Write-Host "Output file: $outputPath"
    
}
catch {
    Write-Error "Error processing file: $($_.Exception.Message)"
    exit 1
}