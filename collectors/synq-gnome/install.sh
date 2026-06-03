#!/usr/bin/env bash
# installs system and Python dependencies for the synq-gnome collector bridge.
# requires apt (Debian/Ubuntu) and Python 3.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

_check_cmd() {
    command -v "$1" >/dev/null 2>&1
}

echo "[INFO] checking system dependencies"

if ! _check_cmd python3; then
    echo "[ERROR] python3 not found -- install Python 3.10+ before continuing"
    exit 1
fi

if ! _check_cmd pip3 && ! python3 -m pip --version >/dev/null 2>&1; then
    echo "[ERROR] pip not found -- install python3-pip before continuing"
    exit 1
fi

if ! _check_cmd apt-get; then
    echo "[WARN] apt-get not available -- skipping system package install"
    echo "[WARN] ensure python3-gi and gir1.2-glib-2.0 are installed manually"
else
    echo "[INFO] installing system packages for PyGObject DBus support"
    sudo apt-get install -y python3-gi python3-dbus gir1.2-glib-2.0 python3-gi-cairo
    echo "[INFO] system packages installed"
fi

echo "[INFO] installing Python dependencies"
pip3 install --user -r "$SCRIPT_DIR/requirements.txt"
echo "[INFO] Python dependencies installed"

echo ""
echo "[INFO] installation complete"
echo "[INFO] usage:"
echo "         export SYNQ_ENDPOINT=http://<server>:5001"
echo "         python3 $SCRIPT_DIR/collector.py"
echo ""
echo "[INFO] the GNOME Shell extension (collectors/gnome) must be installed and"
echo "       enabled in GNOME for the collector to receive window events."
