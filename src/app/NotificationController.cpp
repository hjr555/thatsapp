#include "app/NotificationController.h"

#include <KLocalizedString>
#include <KNotification>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QFileInfo>
#include <QPixmap>
#include <QUrl>
#include <QWebEngineNotification>

#include <climits>

namespace hatsapp {

NotificationController::NotificationController(AppConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_throttler(config.notificationThrottlerConfig())
{
    m_snoozeTimer.setSingleShot(true);
    connect(&m_snoozeTimer, &QTimer::timeout, this, [this]() {
        m_throttler.clearSnooze();
        Q_EMIT notificationStateChanged();
    });
}

NotificationController::~NotificationController()
{
    dismissAllActive();
}

void NotificationController::setWindowFocused(bool focused)
{
    m_windowFocused = focused;
    if (focused && m_config.suppressNotificationsWhenFocused()) {
        dismissAllActive();
    }
}

void NotificationController::reloadConfig()
{
    m_throttler.setConfig(m_config.notificationThrottlerConfig());
    if (m_config.hideNotifications()) {
        dismissAllActive();
    }
    Q_EMIT notificationStateChanged();
}

void NotificationController::setHideNotifications(bool hide)
{
    m_config.setHideNotifications(hide);
    m_config.sync();
    reloadConfig();
}

bool NotificationController::isHidden() const
{
    return m_config.hideNotifications();
}

void NotificationController::snoozeFor(std::chrono::seconds duration)
{
    m_throttler.snoozeFor(duration);
    dismissAllActive();
    updateSnoozeTimer();
    Q_EMIT notificationStateChanged();
}

void NotificationController::clearSnooze()
{
    m_throttler.clearSnooze();
    m_snoozeTimer.stop();
    Q_EMIT notificationStateChanged();
}

bool NotificationController::isSnoozed() const
{
    return m_throttler.isSnoozed();
}

QDateTime NotificationController::snoozeUntilUtc() const
{
    return m_throttler.snoozeUntilUtc();
}

void NotificationController::updateSnoozeTimer()
{
    const QDateTime until = m_throttler.snoozeUntilUtc();
    if (!until.isValid()) {
        m_snoozeTimer.stop();
        return;
    }
    const qint64 remainingMs = QDateTime::currentDateTimeUtc().msecsTo(until);
    if (remainingMs <= 0) {
        clearSnooze();
        return;
    }
    m_snoozeTimer.start(static_cast<int>(qMin<qint64>(remainingMs, INT_MAX)));
}

void NotificationController::dismissAllActive()
{
    const auto entries = m_activeByChat;
    m_activeByChat.clear();
    for (const auto &entry : entries) {
        if (entry.kNotif) {
            entry.kNotif->close();
        }
        if (entry.webNotif) {
            entry.webNotif->close();
        }
    }
}

void NotificationController::presentWebNotification(std::unique_ptr<QWebEngineNotification> notification)
{
    if (!notification) {
        return;
    }

    std::shared_ptr<QWebEngineNotification> webNotif(std::move(notification));

    const QString title = webNotif->title().trimmed();
    const QString chatKey = !title.isEmpty() ? title : webNotif->tag();

    auto existingIt = m_activeByChat.find(chatKey);
    const bool hasActive = (existingIt != m_activeByChat.end() && existingIt->kNotif != nullptr);

    const auto action = m_throttler.evaluate(chatKey, m_windowFocused, hasActive);
    if (action == NotificationThrottler::Action::Suppress) {
        webNotif->close();
        return;
    }

    if (action == NotificationThrottler::Action::ReplaceExisting && hasActive) {
        if (existingIt->webNotif) {
            existingIt->webNotif->close();
        }
        existingIt->webNotif = webNotif;
        KNotification *kNotif = existingIt->kNotif;
        kNotif->setTitle(title.isEmpty() ? i18n("HatsApp") : title);
        kNotif->setText(webNotif->message());
        if (!webNotif->icon().isNull()) {
            kNotif->setPixmap(QPixmap::fromImage(webNotif->icon()));
        }
        webNotif->show();
        kNotif->sendEvent();
        return;
    }

    auto *kNotif = new KNotification(QStringLiteral("newMessage"));
    kNotif->setComponentName(QStringLiteral("hatsapp"));
    kNotif->setTitle(title.isEmpty() ? i18n("HatsApp") : title);
    kNotif->setText(webNotif->message());
    if (!webNotif->icon().isNull()) {
        kNotif->setPixmap(QPixmap::fromImage(webNotif->icon()));
    } else {
        kNotif->setIconName(QStringLiteral("io.github.hjr555.hatsapp"));
    }

    m_activeByChat.insert(chatKey, ActiveEntry{kNotif, webNotif});

    auto *defaultAction = kNotif->addDefaultAction(i18n("Open Chat"));
    connect(defaultAction, &KNotificationAction::activated, this, [this, chatKey, kNotif]() {
        const QString token = kNotif->xdgActivationToken();
        Q_EMIT activateMainWindowRequested(token);
        const auto it = m_activeByChat.constFind(chatKey);
        if (it != m_activeByChat.constEnd() && it->webNotif) {
            it->webNotif->click();
        }
    });

    auto *snoozeAction = kNotif->addAction(i18n("Snooze 1 hour"));
    connect(snoozeAction, &KNotificationAction::activated, this, [this]() {
        snoozeFor(std::chrono::hours(1));
    });

    connect(kNotif, &KNotification::closed, this, [this, chatKey, kNotif]() {
        const auto it = m_activeByChat.find(chatKey);
        if (it != m_activeByChat.end() && it->kNotif == kNotif) {
            if (it->webNotif) {
                it->webNotif->close();
            }
            m_activeByChat.erase(it);
        }
    });

    connect(webNotif.get(), &QWebEngineNotification::closed, kNotif, [kNotif]() {
        if (kNotif) {
            kNotif->close();
        }
    });

    webNotif->show();
    kNotif->sendEvent();
}

void NotificationController::showDownloadComplete(const QString &filePath)
{
    const QFileInfo info(filePath);
    auto *kNotif = new KNotification(QStringLiteral("downloadComplete"));
    kNotif->setComponentName(QStringLiteral("hatsapp"));
    kNotif->setIconName(QStringLiteral("io.github.hjr555.hatsapp"));
    kNotif->setTitle(i18n("Download Complete"));
    kNotif->setText(info.fileName());

    auto *openFolder = kNotif->addAction(i18n("Show in Folder"));
    connect(openFolder, &KNotificationAction::activated, this, [filePath, dir = info.absolutePath()]() {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.FileManager1"),
            QStringLiteral("/org/freedesktop/FileManager1"),
            QStringLiteral("org.freedesktop.FileManager1"),
            QStringLiteral("ShowItems"));
        msg << QStringList{QUrl::fromLocalFile(filePath).toString()} << QString();
        const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        }
    });

    auto *openFile = kNotif->addDefaultAction(i18n("Open File"));
    connect(openFile, &KNotificationAction::activated, this, [filePath]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    });

    kNotif->sendEvent();
}

} // namespace hatsapp
