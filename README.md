# HatsApp

A lightweight, native **C++20 / Qt 6 / KDE Frameworks 6 (KF6)** desktop client for WhatsApp Web designed for **Fedora KDE Plasma 6 (Wayland)**.

## Features

- **Native KDE Plasma 6 integration:** `KMainWindow` geometry persistence, Plasma active Qt style (Breeze/Fusion), `KDBusService` single-instance with Wayland `xdg_activation_v1` focus handoff, and Plasma Task Manager unread badges (`com.canonical.Unity.LauncherEntry`).
- **Anti-spam KDE Notifications (`KNotifications` / `hatsapp.notifyrc`):** Contact avatars, click-to-focus straight to the chat, focus suppression, per-chat popup coalescing & cooldown, Quick Snooze (`15m`, `1h`, `8h`, plus a **"Snooze 1 hour"** toast button), and **Hide Notifications** (`Ctrl+Shift+M`).
- **Focused conversation layout:** Full-bleed WhatsApp Web canvas, hidden-by-default KDE menu bar (`Ctrl+M` or right-click), and **collapsible chat sidebar** (`Ctrl+B` / `F9` / right-click) for full-width conversations.
- **Privacy-strict defaults:** Microphone, camera, geolocation, and screen capture are denied by default and strictly opt-in via **Configure HatsApp** (`Ctrl+,`).
- **Links, protocols & polish:** External links open in your default browser, `whatsapp://` and `whatsapp-smb://` deep links route into the running window (`tel:` untouched), British English (`en_GB`) spellcheck, discrete zoom (`Ctrl++`/`-`/`0`), native KDE Save-As downloads, and opt-in `KStatusNotifierItem` system tray & close-to-tray.

## Build & Install (Fedora KDE)

Install build dependencies once:

```bash
sudo ./scripts/install-build-deps.sh
```

Build, run unit tests (`ctest`), and install to `~/.local`:

```bash
./scripts/install-local.sh
```

Launch with `hatsapp` or from your KDE Application Launcher. A Fedora RPM spec is also available at `packaging/hatsapp.spec`.
