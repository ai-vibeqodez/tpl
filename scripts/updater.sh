#!/usr/bin/env bash
# Sync scripts from ai-warevo/.github to the local project (Linux/macOS).
#
#   scripts/updater.sh [--version] [--check-only] [-h|--help]
#
#   --version      Show this script's version and exit
#   --check-only   Check for updates without making changes (exit 0=up-to-date, 1=update available)
#   -h|--help      Show this help
#
# Downloads all files from the scripts/ directory of the GitHub repo and
# overwrites local copies when they differ. Asks for consent before each
# overwrite.  Also checks itself (this script) against the remote.
#
# SCRIPT_VERSION is auto-updated during the sync — never edit it manually.

SCRIPT_VERSION=1.0.0

set -euo pipefail

# ── Globals ──────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

REMOTE_BASE="https://raw.githubusercontent.com/ai-warevo/.github/refs/heads/main/scripts"
REMOTE_SCRIPTS_DIR="$REMOTE_BASE"

# ── Argument parsing ─────────────────────────────────────────────────
show_version=0
check_only=0
for arg in "$@"; do
    case "$arg" in
        --version)     show_version=1 ;;
        --check-only)  check_only=1 ;;
        -h|--help)     sed -n '2,13p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "[ERROR] unknown argument '$arg' (try --help)" >&2; exit 1 ;;
    esac
done

if [ "$show_version" -eq 1 ]; then
    echo "updater.sh v${SCRIPT_VERSION}"
    exit 0
fi

# ── Logging helpers ──────────────────────────────────────────────────
log_info()  { echo "[INFO]  $*"; }
log_warn()  { echo "[WARN]  $*"; }
log_error() { echo "[ERROR] $*" >&2; }
log_ok()    { echo "[OK]    $*"; }

# ── HTTP helpers ─────────────────────────────────────────────────────
# Download a remote file to stdout.  Returns 1 on failure (no internet / 404).
fetch_remote() {
    local url="$1"
    curl -fsSL --connect-timeout 5 --max-time 15 "$url" 2>/dev/null || return 1
}

