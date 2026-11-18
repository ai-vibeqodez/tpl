#!/usr/bin/env bash
# SCRIPT_VERSION=1.0.0
#
# Update CHANGELOG.md [Unreleased] from git log (Linux/macOS).
#
# Scans commits since the latest tag, groups them by conventional commit prefix,
# and writes them into ## [Unreleased] in CHANGELOG.md.
#
#   scripts/changelog-update.sh [--dry-run] [--force] [-h|--help]
#
#   --dry-run   Print what would be added without modifying files
#   --force     Overwrite [Unreleased] even if it already has entries
#   -h|--help   Show this help
#
# Conventional commit mapping:
#   feat(...)!:, breaking change  → ### Breaking
#   feat(...)                      → ### Added
#   fix(...)                       → ### Fixed
#   perf/re refactor/docs/style/chore/ci/build → ### Changed
#   test(...)                      → ### Fixed
#   revert(...)                    → ### Removed

set -euo pipefail

# ── Pre-flight: check for script updates (non-blocking) ──────────────
_updater="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/updater.sh"
if [ -x "$_updater" ]; then
    bash "$_updater" --check-only >/dev/null 2>&1 || {
        echo "[INFO]  New script updates available. Run: bash scripts/updater.sh" >&2
    }
fi
unset _updater

dry_run=0
force=0
for arg in "$@"; do
    case "$arg" in
        --dry-run) dry_run=1 ;;
        --force)   force=1 ;;
        -h|--help) sed -n '2,26p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "error: unknown argument '$arg' (try --help)" >&2; exit 2 ;;
    esac
done

die() { echo "error: $*" >&2; exit 1; }

cd "$(git rev-parse --show-toplevel)"
changelog="CHANGELOG.md"
[ -f "$changelog" ] || die "$changelog not found"

# Determine the previous tag (latest v* tag).
prev_tag="$(git tag -l 'v*' --sort=-version:refname | head -n1)"
if [ -n "$prev_tag" ]; then
    range="${prev_tag}..HEAD"
else
    range="HEAD"
fi

commits="$(git log --pretty=format:'%s' "$range" 2>/dev/null || true)"
[ -n "$commits" ] || { echo "No commits found in $range"; exit 0; }

# Associative arrays for categories and their display names.
declare -A cat_display
cat_display=([breaking]="Breaking" [feature]="Added" [fix]="Fixed" [change]="Changed" [remove]="Removed")

# Arrays indexed by category key.
declare -a cat_breaking=() cat_feature=() cat_fix=() cat_change=() cat_remove=()

# Skip list: ci/chore commits about infra, not user-visible.
skip_keywords=(workflow github action deps dependency format lint prettier eslint)

is_skip() {
    local line="$1"
    # Only skip ci/chore that match skip keywords
    [[ "$line" =~ ^(ci|chore)\( ]] || return 1
    for kw in "${skip_keywords[@]}"; do
        [[ "$line" == *"$kw"* ]] && return 0
    done
    return 1
}

clean_entry() {
    # Strip "feat(scope): " or "fix: " prefix
    echo "$1" | sed -E 's/^[a-z]+\([^)]*\): *//; s/^[a-z]+: *//'
}

add_entry() {
    local category="$1" subject="$2"
    local entry
    entry="$(clean_entry "$subject")"
    case "$category" in
        breaking) cat_breaking+=("$entry") ;;
        feature)  cat_feature+=("$entry") ;;
        fix)      cat_fix+=("$entry") ;;
        change)   cat_change+=("$entry") ;;
        remove)   cat_remove+=("$entry") ;;
    esac
}

