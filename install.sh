#!/usr/bin/env bash
set -e

ascii_art="\n\e[1;32m\
 ┌───────┐ ┌───┐ ┌─┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐\n\
═╘═╕∙ ·╒═╛═│∙  │═│∙│═╘═╕∙  ╒═╛═│∙╒═════╛═╘═╕∙  ╒═╛═│∙  ╒═╕∙│\n\
░░▒│   │▒░░│   │█│ │ ▓▓│   │░▓▓│ └─────┐ ▓▓│   │░▓ │   │▓└─┘\n\
░░▒│   │▒░▒│   │▓│ │░░░│   │▓██╘═══╕  ∙│░░░│   │▓█░│   │░┌─┐\n\
░░▒│   │▒░▓│   └─┘ │▒▒▒│   │░▒▓┌───┘   │▒▒▒│   │░▒▒│   │░│ │\n\
═══│∙ ·│═══│∙     ∙│═┌─┘∙  └─┐═│∙     ∙│═┌─┘∙  └─┐═│∙  ╘═╛∙│\n\
   ╘═══╛   ╘═══════╛ ╘═══════╛ ╘═══════╛ ╘═══════╛ ╘═══════╛\e[0m\n"
echo -e "$ascii_art"

REPO_OWNER="Dark-Kernel"
REPO_NAME="tuisic"
RELEASE_API="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/latest"
RELEASE_URL="https://github.com/$REPO_OWNER/$REPO_NAME/releases/latest/download"

# --- DETECT OS & ARCH ---
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

if [[ $OS == "darwin" ]]; then
    case $ARCH in
        x86_64) PLATFORM="macos-x64";;
        arm64)  PLATFORM="macos-arm64";;
        *)
            echo -e "\e[91mSorry, Mac architecture $ARCH is not supported by tuisic binary releases.\e[0m"
            unsupported=1
            ;;
    esac
elif [[ $OS == "linux" ]]; then
    case $ARCH in
        x86_64*) PLATFORM="linux-x64";;
        *)
            echo -e "\e[91mSorry, Linux architecture $ARCH is not supported by tuisic binary releases.\e[0m"
            unsupported=1
            ;;
    esac
elif grep -qEi "(Microsoft|WSL)" /proc/version 2>/dev/null; then
    PLATFORM="linux-x64"
    OS="linux"
else
    echo -e "\e[91mSorry, your OS ($OS $ARCH) is not supported by tuisic binary releases.\e[0m"
    unsupported=1
fi

if [[ "$unsupported" == "1" ]]; then
    echo -e "\n\e[1;33mYou can build tuisic from source:\e[0m"
    echo "    git clone https://github.com/${REPO_OWNER}/${REPO_NAME}.git"
    echo "    cd ${REPO_NAME} && mkdir build && cd build"
    echo "    cmake .. -DCMAKE_BUILD_TYPE=Release"
    echo "    make -j\$(nproc)"
    echo "    sudo make install"
    exit 11
fi

BINARY="tuisic-$PLATFORM"
TMPDIR="$(mktemp -d)"
INSTALL_PATH="/usr/local/bin/tuisic"

# Check write permissions for default, fallback to ~/.local/bin
if [ ! -w "$(dirname "$INSTALL_PATH")" ]; then
    INSTALL_PATH="$HOME/.local/bin/tuisic"
    mkdir -p "$HOME/.local/bin"
    echo "No sudo. Will install to $INSTALL_PATH" >&2
fi

BIN_URL="$RELEASE_URL/$BINARY"
echo -e "\nFetching latest $BINARY from $BIN_URL ...\n"

# Download binary
if ! curl -fsSL -o "$TMPDIR/$BINARY" "$BIN_URL"; then
    echo -e "\e[91mDownload failed. Please check your internet connection, or see https://github.com/$REPO_OWNER/$REPO_NAME/releases for latest binaries.\e[0m"
    rm -rf "$TMPDIR"
    exit 2
fi
chmod +x "$TMPDIR/$BINARY"

mv "$TMPDIR/$BINARY" "$INSTALL_PATH"
echo -e "\e[32mInstalled tuisic to $INSTALL_PATH\e[0m\n"

# ---- Runtime dependency check (Linux) ----
if [[ $OS == "linux" ]]; then
    deps_ok=true
    for lib in libfmt.so libmpv.so libcurl.so libpulse.so; do
        if ! ldconfig -p 2>/dev/null | grep -q "$lib"; then
            echo -e "\e[33mWarning: $lib not found — tuisic may fail to run.\e[0m"
            deps_ok=false
        fi
    done
    if [[ "$deps_ok" == "false" ]]; then
        echo -e "\n\e[33mOn Debian/Ubuntu, install missing libraries:\e[0m"
        echo "  sudo apt-get install libfmt-dev libmpv-dev libcurl4-openssl-dev libpulse-dev"
        echo -e "\e[33mOn Fedora/RHEL:\e[0m"
        echo "  sudo dnf install fmt-devel mpv-libs-devel libcurl-devel pulseaudio-libs-devel"
    fi
fi

# Suggest adding to PATH if needed
case ":$PATH:" in
  *:"$HOME/.local/bin":*) ;;  # already in path
  *)
    if [[ "$INSTALL_PATH" == "$HOME/.local/bin/tuisic" ]]; then
      echo -e "\nTo use 'tuisic', add '$HOME/.local/bin' to your PATH:"
      echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
    ;;
esac

# Show version
if "$INSTALL_PATH" --version 2>/dev/null; then
    "$INSTALL_PATH" --version
else
    echo "Run 'tuisic --version' to verify installation." >&2
fi

echo -e "\n\e[1;32mInstall complete. Enjoy tuisic!\e[0m\n"
rm -rf "$TMPDIR"
exit 0
