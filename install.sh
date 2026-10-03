#!/usr/bin/env bash
# tawk installer: builds and installs tawk from source.
#
# From a checkout:     ./install.sh [options]
# Straight from GitHub: curl -fsSL https://raw.githubusercontent.com/loganventer/tawk/main/install.sh | bash
#                       curl -fsSL .../install.sh | bash -s -- --prefix "$HOME/.local"
#
# Options:
#   --prefix DIR       install location (default /usr/local)
#   --ref REF          branch or tag to build when cloning (default main)
#   --repo URL         repository to clone (default https://github.com/loganventer/tawk.git)
#   --with-sidecar     also install the Node.js (Baileys) backend
#   --no-whatsmeow     skip the in-process backend (no Go needed; implies --with-sidecar)
#   --no-deps          do not install system packages
#   --alias NAME       add "alias NAME=tawk" to your shell rc file
#   --uninstall        remove an installed tawk
#   -y, --yes          do not ask before installing packages
#   -h, --help         show this help
set -euo pipefail

REPO_URL="https://github.com/loganventer/tawk.git"
REF="main"
PREFIX="/usr/local"
WITH_SIDECAR=0
WHATSMEOW=1
INSTALL_DEPS=1
ASSUME_YES=0
ALIAS_NAME=""
UNINSTALL=0
MIN_GO_MINOR=21   # Go >= 1.21 fetches the newer toolchain whatsmeow asks for (GOTOOLCHAIN=auto)

say()  { printf '\033[1;32m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m!!\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mxx\033[0m %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

usage() { sed -n '2,20p' "$0" 2>/dev/null | sed 's/^# \{0,1\}//' || true; }

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix)       PREFIX="${2:?--prefix needs a directory}"; shift 2 ;;
        --ref)          REF="${2:?--ref needs a branch or tag}"; shift 2 ;;
        --repo)         REPO_URL="${2:?--repo needs a URL}"; shift 2 ;;
        --with-sidecar) WITH_SIDECAR=1; shift ;;
        --no-whatsmeow) WHATSMEOW=0; WITH_SIDECAR=1; shift ;;
        --no-deps)      INSTALL_DEPS=0; shift ;;
        --alias)        ALIAS_NAME="${2:?--alias needs a name}"; shift 2 ;;
        --uninstall)    UNINSTALL=1; shift ;;
        -y|--yes)       ASSUME_YES=1; shift ;;
        -h|--help)      usage; exit 0 ;;
        *)              die "unknown option: $1 (see --help)" ;;
    esac
done

case "$ALIAS_NAME" in
    ""|[A-Za-z_]*) ;;
    *) die "alias names must start with a letter" ;;
esac
[[ "$ALIAS_NAME" =~ ^[A-Za-z_][A-Za-z0-9_-]*$ || -z "$ALIAS_NAME" ]] || die "invalid alias name"

SUDO=""
need_root_for() {
    local dir="$1"
    while [ ! -d "$dir" ]; do dir="$(dirname "$dir")"; done
    if [ ! -w "$dir" ] && [ "$(id -u)" -ne 0 ]; then
        have sudo || die "$1 is not writable and sudo is not available; use --prefix \$HOME/.local"
        SUDO="sudo"
    fi
}

if [ "$UNINSTALL" -eq 1 ]; then
    need_root_for "$PREFIX/bin"
    $SUDO rm -f "$PREFIX/bin/tawk" "$PREFIX/share/man/man1/tawk.1" "$PREFIX/share/bash-completion/completions/tawk"
    $SUDO rm -rf "$PREFIX/share/tawk" "$PREFIX/lib/tawk" "$PREFIX/share/doc/tawk"
    say "removed tawk from $PREFIX"
    echo "    Your files were kept: ~/.config/tawk (settings), ~/.local/share/tawk (chats, login),"
    echo "    ~/.cache/tawk (media) and ~/.local/state/tawk (logs). Delete them to remove everything."
    exit 0
fi

# ---- platform and dependencies ---------------------------------------------

