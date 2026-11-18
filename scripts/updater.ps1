# SCRIPT_VERSION=1.0.0
<#
.SYNOPSIS
    Update all scripts from ai-warevo/.github (Windows; Linux/macOS: updater.sh).

.DESCRIPTION
    PowerShell wrapper that delegates to updater.sh via bash.
    Used by `make update` on Windows.

.PARAMETER Version
    Show version and exit.

.PARAMETER CheckOnly
    Check for updates without making changes.

.PARAMETER Help
    Show this help.
#>

param(
    [switch]$Version,
    [switch]$CheckOnly,
    [switch]$Help
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$updater = Join-Path $scriptDir "updater.sh"

if (-not (Test-Path $updater)) {
    Write-Error "updater.sh not found in $scriptDir"
    exit 1
}

$args = @()
if ($Version)  { $args += "--version" }
if ($CheckOnly) { $args += "--check-only" }
if ($Help)     { $args += "--help" }

& bash "$updater" $args
exit $LASTEXITCODE
