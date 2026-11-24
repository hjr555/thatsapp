#pragma once

#include "core/NotificationThrottler.h"

#include <KSharedConfig>
#include <QString>

namespace thatsapp {

class AppConfig {
public:
    explicit AppConfig(KSharedConfig::Ptr config = {});

    // General
    [[nodiscard]] bool showMenuBar() const;
    void setShowMenuBar(bool show);

    [[nodiscard]] bool sidebarCollapsed() const;
    void setSidebarCollapsed(bool collapsed);

    [[nodiscard]] bool enableTrayIcon() const;
    void setEnableTrayIcon(bool enabled);

    [[nodiscard]] bool closeToTray() const;
    void setCloseToTray(bool enabled);

    [[nodiscard]] bool startMinimizedToTray() const;
    void setStartMinimizedToTray(bool enabled);

    [[nodiscard]] bool autostart() const;
    void setAutostart(bool enabled);

    [[nodiscard]] QString lastDownloadDir() const;
    void setLastDownloadDir(const QString &dir);

    // Notifications
    [[nodiscard]] bool hideNotifications() const;
    void setHideNotifications(bool hide);

    [[nodiscard]] bool suppressNotificationsWhenFocused() const;
    void setSuppressNotificationsWhenFocused(bool suppress);

    [[nodiscard]] bool coalesceNotificationsPerChat() const;
    void setCoalesceNotificationsPerChat(bool coalesce);

    [[nodiscard]] int notificationCooldownSeconds() const;
    void setNotificationCooldownSeconds(int seconds);

    [[nodiscard]] NotificationThrottler::Config notificationThrottlerConfig() const;

    // Privacy (all hardware capabilities strictly denied by default)
    [[nodiscard]] bool allowMicrophone() const;
    void setAllowMicrophone(bool allow);

    [[nodiscard]] bool allowCamera() const;
    void setAllowCamera(bool allow);

    [[nodiscard]] bool allowGeolocation() const;
    void setAllowGeolocation(bool allow);

    [[nodiscard]] bool allowScreenCapture() const;
    void setAllowScreenCapture(bool allow);

    // Web
    [[nodiscard]] double zoomFactor() const;
    void setZoomFactor(double factor);

    [[nodiscard]] bool spellCheckEnabled() const;
    void setSpellCheckEnabled(bool enabled);

    [[nodiscard]] QString spellCheckLanguage() const;
    void setSpellCheckLanguage(const QString &language);

    [[nodiscard]] bool developerMode() const;
    void setDeveloperMode(bool enabled);

    void sync();

private:
    static QString autostartDesktopFilePath();

    KSharedConfig::Ptr m_config;
};

} // namespace thatsapp
