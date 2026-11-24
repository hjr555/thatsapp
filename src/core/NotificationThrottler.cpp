#include "core/NotificationThrottler.h"

namespace hatsapp {

NotificationThrottler::NotificationThrottler(Config config)
    : m_config(config)
{
}

void NotificationThrottler::setConfig(const Config &config)
{
    m_config = config;
}

const NotificationThrottler::Config &NotificationThrottler::config() const
{
    return m_config;
}

void NotificationThrottler::snoozeFor(std::chrono::seconds duration, const QDateTime &nowUtc)
{
    if (duration.count() <= 0) {
        clearSnooze();
        return;
    }
    m_snoozeUntilUtc = nowUtc.toUTC().addSecs(duration.count());
}

void NotificationThrottler::clearSnooze()
{
    m_snoozeUntilUtc = {};
}

bool NotificationThrottler::isSnoozed(const QDateTime &nowUtc) const
{
    return m_snoozeUntilUtc.isValid() && nowUtc.toUTC() < m_snoozeUntilUtc;
}

QDateTime NotificationThrottler::snoozeUntilUtc() const
{
    return m_snoozeUntilUtc;
}

NotificationThrottler::Action NotificationThrottler::evaluate(
    const QString &chatKey,
    bool windowFocused,
    bool hasActiveNotificationForChat,
    const QDateTime &nowUtc)
{
    const QDateTime utc = nowUtc.toUTC();

    if (m_config.hideNotifications) {
        return Action::Suppress;
    }

    if (isSnoozed(utc)) {
        return Action::Suppress;
    }

    if (m_config.suppressWhenFocused && windowFocused) {
        return Action::Suppress;
    }

    const QString key = normalizeKey(chatKey);

    if (m_config.coalescePerChat && hasActiveNotificationForChat) {
        m_lastNotifiedUtc.insert(key, utc);
        return Action::ReplaceExisting;
    }

    if (m_config.cooldownSeconds > 0) {
        const auto it = m_lastNotifiedUtc.constFind(key);
        if (it != m_lastNotifiedUtc.constEnd()) {
            const qint64 elapsed = it->secsTo(utc);
            if (elapsed >= 0 && elapsed < m_config.cooldownSeconds) {
                return Action::Suppress;
            }
        }
    }

    m_lastNotifiedUtc.insert(key, utc);
    return Action::ShowNew;
}

void NotificationThrottler::resetHistory()
{
    m_lastNotifiedUtc.clear();
}

QString NotificationThrottler::normalizeKey(const QString &chatKey)
{
    const QString trimmed = chatKey.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("_default") : trimmed;
}

} // namespace hatsapp