OS="$(uname -s)"
case "$OS" in
    Linux)                 PLATFORM=linux ;;
    Darwin)                PLATFORM=macos ;;
    MINGW*|MSYS*|CYGWIN*)  PLATFORM=windows ;;
    *)                     die "unsupported platform: $OS" ;;
esac
DISTRO=""
[ -r /etc/os-release ] && DISTRO="$(. /etc/os-release && echo "${PRETTY_NAME:-$NAME}")"
say "platform: $PLATFORM${DISTRO:+ ($DISTRO)}$(grep -qiE 'microsoft|wsl' /proc/sys/kernel/osrelease 2>/dev/null && echo ', running under WSL')"

# On macOS cc, make and git in /usr/bin are stubs that hand over to the Xcode
# tools, so `command -v` finds them even when they cannot run. Check they work.
macos_preflight() {
    xcode-select -p >/dev/null 2>&1 \
        || die "the Xcode command line tools are not installed. Run: xcode-select --install  (then run this installer again)"
    local out
    if ! out="$(cc --version 2>&1)"; then
        case "$out" in
            *[Ll]icense*) die "the Xcode license has not been accepted. Run: sudo xcodebuild -license accept  (then run this installer again)" ;;
            *)            die "the Xcode command line tools do not work: $(printf '%s' "$out" | head -n 1). Try: xcode-select --install" ;;
        esac
    fi
    # Homebrew on Apple silicon lives in /opt/homebrew, which may not be on PATH yet
    if ! have brew; then
        for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do
            [ -x "$b" ] && { eval "$("$b" shellenv)"; break; }
        done
    fi
    if ! have brew && [ "$INSTALL_DEPS" -eq 1 ]; then
        die "Homebrew is needed for ncurses, SQLite and pkg-config. Install it from https://brew.sh:
    /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"
then run this installer again (or pass --no-deps if you provide these yourself)"
    fi
}
[ "$PLATFORM" = macos ] && macos_preflight

for pm in apt-get dnf pacman zypper brew; do
    if have "$pm"; then say "package manager: $pm"; break; fi
done

confirm() {
    [ "$ASSUME_YES" -eq 1 ] && return 0
    [ -t 0 ] || [ -e /dev/tty ] || return 0
    printf '%s [Y/n] ' "$1"
    local reply=""
    read -r reply </dev/tty || true
    case "$reply" in n|N|no|NO) return 1 ;; *) return 0 ;; esac
}

pkg_install() {
    [ "$#" -eq 0 ] && return 0
    local pkgs=("$@")
    confirm "Install packages: ${pkgs[*]}?" || { warn "skipped package installation"; return 0; }
    local root=""
    [ "$(id -u)" -ne 0 ] && have sudo && root="sudo"
    if   have apt-get; then
        say "updating package lists (apt-get update, this can take a minute)"
        $root apt-get update -qq
        say "installing ${pkgs[*]}"
        $root apt-get install -y -qq "${pkgs[@]}"
    elif have dnf;     then $root dnf install -y "${pkgs[@]}"
    elif have pacman;  then $root pacman -S --needed --noconfirm "${pkgs[@]}"
    elif have zypper;  then $root zypper install -y "${pkgs[@]}"
    elif have brew;    then brew install "${pkgs[@]}"
    else warn "no known package manager; install these yourself: ${pkgs[*]}"
    fi
}

go_ok() {
    have go || return 1
    local v; v="$(go env GOVERSION 2>/dev/null | sed 's/^go//')"
    local minor; minor="$(printf '%s' "$v" | cut -d. -f2)"
    [ "${minor:-0}" -ge "$MIN_GO_MINOR" ] 2>/dev/null
}

# Runtime tools: recording and playing voice notes, and opening media in the
# default viewer. Only what is missing is added.
have_audio() { have parecord || have pw-record || have arecord; }
runtime_deps() {
    local audio="$1" opener="$2" pdf="${3:-}"
    have_audio || [ -z "$audio" ] || pkgs+=($audio)
    [ "$PLATFORM" = linux ] && [ -n "$opener" ] && ! have xdg-open && pkgs+=($opener)
    # PDF pages in the viewer
    have pdftoppm || [ -z "$pdf" ] || pkgs+=($pdf)
    # pasting pictures from the clipboard (Alt+V): Wayland and X11 tools
    if [ "$PLATFORM" = linux ]; then
        have wl-paste || pkgs+=(wl-clipboard)
        have xclip || pkgs+=(xclip)
    fi
    return 0
}

