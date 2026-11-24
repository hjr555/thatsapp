Name:           thatsapp
Version:        0.1.0
Release:        1%{?dist}
Summary:        Native KDE Plasma 6 desktop client for WhatsApp Web

License:        MIT
URL:            https://github.com/hjr555/thatsapp
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.24
BuildRequires:  desktop-file-utils
BuildRequires:  extra-cmake-modules >= 6.0
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  cmake(Qt6Core) >= 6.8
BuildRequires:  cmake(Qt6DBus) >= 6.8
BuildRequires:  cmake(Qt6Gui) >= 6.8
BuildRequires:  cmake(Qt6Test) >= 6.8
BuildRequires:  cmake(Qt6WebEngineCore) >= 6.8
BuildRequires:  cmake(Qt6WebEngineWidgets) >= 6.8
BuildRequires:  cmake(Qt6Widgets) >= 6.8
BuildRequires:  cmake(KF6Config) >= 6.0
BuildRequires:  cmake(KF6ConfigWidgets) >= 6.0
BuildRequires:  cmake(KF6CoreAddons) >= 6.0
BuildRequires:  cmake(KF6DBusAddons) >= 6.0
BuildRequires:  cmake(KF6I18n) >= 6.0
BuildRequires:  cmake(KF6Notifications) >= 6.0
BuildRequires:  cmake(KF6StatusNotifierItem) >= 6.0
BuildRequires:  cmake(KF6WidgetsAddons) >= 6.0
BuildRequires:  cmake(KF6WindowSystem) >= 6.0
BuildRequires:  cmake(KF6XmlGui) >= 6.0

Requires:       hicolor-icon-theme
Requires:       qt6-qtwebengine

%description
ThatsApp is a lightweight, native C++20 / Qt 6 / KDE Frameworks 6 desktop
client for WhatsApp Web designed for KDE Plasma 6 on Wayland. It provides
single-instance Wayland focus handoff, native KNotifications with anti-spam
snooze and per-chat cooldown controls, Plasma Task Manager unread badges,
privacy-strict hardware permissions, and whatsapp:// protocol handling.

%prep
%autosetup

%build
%cmake -GNinja -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install

%check
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.hjr555.thatsapp.desktop

%files
%{_bindir}/thatsapp
%{_datadir}/applications/io.github.hjr555.thatsapp.desktop
%{_datadir}/knotifications6/thatsapp.notifyrc
%{_datadir}/icons/hicolor/scalable/apps/io.github.hjr555.thatsapp.svg

%changelog
* Tue Nov 24 2026 Hywel <hywel@localhost> - 0.1.0-1
- Initial Fedora KDE Plasma 6 release
