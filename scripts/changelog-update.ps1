<#
.SYNOPSIS
    Update CHANGELOG.md [Unreleased] from git log (Windows; Linux/macOS: scripts/changelog-update.sh).

.DESCRIPTION
    Scans commits since the latest tag, groups them by conventional commit prefix,
    and writes them into ## [Unreleased] in CHANGELOG.md.

.PARAMETER DryRun
    Print what would be added without modifying files.

.PARAMETER Force
    Overwrite [Unreleased] even if it already has entries.

.EXAMPLE
    .\scripts\changelog-update.ps1
    .\scripts\changelog-update.ps1 -DryRun
    .\scripts\changelog-update.ps1 -Force
#>
[CmdletBinding()]
param(
    [switch]$DryRun,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Fail([string]$Message) {
    [Console]::Error.WriteLine("error: $Message")
    exit 1
}

function Get-GitOutput {
    $out = & git @args
    if ($LASTEXITCODE -ne 0) { Fail "git $($args -join ' ') failed (exit code $LASTEXITCODE)" }
    return $out
}

$root = (& git rev-parse --show-toplevel).Trim()
Set-Location $root

$changelog = 'CHANGELOG.md'
if (-not (Test-Path -LiteralPath $changelog)) { Fail "$changelog not found" }

# Determine the previous tag.
$prevTag = (& git tag -l 'v*' --sort=-version:refname)[0]
if ($prevTag) {
    $range = "$prevTag..HEAD"
} else {
    $range = 'HEAD'
}

# Collect commit subjects.
$commits = (& git log --pretty=format:'%s' $range) -split "`n" | Where-Object { $_ }
if (-not $commits) { Write-Host "No commits found in $range"; exit 0 }

# Skip keywords for infra-only ci/chore commits.
$skipKeywords = @('workflow', 'github', 'action', 'deps', 'dependency', 'format', 'lint', 'prettier', 'eslint')

function ShouldSkip([string]$subject) {
    foreach ($p in @('ci', 'chore')) {
        if ($subject.StartsWith("$p(")) {
            foreach ($k in $skipKeywords) {
                if ($subject -ilike "*$k*") { return $true }
            }
        }
    }
    return $false
}

function Clean-Entry([string]$subject) {
    return $subject -replace '^[a-z]+\([^)]*\): *', '' -replace '^[a-z]+: *', ''
}

# Categories as ordered hashtable of arrays.
$categories = @{
    breaking = @()
    feature  = @()
    fix      = @()
    change   = @()
    remove   = @()
}
$catOrder = @('breaking', 'feature', 'fix', 'change', 'remove')

foreach ($line in $commits) {
    if (-not $line) { continue }

    $isBreaking = $false
    $cleanLine = $line

    # Check for breaking change: !: suffix
    if ($cleanLine -match '^[a-z]+\([^)]*\)!: ') {
        $isBreaking = $true
        $cleanLine = $cleanLine -replace '!:', ':'
    } elseif ($cleanLine -match '^[a-z]+!: ') {
        $isBreaking = $true
        $cleanLine = $cleanLine -replace '!: ', ': '
    }

    if ($cleanLine -match 'BREAKING CHANGE') {
        $isBreaking = $true
    }

    # Skip infra-only ci/chore
    if (ShouldSkip $cleanLine) { continue }

    $category = 'change'
    if ($cleanLine -match '^feat(\(|:)') { $category = 'feature' }
    elseif ($cleanLine -match '^fix(\(|:)') { $category = 'fix' }
    elseif ($cleanLine -match '^(perf|refactor|docs|style|chore|ci|build)(\(|:)') { $category = 'change' }
    elseif ($cleanLine -match '^test(\(|:)') { $category = 'fix' }
    elseif ($cleanLine -match '^revert(\(|:)') { $category = 'remove' }

    $entry = Clean-Entry $cleanLine
    if ($isBreaking) {
        $categories.breaking += $entry
    } else {
        $categories[$category] += $entry
    }
}

# --dry-run: print what would be added.
if ($DryRun) {
    Write-Host "=== What would be added to [Unreleased] ==="
    $catDisplay = @{
        breaking = 'Breaking'; feature = 'Added'; fix = 'Fixed'
        change = 'Changed'; remove = 'Removed'
    }
    foreach ($key in $catOrder) {
        $entries = $categories[$key]
        if (-not $entries -or $entries.Count -eq 0) { continue }
        Write-Host ""
        Write-Host "### $($catDisplay[$key])"
        Write-Host ""
        foreach ($e in $entries) { Write-Host "- $e" }
    }
    exit 0
}

# Read the changelog early (needed for body check and replacement below).
$raw = [System.IO.File]::ReadAllText((Join-Path $root $changelog))
$crlf = $raw.Contains("`r`n")
$lines = @($raw -split "\r?\n")
if ($lines.Count -gt 0 -and $lines[$lines.Count - 1] -eq '') { $lines = $lines[0..($lines.Count - 2)] }

# Find the ## [Unreleased] section boundaries.
$hasUnreleased = $false
$startIdx = -1
$endIdx = $lines.Count
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^## \[Unreleased\]') { $startIdx = $i; $hasUnreleased = $true; break }
}
if ($startIdx -ge 0) {
    for ($i = $startIdx + 1; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^## \[') { $endIdx = $i; break }
        if ($lines[$i] -match '^\[[^]]+\]:') { $endIdx = $i; break }
    }
}

# Check if [Unreleased] already has entries (non-blank, non-comment lines between heading and next section).
if ($hasUnreleased -and -not $Force) {
    $hasBody = $false
    for ($i = $startIdx + 1; $i -lt $endIdx; $i++) {
        if ($lines[$i] -notmatch '^\s*$' -and $lines[$i] -notmatch '^<!--') { $hasBody = $true; break }
    }
    if ($hasBody) {
        Fail '[Unreleased] already has entries. Use -Force to overwrite, or run release.sh to cut a release first.'
    }
}

# Build the [Unreleased] section content.
$sectionLines = @('## [Unreleased]')

$catDisplay = @{
    breaking = 'Breaking'; feature = 'Added'; fix = 'Fixed'
    change = 'Changed'; remove = 'Removed'
}

foreach ($key in $catOrder) {
    $entries = $categories[$key]
    if (-not $entries -or $entries.Count -eq 0) { continue }

    $sectionLines += ''
    $sectionLines += "### $($catDisplay[$key])"
    $sectionLines += ''
    foreach ($e in $entries) { $sectionLines += "- $e" }
}

$sectionContent = ($sectionLines -join "`n") + "`n"

$result = @()

if ($hasUnreleased) {
    # Replace existing [Unreleased] section.
    $result += $lines[0..($startIdx)]
    $result += ($sectionContent -split "`n")
    $result += $lines[$endIdx..($lines.Count - 1)]
} else {
    # Insert after "# Changelog" header.
    $inserted = $false
    foreach ($line in $lines) {
        $result += $line
        if (-not $inserted -and $line -match '^# Changelog$') {
            $result += ''
            $result += ($sectionContent -split "`n")
            $inserted = $true
        }
    }
}

# Write back.
$newline = if ($crlf) { "`r`n" } else { "`n" }
$text = ($result -join $newline) + $newline
[System.IO.File]::WriteAllText((Join-Path $root $changelog), $text, (New-Object System.Text.UTF8Encoding($false)))

$total = 0
foreach ($key in $catOrder) { $total += ($categories[$key]).Count }
Write-Host "Updated CHANGELOG.md: $total commit(s) added to [Unreleased]."
