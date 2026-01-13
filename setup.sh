#!/bin/bash

set -e

# https://ziglang.org/download/community-mirrors.txt
MIRROR=https://zigmirror.meox.dev

# detect aarch64 macOS
ARCH=$(uname -m)
OS=$(uname -s)
if [[ "$ARCH" == "arm64" && "$OS" == "Darwin" ]]; then
    TAR_NAME="zig-aarch64-macos-0.15.2.tar.xz"
else
    echo "Unsupported architecture: $ARCH on $OS"
    exit 1
fi

echo "Downloading Zig $TAR_NAME from $MIRROR"

# download tarball
# TEMP_DIR=$(mktemp -d)
rm -rf ~/test
mkdir ~/test
TEMP_DIR=~/test
curl --output-dir "$TEMP_DIR" -LO "$MIRROR/$TAR_NAME?source=soup"
if [[ $? -ne 0 ]]; then
    echo "Failed to download Zig from $MIRROR/$TAR_NAME"
    exit 1
fi

# verified offline with:
# minisign -Vm zig-aarch64-macos-0.15.2.tar.xz -P RWSGOq2NVecA2UPNdBUZykf1CCb147pkmdtYxgb3Ti+JO/wCYvhbAb/U
#
# ensure hash matches expected
EXPECTED_HASH=3cc2bab367e185cdfb27501c4b30b1b0653c28d9f73df8dc91488e66ece5fa6b
DOWNLOADED_HASH=$(shasum -a 256 "$TEMP_DIR/$TAR_NAME" | awk '{print $1}')
if [[ "$EXPECTED_HASH" != "$DOWNLOADED_HASH" ]]; then
    echo "Hash mismatch! Expected $EXPECTED_HASH but got $DOWNLOADED_HASH"
    exit 1
fi

# extract to install dir
INSTALL_DIR=third-party/zig
echo "Installing Zig to $INSTALL_DIR"
rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"
tar -xf "$TEMP_DIR/$TAR_NAME" --strip-components=1 -C "$INSTALL_DIR"

echo "Cleaning up"
rm -rf "$TEMP_DIR"

echo "Setup complete."