#!/usr/bin/env bash
set -euo pipefail

dnf install -y --disablerepo='*tailscale*' \
    cmake \
    gcc-c++ \
    extra-cmake-modules \
    qt6-qtbase-devel \
    qt6-qtwebengine-devel \
    kf6-kcoreaddons-devel \
    kf6-kconfig-devel \
    kf6-kconfigwidgets-devel \
    kf6-kdbusaddons-devel \
    kf6-ki18n-devel \
    kf6-knotifications-devel \
    kf6-kstatusnotifieritem-devel \
    kf6-kwidgetsaddons-devel \
    kf6-kwindowsystem-devel \
    kf6-kxmlgui-devel
