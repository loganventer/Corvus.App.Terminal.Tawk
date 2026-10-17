#!/bin/sh
# migrate-from-legacy.sh: copy a tawk install's per-user files to the paths
# corvus-tawk uses. See CUTOVER.md for the whole switch-over.
#
#   sh scripts/migrate-from-legacy.sh [--dry-run] [--force]
#
#   --dry-run   print what would be done and change nothing
#   --force     replace corvus/tawk folders that already exist (each one is
#               kept aside as <folder>.before-migrate-<time>), and run even
#               while the old tawk is running (not safe for the databases)
#   -h, --help  show this help
#
# What it does (it copies, never moves; the old folders are left as they are):
#   ~/.config/tawk       -> ~/.config/corvus/tawk        settings, your themes and sounds, db.key
#   ~/.local/share/tawk  -> ~/.local/share/corvus/tawk   chats (tawk.db with -wal/-shm) and the
#                                                        WhatsApp login (auth/, accounts/N/auth/)
#   ~/.local/state/tawk  -> ~/.local/state/corvus/tawk   logs
#   ~/.cache/tawk        -> ~/.cache/corvus/tawk         downloaded media
# Each base follows XDG_CONFIG_HOME, XDG_DATA_HOME, XDG_STATE_HOME and
# XDG_CACHE_HOME when they are set. The runtime folder ($XDG_RUNTIME_DIR/tawk:
# control.sock, admin.token) is not copied: corvus-tawk makes its own in
# $XDG_RUNTIME_DIR/corvus/tawk when it starts.
#
# Paths inside the copies that name the old folders are pointed at the new
# ones: the folder settings in config.ini (data_dir, media_dir, attach_dir,
# sound_file, sidecar_dir, ...) and the media paths in tawk.db (when the
# sqlite3 command is installed and the database is not encrypted). The
# WhatsApp login is copied byte for byte, so this computer stays the same
# linked device and is not asked to link again.
#
# POSIX sh, and safe to run again: a folder it already copied is reported and
# left alone. It exits 0 when everything is in place, 1 when something was
# refused or failed, 2 for a usage error.
set -u

DRY_RUN=0
FORCE=0
for arg in "$@"; do
    case "$arg" in
        --dry-run|-n) DRY_RUN=1 ;;
        --force|-f)   FORCE=1 ;;
        -h|--help)    awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"; exit 0 ;;
        *)            printf 'migrate-from-legacy: unknown option %s (see --help)\n' "$arg" >&2; exit 2 ;;
    esac
done

umask 077
MARK=".migrated-from-legacy"
STAMP="$(date +%Y%m%d-%H%M%S)"
PROBLEMS=0

say()  { printf '%s\n' "$*"; }
warn() { printf '!! %s\n' "$*" >&2; }
plan() { if [ "$DRY_RUN" -eq 1 ]; then printf '   would: %s\n' "$*"; else printf '   %s\n' "$*"; fi; }

