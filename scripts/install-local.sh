#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PREFIX="${HOME}/.local"

echo "==> Configuring ThatsApp (prefix: ${PREFIX})"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DBUILD_TESTING=ON

echo "==> Building ThatsApp"
cmake --build "${BUILD_DIR}"

echo "==> Running unit tests"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> Installing to ${PREFIX}"
cmake --install "${BUILD_DIR}"

# Guardrail: never leave a user-local hicolor/index.theme that shadows /usr/share/icons/hicolor/index.theme
if [[ -e "${PREFIX}/share/icons/hicolor/index.theme" ]]; then
    echo "ERROR: Unexpected ${PREFIX}/share/icons/hicolor/index.theme detected!" >&2
    exit 1
fi

echo "==> Refreshing desktop & KDE icon caches"
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${PREFIX}/share/applications" || true
fi
if command -v xdg-mime >/dev/null 2>&1; then
    xdg-mime default io.github.hjr555.thatsapp.desktop x-scheme-handler/whatsapp || true
    xdg-mime default io.github.hjr555.thatsapp.desktop x-scheme-handler/whatsapp-smb || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t --ignore-theme-index "${PREFIX}/share/icons/hicolor" || true
fi
rm -f "${HOME}/.cache/icon-cache.kcache"
if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental || true
fi
if command -v gdbus >/dev/null 2>&1; then
    for group in 0 1 2 3 4 5; do
        gdbus emit --session --object-path /KIconLoader --signal org.kde.KIconLoader.iconChanged "${group}" || true
    done
fi

echo "==> Verifying desktop entry and icon resolution"
if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "${PREFIX}/share/applications/io.github.hjr555.thatsapp.desktop"
fi
if command -v kiconfinder6 >/dev/null 2>&1; then
    ICON_PATH="$(kiconfinder6 io.github.hjr555.thatsapp || true)"
    echo "Resolved icon: ${ICON_PATH}"
fi

echo "==> Done! Launch with: thatsapp"
