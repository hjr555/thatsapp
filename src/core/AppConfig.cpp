#include "core/AppConfig.h"
#include "core/ZoomLevels.h"

#include <KConfigGroup>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

namespace hatsapp {

AppConfig::AppConfig(KSharedConfig::Ptr config)
    : m_config(config ? std::move(config) : KSharedConfig::openConfig(QStringLiteral("hatsapprc")))
{
}

bool AppConfig::showMenuBar() const
{
    return m_config->group(QStringLiteral("General")).readEntry("showMenuBar", false);
}

void AppConfig::setShowMenuBar(bool show)
{
    m_config->group(QStringLiteral("General")).writeEntry("showMenuBar", show);
}

bool AppConfig::sidebarCollapsed() const
{
    return m_config->group(QStringLiteral("General")).readEntry("sidebarCollapsed", false);
}

void AppConfig::setSidebarCollapsed(bool collapsed)
{
    m_config->group(QStringLiteral("General")).writeEntry("sidebarCollapsed", collapsed);
}

bool AppConfig::enableTrayIcon() const
{
    return m_config->group(QStringLiteral("General")).readEntry("enableTrayIcon", false);
}

void AppConfig::setEnableTrayIcon(bool enabled)
{
    m_config->group(QStringLiteral("General")).writeEntry("enableTrayIcon", enabled);
}

bool AppConfig::closeToTray() const
{
    return m_config->group(QStringLiteral("General")).readEntry("closeToTray", false);
}

void AppConfig::setCloseToTray(bool enabled)
{
    m_config->group(QStringLiteral("General")).writeEntry("closeToTray", enabled);
}

bool AppConfig::startMinimizedToTray() const
{
    return m_config->group(QStringLiteral("General")).readEntry("startMinimizedToTray", false);
}

void AppConfig::setStartMinimizedToTray(bool enabled)
{
    m_config->group(QStringLiteral("General")).writeEntry("startMinimizedToTray", enabled);
}

QString AppConfig::autostartDesktopFilePath()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(configDir).filePath(QStringLiteral("autostart/io.github.hjr555.hatsapp.desktop"));
}

bool AppConfig::autostart() const
{
    return QFile::exists(autostartDesktopFilePath());
}

void AppConfig::setAutostart(bool enabled)
{
    const QString path = autostartDesktopFilePath();
    if (!enabled) {
        QFile::remove(path);
        return;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=HatsApp\n"
            << "Exec=hatsapp" << ((enableTrayIcon() && startMinimizedToTray()) ? " --minimized" : "") << "\n"
            << "Icon=io.github.hjr555.hatsapp\n"
            << "X-GNOME-Autostart-enabled=true\n";
    }
}

QString AppConfig::lastDownloadDir() const
{
    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return m_config->group(QStringLiteral("General")).readEntry("lastDownloadDir", fallback);
}

void AppConfig::setLastDownloadDir(const QString &dir)
{
    if (!dir.isEmpty()) {
        m_config->group(QStringLiteral("General")).writeEntry("lastDownloadDir", dir);
    }
}

bool AppConfig::hideNotifications() const
{
    return m_config->group(QStringLiteral("Notifications")).readEntry("hideNotifications", false);
}

void AppConfig::setHideNotifications(bool hide)
{
    m_config->group(QStringLiteral("Notifications")).writeEntry("hideNotifications", hide);
}

bool AppConfig::suppressNotificationsWhenFocused() const
{
    return m_config->group(QStringLiteral("Notifications")).readEntry("suppressWhenFocused", true);
}

void AppConfig::setSuppressNotificationsWhenFocused(bool suppress)
{
    m_config->group(QStringLiteral("Notifications")).writeEntry("suppressWhenFocused", suppress);
}

bool AppConfig::coalesceNotificationsPerChat() const
{
    return m_config->group(QStringLiteral("Notifications")).readEntry("coalescePerChat", true);
}

void AppConfig::setCoalesceNotificationsPerChat(bool coalesce)
{
    m_config->group(QStringLiteral("Notifications")).writeEntry("coalescePerChat", coalesce);
}

int AppConfig::notificationCooldownSeconds() const
{
    const int val = m_config->group(QStringLiteral("Notifications")).readEntry("cooldownSeconds", 10);
    return std::clamp(val, 0, 300);
}

void AppConfig::setNotificationCooldownSeconds(int seconds)
{
    m_config->group(QStringLiteral("Notifications")).writeEntry("cooldownSeconds", std::clamp(seconds, 0, 300));
}

NotificationThrottler::Config AppConfig::notificationThrottlerConfig() const
{
    return {
        .hideNotifications = hideNotifications(),
        .suppressWhenFocused = suppressNotificationsWhenFocused(),
        .coalescePerChat = coalesceNotificationsPerChat(),
        .cooldownSeconds = notificationCooldownSeconds(),
    };
}

bool AppConfig::allowMicrophone() const
{
    return m_config->group(QStringLiteral("Privacy")).readEntry("allowMicrophone", false);
}

void AppConfig::setAllowMicrophone(bool allow)
{
    m_config->group(QStringLiteral("Privacy")).writeEntry("allowMicrophone", allow);
}

bool AppConfig::allowCamera() const
{
    return m_config->group(QStringLiteral("Privacy")).readEntry("allowCamera", false);
}

void AppConfig::setAllowCamera(bool allow)
{
    m_config->group(QStringLiteral("Privacy")).writeEntry("allowCamera", allow);
}

bool AppConfig::allowGeolocation() const
{
    return m_config->group(QStringLiteral("Privacy")).readEntry("allowGeolocation", false);
}

void AppConfig::setAllowGeolocation(bool allow)
{
    m_config->group(QStringLiteral("Privacy")).writeEntry("allowGeolocation", allow);
}

bool AppConfig::allowScreenCapture() const
{
    return m_config->group(QStringLiteral("Privacy")).readEntry("allowScreenCapture", false);
}

void AppConfig::setAllowScreenCapture(bool allow)
{
    m_config->group(QStringLiteral("Privacy")).writeEntry("allowScreenCapture", allow);
}

double AppConfig::zoomFactor() const
{
    return clampZoom(m_config->group(QStringLiteral("Web")).readEntry("zoomFactor", kDefaultZoom));
}

void AppConfig::setZoomFactor(double factor)
{
    m_config->group(QStringLiteral("Web")).writeEntry("zoomFactor", clampZoom(factor));
}

bool AppConfig::spellCheckEnabled() const
{
    return m_config->group(QStringLiteral("Web")).readEntry("spellCheckEnabled", true);
}

void AppConfig::setSpellCheckEnabled(bool enabled)
{
    m_config->group(QStringLiteral("Web")).writeEntry("spellCheckEnabled", enabled);
}

QString AppConfig::spellCheckLanguage() const
{
    return m_config->group(QStringLiteral("Web")).readEntry("spellCheckLanguage", QStringLiteral("en_GB"));
}

void AppConfig::setSpellCheckLanguage(const QString &language)
{
    if (!language.isEmpty()) {
        m_config->group(QStringLiteral("Web")).writeEntry("spellCheckLanguage", language);
    }
}

bool AppConfig::developerMode() const
{
    return m_config->group(QStringLiteral("Web")).readEntry("developerMode", false);
}

void AppConfig::setDeveloperMode(bool enabled)
{
    m_config->group(QStringLiteral("Web")).writeEntry("developerMode", enabled);
}

void AppConfig::sync()
{
    m_config->sync();
}

} // namespace hatsapp