# ── SemVer comparison ────────────────────────────────────────────────
# Returns 0 if $1 > $2, 1 otherwise.
version_gt() {
    # Split into arrays.
    IFS='.' read -r -a a <<< "$1"
    IFS='.' read -r -a b <<< "$2"

    local max=${#a[@]}
    [ ${#b[@]} -gt "$max" ] && max=${#b[@]}

    for ((i = 0; i < max; i++)); do
        local va="${a[$i]:-0}"
        local vb="${b[$i]:-0}"
        if ((va > vb)); then return 0; fi
        if ((va < vb)); then return 1; fi
    done
    return 1  # equal → not greater
}

# ── Parse SCRIPT_VERSION from a script's content ─────────────────────
parse_version() {
    local content="$1"
    echo "$content" | grep -oP 'SCRIPT_VERSION=\K[0-9]+\.[0-9]+\.[0-9]+' || true
}

# ── Consent ──────────────────────────────────────────────────────────
ask_consent() {
    local file="$1"
    local action="${2:-overwrite}"
    printf "[CONSENT] %s %s? [Y/n] " "$action" "$file"
    local reply
    IFS= read -r reply || reply="y"
    reply="${reply,,}"  # lowercase
    [ -z "$reply" ] || [ "$reply" = "y" ] || [ "$reply" = "yes" ]
}

# ── Conflict detection ───────────────────────────────────────────────
# Returns 0 if files differ, 1 if identical.
files_differ() {
    local local_file="$1" remote_content="$2"
    diff --quiet "$local_file" <(echo "$remote_content") >/dev/null 2>&1
    return $?  # diff returns 0 = same, 1 = different
}

# ── Main logic ───────────────────────────────────────────────────────

log_info "updater.sh v${SCRIPT_VERSION} — starting"
log_info "Repository root: ${REPO_ROOT}"

# 1. Check internet connectivity by fetching the remote list of scripts.
log_info "Checking remote repository…"
remote_files="$(fetch_remote "${REMOTE_SCRIPTS_DIR}/" 2>/dev/null || true)"

# Try to detect actual file names from raw URL listing (simple heuristic:
# raw.githubusercontent.com doesn't index directories, so we enumerate
# known script names and probe each one).
known_scripts=(
    "changelog-update.sh"
    "changelog-update.ps1"
    "release.sh"
    "release.ps1"
    "updater.sh"
)

declare -a available_scripts=()
for script in "${known_scripts[@]}"; do
    if fetch_remote "${REMOTE_SCRIPTS_DIR}/${script}" >/dev/null 2>&1; then
        available_scripts+=("$script")
    fi
done

if [ ${#available_scripts[@]} -eq 0 ]; then
    log_warn "Cannot reach remote scripts repository (no internet or scripts/ is empty)"
    log_warn "Skipping update — local scripts are unchanged"
    exit 0
fi

log_info "Found ${#available_scripts[@]} script(s) on remote: ${available_scripts[*]}"

# 2. Self-check: is updater.sh itself outdated?
self_remote="$(fetch_remote "${REMOTE_SCRIPTS_DIR}/updater.sh")" || true
if [ -n "$self_remote" ]; then
    self_remote_version="$(parse_version "$self_remote")"
    if [ -n "$self_remote_version" ]; then
        log_info "Local version : ${SCRIPT_VERSION}"
        log_info "Remote version: ${self_remote_version}"

        if version_gt "$self_remote_version" "$SCRIPT_VERSION"; then
            log_info "New version available: ${self_remote_version}"
        else
            log_info "Already up to date (v${SCRIPT_VERSION})"
        fi
    else
        log_warn "SCRIPT_VERSION not found in remote updater.sh — skipping self-check"
    fi
else
    log_info "Remote updater.sh not yet available (first deploy?) — skipping self-check"
fi

# 3. Sync each script.
updated=0
skipped=0
errors=0

for script in "${available_scripts[@]}"; do
    log_info "Processing ${script}…"

    local_file="${SCRIPT_DIR}/${script}"
    remote_content="$(fetch_remote "${REMOTE_SCRIPTS_DIR}/${script}")" || {
        log_error "Failed to download ${script} from remote"
        ((errors++)) || true
        continue
    }

    remote_version="$(parse_version "$remote_content")"

    # If local file doesn't exist, always sync.
    if [ ! -f "$local_file" ]; then
        if [ "$check_only" -eq 1 ]; then
            log_info "  [check-only] ${script} is missing locally — would create"
            updated=$((updated + 1))
        else
            if ask_consent "$script" "create"; then
                echo "$remote_content" > "$local_file"
                log_ok "Created ${script} (v${remote_version})"
                updated=$((updated + 1))
            else
                log_info "  Skipped (user declined)"
                skipped=$((skipped + 1))
            fi
        fi
        continue
    fi

    # Local file exists — check if it differs.
    if files_differ "$local_file" "$remote_content"; then
        local_version="$(parse_version "$(cat "$local_file")")"
        if [ "$check_only" -eq 1 ]; then
            log_info "  [check-only] ${script} differs — local: ${local_version:-?}, remote: ${remote_version:-?}"
            updated=$((updated + 1))
        else
            if ask_consent "$script" "overwrite"; then
                echo "$remote_content" > "$local_file"
                log_ok "Updated ${script} (${local_version:-?} → ${remote_version:-?})"
                updated=$((updated + 1))
            else
                log_info "  Skipped (user declined)"
                skipped=$((skipped + 1))
            fi
        fi
    else
        log_info "  ${script} is up to date"
    fi
done

# 4. Summary.
echo ""
log_info "Summary: ${updated} updated, ${skipped} skipped, ${errors} errors"

if [ "$check_only" -eq 1 ]; then
    if [ "$updated" -gt 0 ]; then
        log_info "Updates available — run without --check-only to apply"
        exit 1
    else
        log_info "All scripts are up to date"
        exit 0
    fi
fi

if [ "$errors" -gt 0 ]; then
    log_error "Completed with ${errors} error(s)"
    exit 1
fi

log_ok "All scripts synced successfully"
exit 0