install_deps() {
    local pkgs=()
    if have apt-get; then
        # Named one by one: build-essential can be installed already while make or the compiler was removed,
        # and apt then installs nothing for it.
        have cc || have gcc || pkgs+=(gcc libc6-dev)
        have make || pkgs+=(make)
        have pkg-config || pkgs+=(pkg-config)
        have git || pkgs+=(git)
        dpkg -s libncurses-dev >/dev/null 2>&1 || pkgs+=(libncurses-dev)
        dpkg -s libsqlite3-dev >/dev/null 2>&1 || pkgs+=(libsqlite3-dev)
        dpkg -s libsqlcipher-dev >/dev/null 2>&1 || pkgs+=(libsqlcipher-dev)   # encrypting your chats
        have openssl || pkgs+=(openssl)                                           # encrypted backups
        have ffmpeg || pkgs+=(ffmpeg)
        runtime_deps pulseaudio-utils xdg-utils poppler-utils
        [ "$WHATSMEOW" -eq 1 ] && ! go_ok && pkgs+=(golang-go)
        [ "$WITH_SIDECAR" -eq 1 ] && ! have npm && pkgs+=(nodejs npm)
    elif have dnf; then
        have cc || pkgs+=(gcc)
        have make || pkgs+=(make)
        pkgs+=(pkgconf-pkg-config ncurses-devel sqlite-devel sqlcipher-devel openssl git)
        have ffmpeg || pkgs+=(ffmpeg-free)
        runtime_deps pulseaudio-utils xdg-utils poppler-utils
        [ "$WHATSMEOW" -eq 1 ] && ! go_ok && pkgs+=(golang)
        [ "$WITH_SIDECAR" -eq 1 ] && ! have npm && pkgs+=(nodejs npm)
    elif have pacman; then
        pkgs+=(base-devel pkgconf ncurses sqlite sqlcipher openssl git)
        have ffmpeg || pkgs+=(ffmpeg)
        runtime_deps libpulse xdg-utils poppler
        [ "$WHATSMEOW" -eq 1 ] && ! go_ok && pkgs+=(go)
        [ "$WITH_SIDECAR" -eq 1 ] && ! have npm && pkgs+=(nodejs npm)
    elif have zypper; then
        pkgs+=(gcc make pkg-config ncurses-devel sqlite3-devel sqlcipher-devel openssl git)
        have ffmpeg || pkgs+=(ffmpeg)
        runtime_deps pulseaudio-utils xdg-utils poppler-tools
        [ "$WHATSMEOW" -eq 1 ] && ! go_ok && pkgs+=(go)
        [ "$WITH_SIDECAR" -eq 1 ] && ! have npm && pkgs+=(nodejs20 npm20)
    elif have brew; then
        pkgs+=(ncurses sqlite sqlcipher pkg-config)
        have ffmpeg || pkgs+=(ffmpeg)
        have pdftoppm || pkgs+=(poppler)
        have rec || pkgs+=(sox)                    # voice notes: ffmpeg's mic capture clicks on some Macs
        have pngpaste || pkgs+=(pngpaste)          # Alt+V pastes a picture
        [ "$WHATSMEOW" -eq 1 ] && ! go_ok && pkgs+=(go)
        [ "$WITH_SIDECAR" -eq 1 ] && ! have npm && pkgs+=(node)
    fi
    # ${arr[@]+...} keeps bash 3.2 (macOS) happy with an empty array under set -u
    pkg_install ${pkgs[@]+"${pkgs[@]}"}
}

[ "$INSTALL_DEPS" -eq 1 ] && install_deps