home="${HOME:?HOME is not set}"
abs_or() { case "${1:-}" in /*) printf '%s' "$1" ;; *) printf '%s' "$2" ;; esac; }
CONFIG_BASE="$(abs_or "${XDG_CONFIG_HOME:-}" "$home/.config")"
DATA_BASE="$(abs_or "${XDG_DATA_HOME:-}" "$home/.local/share")"
STATE_BASE="$(abs_or "${XDG_STATE_HOME:-}" "$home/.local/state")"
CACHE_BASE="$(abs_or "${XDG_CACHE_HOME:-}" "$home/.cache")"

OLD_CONFIG="$CONFIG_BASE/tawk"; NEW_CONFIG="$CONFIG_BASE/corvus/tawk"
OLD_DATA="$DATA_BASE/tawk";     NEW_DATA="$DATA_BASE/corvus/tawk"
OLD_STATE="$STATE_BASE/tawk";   NEW_STATE="$STATE_BASE/corvus/tawk"
OLD_CACHE="$CACHE_BASE/tawk";   NEW_CACHE="$CACHE_BASE/corvus/tawk"
if [ -n "${XDG_RUNTIME_DIR:-}" ] && [ "${XDG_RUNTIME_DIR#/}" != "$XDG_RUNTIME_DIR" ]; then
    OLD_RUNTIME="$XDG_RUNTIME_DIR/tawk"; NEW_RUNTIME="$XDG_RUNTIME_DIR/corvus/tawk"
else
    OLD_RUNTIME="$OLD_STATE"; NEW_RUNTIME="$NEW_STATE"
fi

say "corvus-tawk: migrate from tawk$( [ "$DRY_RUN" -eq 1 ] && printf ' (dry run: nothing is changed)')"

# ---- is the old tawk still running? ----------------------------------------

running=""
if command -v pgrep >/dev/null 2>&1 && pgrep -x tawk >/dev/null 2>&1; then
    running="a process named tawk is running (pid $(pgrep -x tawk | tr '\n' ' ' | sed 's/ $//'))"
fi
if [ -z "$running" ] && [ -f "$OLD_DATA/tawk.lock" ] && command -v flock >/dev/null 2>&1; then
    flock -n -E 75 "$OLD_DATA/tawk.lock" true 2>/dev/null
    [ $? -eq 75 ] && running="$OLD_DATA/tawk.lock is held by a running tawk"
fi
if [ -z "$running" ] && [ -S "$OLD_RUNTIME/control.sock" ]; then
    running="its control socket $OLD_RUNTIME/control.sock is still there (tawk may be running)"
fi
if [ -n "$running" ]; then
    if [ "$DRY_RUN" -eq 1 ] && [ "$FORCE" -eq 0 ]; then
        warn "tawk looks like it is running: $running."
        warn "The real run will refuse until it is quit; showing the plan anyway."
    elif [ "$FORCE" -eq 1 ]; then
        warn "tawk looks like it is running: $running."
        warn "--force: copying anyway. A database copied while it is being written can be inconsistent."
    else
        warn "tawk looks like it is running: $running."
        warn "Quit it first (Q, or /quit), then run this again. --force copies anyway, which is not safe for the databases."
        exit 1
    fi
fi

# ---- copy one folder ---------------------------------------------------------

# copy_dir OLD NEW WHAT: copies OLD to NEW with modes kept, through a
# temporary folder next to NEW, so NEW is either complete or absent.
copy_dir() {
    old="$1"; new="$2"; what="$3"
    say ""
    say "$what: $old -> $new"
    if [ ! -d "$old" ]; then
        say "   nothing to copy ($old does not exist)"
        return 0
    fi
    if [ -e "$new" ]; then
        if [ -f "$new/$MARK" ] && [ "$FORCE" -eq 0 ]; then
            say "   already copied ($(sed -n 1p "$new/$MARK" 2>/dev/null)); left as it is"
            return 0
        fi
        if [ "$FORCE" -eq 0 ]; then
            warn "$new already exists; not overwritten. Look at it, then remove it or run with --force."
            PROBLEMS=$((PROBLEMS + 1))
            return 1
        fi
        plan "keep the existing $new aside as $new.before-migrate-$STAMP"
        if [ "$DRY_RUN" -eq 0 ]; then
            mv "$new" "$new.before-migrate-$STAMP" || { warn "could not move $new aside"; PROBLEMS=$((PROBLEMS + 1)); return 1; }
        fi
    fi
    parent="$(dirname "$new")"
    plan "create $parent (mode 0700) if missing"
    plan "copy $old to $new, keeping file modes (cp -Rp), SQLite -wal and -shm files included"
    [ "$DRY_RUN" -eq 1 ] && return 0
    if [ ! -d "$parent" ]; then mkdir -p "$parent" && chmod 0700 "$parent"; fi   # an existing corvus/ keeps its mode
    tmp="$new.migrating-$$"
    rm -rf "$tmp"
    if ! cp -Rp "$old" "$tmp"; then
        warn "copying $old failed; nothing was put at $new"
        rm -rf "$tmp"
        PROBLEMS=$((PROBLEMS + 1))
        return 1
    fi
    rm -f "$tmp/tawk.lock" "$tmp/control.sock" "$tmp/admin.token"   # belong to a running instance, made again on start
    if ! mv "$tmp" "$new"; then
        warn "could not put the copy at $new"
        rm -rf "$tmp"
        PROBLEMS=$((PROBLEMS + 1))
        return 1
    fi
    printf 'copied from %s on %s by scripts/migrate-from-legacy.sh\n' "$old" "$STAMP" > "$new/$MARK"
    chmod 0600 "$new/$MARK"
    say "   done"
    return 0
}

copy_dir "$OLD_CONFIG" "$NEW_CONFIG" "config"; config_rc=$?
copy_dir "$OLD_DATA"   "$NEW_DATA"   "data (chats and the WhatsApp login)"; data_rc=$?
copy_dir "$OLD_STATE"  "$NEW_STATE"  "state (logs)"
copy_dir "$OLD_CACHE"  "$NEW_CACHE"  "cache (downloaded media)"

# ---- the WhatsApp login -----------------------------------------------------

check_login() {
    say ""
    say "WhatsApp login:"
    found=0
    for db in "$OLD_DATA/auth/whatsmeow.db" "$OLD_DATA"/accounts/*/auth/whatsmeow.db; do
        [ -f "$db" ] || continue
        found=1
        rel="${db#"$OLD_DATA"/}"
        if [ "$DRY_RUN" -eq 1 ]; then plan "copy $rel (with its -wal and -shm) byte for byte"; continue; fi
        same=1
        for part in "" -wal -shm; do
            [ -f "$db$part" ] || continue
            if ! cmp -s "$db$part" "$NEW_DATA/$rel$part"; then
                warn "$NEW_DATA/$rel$part differs from $db$part (tawk ran after the copy?); run again with --force to copy afresh"
                PROBLEMS=$((PROBLEMS + 1))
                same=0
            fi
        done
        [ "$same" -eq 1 ] && say "   $rel: copied and checked byte for byte"
    done
    for creds in "$OLD_DATA/auth/creds.json" "$OLD_DATA"/accounts/*/auth/creds.json; do
        [ -f "$creds" ] || continue
        found=1
        plan "${creds#"$OLD_DATA"/} (Node.js backend login): copied with its folder"
    done
    [ "$found" -eq 0 ] && say "   no login found in $OLD_DATA (not linked yet, or kept elsewhere)"
    return 0
}
[ "$data_rc" -eq 0 ] && check_login

# ---- paths inside the copies ------------------------------------------------

# Escapes a string for use as a literal in a sed pattern or replacement.
sed_lit() { printf '%s' "$1" | sed 's/[][\.*^$/&|]/\\&/g'; }

rewrite_config() {
    ini="$NEW_CONFIG/config.ini"
    say ""
    say "config.ini folder settings:"
    if [ "$DRY_RUN" -eq 1 ]; then
        [ -f "$OLD_CONFIG/config.ini" ] || { say "   no config.ini"; return 0; }
        grep -nE "=[[:space:]]*($OLD_DATA|$OLD_CACHE|$OLD_STATE|$OLD_CONFIG)(/|\$)|/share/tawk/|/lib/tawk/" "$OLD_CONFIG/config.ini" 2>/dev/null \
            | sed 's/^/   would point at the new folders: line /'
        return 0
    fi
    [ -f "$ini" ] || { say "   no config.ini"; return 0; }
    [ -f "$ini.legacy-paths" ] && { say "   already rewritten (the original is $ini.legacy-paths)"; return 0; }
    cp -p "$ini" "$ini.legacy-paths"
    # Installed files first: <prefix>/share/tawk/ and <prefix>/lib/tawk/ became
    # <prefix>/share/corvus-tawk/ and <prefix>/lib/corvus-tawk/. Then the per-user folders.
    set --
    for sub in sounds themes emoji; do
        set -- "$@" -e "s|^\([^#=]*=[[:space:]]*/.*\)/share/tawk/$sub/|\1/share/corvus-tawk/$sub/|"
    done
    set -- "$@" -e "s|^\([^#=]*=[[:space:]]*/.*\)/lib/tawk/sidecar|\1/lib/corvus-tawk/sidecar|"
    for pair in "$OLD_DATA|$NEW_DATA" "$OLD_CACHE|$NEW_CACHE" "$OLD_STATE|$NEW_STATE" "$OLD_CONFIG|$NEW_CONFIG"; do
        o="$(sed_lit "${pair%%|*}")"; n="$(sed_lit "${pair#*|}")"
        set -- "$@" -e "s|=\([[:space:]]*\)$o/|=\1$n/|" -e "s|=\([[:space:]]*\)$o\$|=\1$n|"
    done
    sed "$@" "$ini.legacy-paths" > "$ini.tmp" && chmod 0600 "$ini.tmp" && mv "$ini.tmp" "$ini"
    if cmp -s "$ini" "$ini.legacy-paths"; then
        say "   nothing named the old folders"
    else
        diff "$ini.legacy-paths" "$ini" | sed -n 's/^> /   now: /p'
        say "   (the copied original is kept as $ini.legacy-paths)"
    fi
}
[ "$config_rc" -eq 0 ] && rewrite_config