while IFS= read -r line; do
    [ -z "$line" ] && continue

    local_type=""

    # Check for breaking change: !: suffix or BREAKING CHANGE footer
    re_breaking_scope='^[a-z]+\([^)]*\)!\: '
    re_breaking_plain='^[a-z]+!\: '
    if [[ "$line" =~ $re_breaking_scope ]] || [[ "$line" =~ $re_breaking_plain ]]; then
        local_type="breaking"
        # Normalize: remove ! before :
        line="${line//!:/ :}"
    fi
    if [[ "$line" == *"BREAKING CHANGE"* ]]; then
        local_type="breaking"
    fi

    # Skip infra-only ci/chore
    if is_skip "$line"; then continue; fi

    # Map conventional commit prefix (with or without scope)
    if [[ "$line" =~ ^feat\( ]] || [[ "$line" =~ ^feat\: ]]; then
        local_type="${local_type:+$local_type,}feature"
    elif [[ "$line" =~ ^fix\( ]] || [[ "$line" =~ ^fix\: ]]; then
        local_type="${local_type:+$local_type,}fix"
    elif [[ "$line" =~ ^(perf|refactor|docs|style|chore|ci|build)\( ]] || [[ "$line" =~ ^(perf|refactor|docs|style|chore|ci|build)\: ]]; then
        local_type="${local_type:+$local_type,}change"
    elif [[ "$line" =~ ^test\( ]] || [[ "$line" =~ ^test\: ]]; then
        local_type="${local_type:+$local_type,}fix"
    elif [[ "$line" =~ ^revert\( ]] || [[ "$line" =~ ^revert\: ]]; then
        local_type="${local_type:+$local_type,}remove"
    else
        local_type="${local_type:+$local_type,}change"
    fi

    # Add to each category
    IFS=',' read -ra types <<< "$local_type"
    for t in "${types[@]}"; do
        add_entry "$t" "$line"
    done
done <<< "$commits"

# --dry-run: print what would be added.
if [ "$dry_run" -eq 1 ]; then
    echo "=== What would be added to [Unreleased] ==="
    for key in breaking feature fix change remove; do
        local_display="${cat_display[$key]}"
        case "$key" in
            breaking) entries=("${cat_breaking[@]+"${cat_breaking[@]}"}") ;;
            feature)  entries=("${cat_feature[@]+"${cat_feature[@]}"}") ;;
            fix)      entries=("${cat_fix[@]+"${cat_fix[@]}"}") ;;
            change)   entries=("${cat_change[@]+"${cat_change[@]}"}") ;;
            remove)   entries=("${cat_remove[@]+"${cat_remove[@]}"}") ;;
        esac
        [ "${#entries[@]}" -eq 0 ] && continue
        echo ""
        echo "### $local_display"
        echo ""
        for e in "${entries[@]}"; do echo "- $e"; done
    done
    exit 0
fi

# Check if [Unreleased] already has entries (unless --force).
if [ "$force" -eq 0 ] && grep -q '^## \[Unreleased\]' "$changelog"; then
    unreleased_body="$(awk '/^## \[Unreleased\]/ { found=1; next } found && /^## \[/ { exit } found { print }' "$changelog")"
    if [ -n "$unreleased_body" ]; then
        die "[Unreleased] already has entries. Use --force to overwrite, or run release.sh to cut a release first."
    fi
fi

# Build section into a temp file.
section_file="$(mktemp)"
trap 'rm -f "$section_file"' EXIT

echo "## [Unreleased]" > "$section_file"

for key in breaking feature fix change remove; do
    local_display="${cat_display[$key]}"
    case "$key" in
        breaking) entries=("${cat_breaking[@]+"${cat_breaking[@]}"}") ;;
        feature)  entries=("${cat_feature[@]+"${cat_feature[@]}"}") ;;
        fix)      entries=("${cat_fix[@]+"${cat_fix[@]}"}") ;;
        change)   entries=("${cat_change[@]+"${cat_change[@]}"}") ;;
        remove)   entries=("${cat_remove[@]+"${cat_remove[@]}"}") ;;
    esac
    [ "${#entries[@]}" -eq 0 ] && continue

    echo "" >> "$section_file"
    echo "### $local_display" >> "$section_file"
    echo "" >> "$section_file"
    for e in "${entries[@]}"; do echo "- $e" >> "$section_file"; done
done

# Write to changelog.
work="$(mktemp)"
out="$(mktemp)"
trap 'rm -f "$work" "$out" "$section_file"' EXIT

# Handle line endings
crlf=0
if grep -q $'\r' "$changelog"; then crlf=1; fi
tr -d '\r' < "$changelog" > "$work"

if grep -q '^## \[Unreleased\]' "$work"; then
    # Replace existing [Unreleased] section (keep the heading line, replace body).
    awk -v secfile="$section_file" '
        /^## \[Unreleased\]/ { in_u = 1; print; next }
        in_u && /^## \[/ {
            in_u = 0
            # Print section body (skip the first line which is "## [Unreleased]" already printed)
            first = 1
            while ((getline line < secfile) > 0) { if (first) { first = 0; continue } print line }
            close(secfile)
            print ""
            print
            next
        }
        in_u { next }
        { print }
    ' "$work" > "$out"
else
    # Insert [Unreleased] after "# Changelog" header.
    awk -v secfile="$section_file" '
        /^# Changelog$/ { print; print ""; while ((getline line < secfile) > 0) print line; next }
        { print }
    ' "$work" > "$out"
fi

# Restore CRLF if needed.
if [ "$crlf" -eq 1 ]; then sed 's/$/\r/' "$out" > "$changelog"; else cp "$out" "$changelog"; fi

# Count total entries.
total=0
for key in breaking feature fix change remove; do
    case "$key" in
        breaking) n="${#cat_breaking[@]}" ;;
        feature)  n="${#cat_feature[@]}" ;;
        fix)      n="${#cat_fix[@]}" ;;
        change)   n="${#cat_change[@]}" ;;
        remove)   n="${#cat_remove[@]}" ;;
    esac
    total=$((total + n))
done
echo "Updated CHANGELOG.md: $total commit(s) added to [Unreleased]."
