#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>

#include <chrono>

namespace thatsapp {

class NotificationThrottler {
public:
    enum class Action {
        ShowNew,
        ReplaceExisting,
        Suppress
    };

    struct Config {
        bool hideNotifications = false;
        bool suppressWhenFocused = true;
        bool coalescePerChat = true;
        int cooldownSeconds = 10;
    };

    NotificationThrottler() = default;
    explicit NotificationThrottler(Config config);

    void setConfig(const Config &config);
    [[nodiscard]] const Config &config() const;

    void snoozeFor(std::chrono::seconds duration, const QDateTime &nowUtc = QDateTime::currentDateTimeUtc());
    void clearSnooze();
    [[nodiscard]] bool isSnoozed(const QDateTime &nowUtc = QDateTime::currentDateTimeUtc()) const;
    [[nodiscard]] QDateTime snoozeUntilUtc() const;

    [[nodiscard]] Action evaluate(
        const QString &chatKey,
        bool windowFocused,
        bool hasActiveNotificationForChat,
        const QDateTime &nowUtc = QDateTime::currentDateTimeUtc());

    void resetHistory();

private:
    static QString normalizeKey(const QString &chatKey);

    Config m_config;
    QDateTime m_snoozeUntilUtc;
    QHash<QString, QDateTime> m_lastNotifiedUtc;
};

} // namespace thatsapp