version_of() {
    case "$1" in
        cc)         cc --version 2>/dev/null | head -n 1 ;;
        go)         go env GOVERSION 2>/dev/null ;;
        node)       node --version 2>/dev/null ;;
        ffmpeg)     ffmpeg -version 2>/dev/null | head -n 1 | cut -d' ' -f1-3 ;;
        parecord)   have parecord && echo "found" ;;
        xdg-open)   have xdg-open && echo "found" ;;
        pdftoppm)   have pdftoppm && pdftoppm -v 2>&1 | head -n 1 ;;
        wl-paste)   have wl-paste && echo "found" ;;
        xclip)      have xclip && echo "found" ;;
        ncursesw)   pkg-config --modversion ncursesw 2>/dev/null || pkg-config --modversion ncurses 2>/dev/null ;;
        sqlite3)    pkg-config --modversion sqlite3 2>/dev/null ;;
        *)          "$1" --version 2>/dev/null | head -n 1 ;;
    esac
}
say "toolchain"
for tool in cc make pkg-config git ncursesw sqlite3 go node ffmpeg pdftoppm parecord xdg-open wl-paste xclip; do
    v="$(version_of "$tool" || true)"
    if [ -n "$v" ]; then printf '    %-11s %s\n' "$tool" "$v"; else printf '    %-11s %s\n' "$tool" "not found"; fi
done

# What to type for a tool the package manager did not bring.
install_hint() {
    if   have apt-get; then printf 'sudo apt-get install %s' "$1"
    elif have dnf;     then printf 'sudo dnf install %s' "$1"
    elif have pacman;  then printf 'sudo pacman -S %s' "$1"
    elif have zypper;  then printf 'sudo zypper install %s' "$1"
    elif have brew;    then printf 'xcode-select --install'
    else printf 'install %s with your package manager' "$1"
    fi
}
have make || die "make is required and is still missing. Run this, then run the installer again:
    $(install_hint make)"
have cc || have gcc || have clang || die "a C compiler is required and is still missing. Run this, then run the installer again:
    $(install_hint gcc)"
if [ "$WHATSMEOW" -eq 1 ] && ! go_ok; then
    warn "Go 1.$MIN_GO_MINOR+ not found; building with the Node.js backend only"
    WHATSMEOW=0
    WITH_SIDECAR=1
fi
if [ "$WITH_SIDECAR" -eq 1 ] && ! have npm; then
    die "the Node.js backend needs node and npm"
fi
if ! have ffmpeg; then
    FFMPEG_CMD=""
    if   have apt-get; then FFMPEG_CMD="sudo apt-get install ffmpeg"
    elif have dnf;     then FFMPEG_CMD="sudo dnf install ffmpeg-free"
    elif have pacman;  then FFMPEG_CMD="sudo pacman -S ffmpeg"
    elif have zypper;  then FFMPEG_CMD="sudo zypper install ffmpeg"
    elif have brew;    then FFMPEG_CMD="brew install ffmpeg"
    fi
    warn "ffmpeg not found: voice notes will not record or play until it is installed${FFMPEG_CMD:+ (run: $FFMPEG_CMD)}"
fi

if [ "$PLATFORM" = macos ] && have brew; then
    # Homebrew's ncurses has wide-character support; the system one is dated.
    export PKG_CONFIG_PATH="$(brew --prefix ncurses)/lib/pkgconfig:$(brew --prefix sqlite)/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
fi

# ---- source: this checkout, or a fresh clone --------------------------------

