#pragma once

#include "core/AppConfig.h"
#include "core/NotificationThrottler.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <chrono>
#include <memory>

class KNotification;
class QWebEngineNotification;

namespace hatsapp {

class NotificationController : public QObject {
    Q_OBJECT
public:
    explicit NotificationController(AppConfig &config, QObject *parent = nullptr);
    ~NotificationController() override;

    void setWindowFocused(bool focused);
    void reloadConfig();

    void setHideNotifications(bool hide);
    [[nodiscard]] bool isHidden() const;

    void snoozeFor(std::chrono::seconds duration);
    void clearSnooze();
    [[nodiscard]] bool isSnoozed() const;
    [[nodiscard]] QDateTime snoozeUntilUtc() const;

    void presentWebNotification(std::unique_ptr<QWebEngineNotification> notification);
    void showDownloadComplete(const QString &filePath);

Q_SIGNALS:
    void activateMainWindowRequested(const QString &xdgActivationToken);
    void notificationStateChanged();

private:
    struct ActiveEntry {
        QPointer<KNotification> kNotif;
        std::shared_ptr<QWebEngineNotification> webNotif;
    };

    void updateSnoozeTimer();
    void dismissAllActive();

    AppConfig &m_config;
    NotificationThrottler m_throttler;
    bool m_windowFocused = false;
    QTimer m_snoozeTimer;
    QHash<QString, ActiveEntry> m_activeByChat;
};

} // namespace hatsapp
