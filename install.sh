#!/bin/bash
set -e

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTENSION_ID="synq-gnome@rsim"
EXTENSION_SRC="${REPO_DIR}/collectors/gnome/${EXTENSION_ID}"
EXTENSION_DEST="${HOME}/.local/share/gnome-shell/extensions/${EXTENSION_ID}"

echo "[INFO] Synq installer"
echo ""

# ---------------------------------------------------------------------------
# step 1: init git submodule (GNOME extension source)
# ---------------------------------------------------------------------------

if [ ! -f "${REPO_DIR}/collectors/gnome/${EXTENSION_ID}/metadata.json" ]; then
    echo "[INFO] initializing git submodule for GNOME extension"
    git -C "$REPO_DIR" submodule update --init --recursive
else
    echo "[INFO] submodule already initialized"
fi

# ---------------------------------------------------------------------------
# step 2: start server via Docker
# ---------------------------------------------------------------------------

echo ""
echo "[INFO] checking Docker"

if ! command -v docker &>/dev/null; then
    echo "[ERROR] Docker not installed."
    echo "        Ubuntu: sudo apt-get install docker.io docker-compose"
    echo "        Other:  https://docs.docker.com/get-docker/"
    exit 1
fi

if ! command -v docker-compose &>/dev/null; then
    echo "[ERROR] docker-compose not installed."
    echo "        sudo apt-get install docker-compose"
    exit 1
fi

if ! docker ps &>/dev/null; then
    echo "[ERROR] Docker is not running or permission denied."
    echo "        sudo usermod -aG docker \$USER  then log out and back in."
    exit 1
fi

echo "[INFO] stopping any existing Synq containers"
docker-compose -f "${REPO_DIR}/docker-compose.yml" down 2>/dev/null || true

echo "[INFO] building and starting Synq server"
docker-compose -f "${REPO_DIR}/docker-compose.yml" up -d --build

echo "[INFO] waiting for services to be healthy"
sleep 6

if docker-compose -f "${REPO_DIR}/docker-compose.yml" ps | grep -q "Up"; then
    echo "[INFO] server is running at http://localhost:5001"
    echo "[INFO] dashboard: http://localhost:5001/dashboard"
else
    echo "[ERROR] server failed to start -- check: docker-compose logs"
    exit 1
fi

# ---------------------------------------------------------------------------
# step 3: install GNOME extension and bridge collector
# ---------------------------------------------------------------------------

echo ""
echo "[INFO] checking GNOME desktop"

IS_GNOME=false
if echo "${XDG_CURRENT_DESKTOP:-}" | grep -qi "gnome"; then
    IS_GNOME=true
elif echo "${GDMSESSION:-}" | grep -qi "gnome"; then
    IS_GNOME=true
elif [ -n "${GNOME_DESKTOP_SESSION_ID:-}" ]; then
    IS_GNOME=true
fi

if [ "$IS_GNOME" = "false" ]; then
    echo "[WARN] GNOME desktop not detected -- skipping extension and collector install."
    echo "[WARN] if running GNOME, re-run this script from a GNOME session."
    echo ""
    echo "[INFO] server is ready. useful commands:"
    echo "         docker-compose logs -f     -- view server logs"
    echo "         docker-compose down        -- stop server"
    echo ""
    exit 0
fi

echo "[INFO] GNOME desktop detected"

# install extension files
if [ -d "$EXTENSION_SRC" ]; then
    echo "[INFO] installing GNOME extension to ${EXTENSION_DEST}"
    mkdir -p "$EXTENSION_DEST"
    cp -r "${EXTENSION_SRC}/." "$EXTENSION_DEST/"
    echo "[INFO] extension files copied"
else
    echo "[ERROR] extension source not found at ${EXTENSION_SRC}"
    echo "        run: git submodule update --init --recursive"
    exit 1
fi

# install Python bridge dependencies
echo "[INFO] installing bridge collector dependencies"
bash "${REPO_DIR}/collectors/synq-gnome/install.sh"

# enable extension
echo ""
echo "[INFO] enabling GNOME extension ${EXTENSION_ID}"

ENABLED=false
if command -v gnome-extensions &>/dev/null; then
    if gnome-extensions enable "$EXTENSION_ID" 2>/dev/null; then
        ENABLED=true
        echo "[INFO] extension enabled"
    fi
fi

# handle session type: Wayland requires logout; X11 can reload in place
SESSION_TYPE="${XDG_SESSION_TYPE:-unknown}"

if [ "$ENABLED" = "true" ] && [ "$SESSION_TYPE" = "x11" ]; then
    echo "[INFO] X11 session -- reloading GNOME Shell"
    busctl --user call org.gnome.Shell /org/gnome/Shell \
        org.gnome.Shell Eval s \
        'Meta.restart("Restarting GNOME Shell", global.context)' \
        2>/dev/null && echo "[INFO] GNOME Shell reloaded" || \
        echo "[WARN] could not reload GNOME Shell automatically"
else
    echo ""
    echo "[WARN] log out and back in to activate the GNOME extension."
    echo "[INFO] after re-login, open GNOME Extensions to confirm '${EXTENSION_ID}' is on."
    # try to open extensions app for the user
    gnome-extensions-app 2>/dev/null & disown 2>/dev/null || \
    gnome-control-center extensions 2>/dev/null & disown 2>/dev/null || true
fi

# ---------------------------------------------------------------------------
# step 4: print next steps
# ---------------------------------------------------------------------------

echo ""
echo "[INFO] setup complete. next steps:"
echo ""
echo "  1. if prompted above: log out and back in to activate the extension."
echo ""
echo "  2. start the collector bridge:"
echo "         export SYNQ_ENDPOINT=http://localhost:5001"
echo "         python3 ${REPO_DIR}/collectors/synq-gnome/collector.py"
echo ""
echo "  3. view your activity at: http://localhost:5001/dashboard"
echo ""
echo "  server commands:"
echo "         docker-compose logs -f        -- view server logs"
echo "         docker-compose down           -- stop server"
echo "         docker-compose restart        -- restart server"
echo ""