SCRIPT_DIR=""
if [ -n "${BASH_SOURCE[0]:-}" ] && [ -f "${BASH_SOURCE[0]}" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi
CLEANUP=""
if [ -n "$SCRIPT_DIR" ] && [ -f "$SCRIPT_DIR/Makefile" ] && [ -f "$SCRIPT_DIR/src/main.c" ]; then
    SRC="$SCRIPT_DIR"
    say "building from $SRC"
else
    have git || die "git is required to download tawk"
    # The download and the build need room. /tmp is small on some systems (a memory disk, or a full one),
    # so the first folder with enough free space is used: TMPDIR or /tmp, then your cache folder.
    NEED_MB=600
    free_mb() { df -Pk "$1" 2>/dev/null | awk 'NR==2 { print int($4 / 1024) }'; }
    BUILD_PARENT=""
    for dir in "${TMPDIR:-/tmp}" "${XDG_CACHE_HOME:-$HOME/.cache}"; do
        mkdir -p "$dir" 2>/dev/null || continue
        [ -w "$dir" ] || continue
        if [ "$(free_mb "$dir" || echo 0)" -ge "$NEED_MB" ] 2>/dev/null; then BUILD_PARENT="$dir"; break; fi
        warn "$dir has only $(free_mb "$dir") MB free; tawk needs about $NEED_MB MB to build"
    done
    [ -n "$BUILD_PARENT" ] || die "not enough free disk space to download and build tawk (about $NEED_MB MB is needed).
Free some space, or name a folder on a disk that has room and run the installer again:
    TMPDIR=/path/with/space bash install.sh
Free space now:
$(df -h "${TMPDIR:-/tmp}" "$HOME" 2>/dev/null)"
    SRC="$(mktemp -d "$BUILD_PARENT/tawk-build.XXXXXX")"
    CLEANUP="$SRC"
    trap '[ -n "$CLEANUP" ] && rm -rf "$CLEANUP"' EXIT
    say "downloading tawk ($REF) from $REPO_URL into $SRC"
    git clone --quiet --depth 1 --branch "$REF" "$REPO_URL" "$SRC" || die "the download failed. If it says it was unable to write a file, the disk is full or the folder cannot be written to.
Free space now:
$(df -h "$BUILD_PARENT" "$HOME" 2>/dev/null)"
    say "building commit $(git -C "$SRC" log -1 --format='%h %s (%cd)' --date=short)"
fi

# ---- build and install ------------------------------------------------------

cd "$SRC"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
# The whatsmeow bridge's SQLite package needs about 2 GB of memory to compile.
FREE_MB="$(awk '/^MemAvailable:/ {m=$2} /^SwapFree:/ {s=$2} END {if (m) print int((m+s)/1024)}' /proc/meminfo 2>/dev/null || true)"
if [ "$WHATSMEOW" -eq 1 ] && [ -n "$FREE_MB" ] && [ "$FREE_MB" -lt 2560 ]; then
    warn "only about $FREE_MB MB of memory and swap are free; compiling the whatsmeow bridge may be killed."
    warn "if it is, add swap (sudo fallocate -l 4G /swapfile && sudo chmod 600 /swapfile && sudo mkswap /swapfile && sudo swapon /swapfile) and run this again, or install with --no-whatsmeow"
    JOBS=1
fi
say "compiling (whatsmeow=$WHATSMEOW)"
GOTOOLCHAIN=auto make -j"$JOBS" WHATSMEOW="$WHATSMEOW" PREFIX="$PREFIX"
if [ "$WITH_SIDECAR" -eq 1 ]; then
    say "installing Node.js backend dependencies"
    make sidecar
fi

need_root_for "$PREFIX/bin"
say "installing to $PREFIX${SUDO:+ (using sudo, $PREFIX is not writable by you)}"
$SUDO make install WHATSMEOW="$WHATSMEOW" PREFIX="$PREFIX"

if [ -n "$ALIAS_NAME" ]; then
    RC="$HOME/.bashrc"
    [ -n "${ZSH_VERSION:-}" ] || [ "$(basename "${SHELL:-}")" = zsh ] && RC="$HOME/.zshrc"
    if ! grep -qE "^alias $ALIAS_NAME=" "$RC" 2>/dev/null; then
        printf "\nalias %s='%s'\n" "$ALIAS_NAME" "$PREFIX/bin/tawk" >> "$RC"
        say "added alias $ALIAS_NAME to $RC (open a new shell to use it)"
    else
        warn "alias $ALIAS_NAME already exists in $RC; left it unchanged"
    fi
fi

say "checking the setup (tawk --doctor)"
"$PREFIX/bin/tawk" --doctor || warn "fix the problems above, then run: tawk --doctor"

say "done. Run: tawk"
echo "    Link it from your phone: WhatsApp > Settings > Linked devices > Link a device"
echo "    Settings live in ~/.config/tawk/config.ini (or press F2 inside tawk)"
