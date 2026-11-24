# ThatsApp

A Qt 6 / KDE Frameworks 6 wrapper for WhatsApp Web, built for KDE Plasma 6 on Wayland.

## Features

- Native Plasma notifications with contact avatars, click-to-open chat, per-chat cooldowns, and quick snooze
- Unread count badge in the Plasma task manager
- Collapsible chat list sidebar (`Ctrl+B` / `F9`) and hidden-by-default menu bar (`Ctrl+M`)
- Optional system tray icon and close-to-tray
- Camera, mic, location, and screen capture off by default (toggleable in Settings, `Ctrl+,`)
- Opens external links in your default browser and handles `whatsapp://` links
- Native KDE Save As dialog, zoom shortcuts, and single-instance window focus

## Building and installing

Built and tested on Fedora KDE. Install the build dependencies first:

```bash
sudo ./scripts/install-build-deps.sh
```

Then build, run the tests, and install to `~/.local`:

```bash
./scripts/install-local.sh
```

Run `thatsapp` or launch it from your application menu. There's also a Fedora spec file in `packaging/thatsapp.spec`.