rewrite_database() {
    db="$NEW_DATA/tawk.db"
    say ""
    say "media paths in tawk.db:"
    if [ "$DRY_RUN" -eq 1 ]; then
        [ -f "$OLD_DATA/tawk.db" ] || { say "   no tawk.db"; return 0; }
        plan "point messages.media_path, statuses.media_path, profiles.picture and profiles.picture_full under $OLD_CACHE or $OLD_DATA at $NEW_CACHE and $NEW_DATA (in the copy only)"
        return 0
    fi
    [ -f "$db" ] || { say "   no tawk.db"; return 0; }
    [ -f "$NEW_DATA/.media-paths-rewritten" ] && { say "   already rewritten"; return 0; }
    if ! command -v sqlite3 >/dev/null 2>&1; then
        warn "sqlite3 is not installed, so media paths in $db still name $OLD_CACHE."
        warn "Keep $OLD_CACHE until you have run this again with sqlite3, or media already downloaded is fetched again."
        PROBLEMS=$((PROBLEMS + 1))
        return 1
    fi
    if ! sqlite3 "$db" 'PRAGMA user_version;' >/dev/null 2>&1; then
        warn "$db cannot be read by sqlite3 (encrypted with SQLCipher?); its media paths still name $OLD_CACHE."
        warn "Keep $OLD_CACHE: corvus-tawk shows media from there, and fetches again what is missing."
        return 0
    fi
    q() { printf "'%s'" "$(printf '%s' "$1" | sed "s/'/''/g")"; }
    sql=""
    for col in messages.media_path statuses.media_path profiles.picture profiles.picture_full; do
        t="${col%.*}"; c="${col#*.}"
        for pair in "$OLD_CACHE|$NEW_CACHE" "$OLD_DATA|$NEW_DATA"; do
            o="${pair%%|*}"; n="${pair#*|}"
            sql="$sql UPDATE $t SET $c = $(q "$n") || substr($c, length($(q "$o")) + 1) WHERE substr($c, 1, length($(q "$o")) + 1) = $(q "$o/");"
        done
    done
    # one transaction; a table an older database lacks makes it fail as a whole
    if out="$(sqlite3 -cmd '.timeout 5000' "$db" "BEGIN; $sql SELECT total_changes(); COMMIT;" 2>&1)"; then
        say "   $out path(s) now name the new folders"
        : > "$NEW_DATA/.media-paths-rewritten"
    else
        warn "could not update the media paths in $db: $out"
        warn "Keep $OLD_CACHE until this is sorted out."
        PROBLEMS=$((PROBLEMS + 1))
    fi
}
[ "$data_rc" -eq 0 ] && rewrite_database

# ---- what is left for you --------------------------------------------------

say ""
say "runtime: $OLD_RUNTIME (control.sock, admin.token) is not copied; corvus-tawk makes $NEW_RUNTIME when it starts."

old_vars="$(env | sed -n 's/^\(TAWK_[A-Za-z0-9_]*\)=.*/\1/p' | sort -u)"
if [ -n "$old_vars" ]; then
    say ""
    warn "these old environment variables are still set, and corvus-tawk does not read them:"
    for v in $old_vars; do
        case "$v" in
            TAWK_MCP_*) nv="CORVUS_TAWK_MCP_${v#TAWK_MCP_}" ;;
            *)          nv="CORVUS_$v" ;;
        esac
        warn "    $v  ->  set $nv instead"
    done
fi
old_vars="$(env | sed -n 's/^\(TAWKMCP_[A-Za-z0-9_]*\)=.*/\1/p' | sort -u)"
for v in $old_vars; do warn "    $v  ->  set CORVUS_TAWK_MCP_${v#TAWKMCP_} instead (read by corvus-tawk-mcp)"; done

say ""
if [ "$PROBLEMS" -gt 0 ]; then
    warn "$PROBLEMS problem(s) above; fix them before starting corvus-tawk."
    exit 1
fi
if [ "$DRY_RUN" -eq 1 ]; then
    say "dry run finished: nothing was changed."
else
    say "finished. The old folders are untouched; remove them yourself once corvus-tawk runs well."
    say "next: install corvus-tawk (CUTOVER.md), then start it with: corvus-tawk"
fi
exit 0
