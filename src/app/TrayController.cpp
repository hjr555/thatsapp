#include "app/TrayController.h"

#include <KLocalizedString>
#include <KStatusNotifierItem>

namespace thatsapp {

TrayController::TrayController(QObject *parent)
    : QObject(parent)
{
}

TrayController::~TrayController()
{
    destroyTray();
}

void TrayController::setEnabled(bool enabled)
{
    if (enabled == isEnabled()) {
        return;
    }
    if (enabled) {
        createTray();
    } else {
        destroyTray();
    }
}

bool TrayController::isEnabled() const
{
    return m_sni != nullptr;
}

void TrayController::setUnreadCount(int count)
{
    m_unreadCount = qMax(0, count);
    if (!m_sni) {
        return;
    }
    m_sni->setStatus(m_unreadCount > 0 ? KStatusNotifierItem::NeedsAttention : KStatusNotifierItem::Active);
    updateToolTip();
}

void TrayController::setNotificationMutedOrSnoozed(bool hidden, bool snoozed)
{
    m_hidden = hidden;
    m_snoozed = snoozed;
    if (m_hideNotifAction) {
        m_hideNotifAction->setChecked(hidden);
    }
    if (m_clearSnoozeAction) {
        m_clearSnoozeAction->setVisible(snoozed);
    }
    updateToolTip();
}

void TrayController::createTray()
{
    m_sni = new KStatusNotifierItem(QStringLiteral("io.github.hjr555.thatsapp"), this);
    m_sni->setCategory(KStatusNotifierItem::Communications);
    m_sni->setIconByName(QStringLiteral("io.github.hjr555.thatsapp"));
    m_sni->setAttentionIconByName(QStringLiteral("io.github.hjr555.thatsapp"));
    m_sni->setTitle(i18n("ThatsApp"));
    m_sni->setStandardActionsEnabled(false);

    m_menu = new QMenu();
    auto *toggleAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("window-restore")), i18n("Show / Hide ThatsApp"));
    connect(toggleAction, &QAction::triggered, this, [this]() {
        Q_EMIT toggleWindowRequested(m_sni ? m_sni->providedToken() : QString());
    });

    m_menu->addSeparator();

    m_hideNotifAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("notifications-disabled")), i18n("Hide Notifications"));
    m_hideNotifAction->setCheckable(true);
    m_hideNotifAction->setChecked(m_hidden);
    connect(m_hideNotifAction, &QAction::toggled, this, &TrayController::toggleHideNotificationsRequested);

    auto *snoozeMenu = m_menu->addMenu(QIcon::fromTheme(QStringLiteral("chronometer")), i18n("Snooze Notifications"));
    connect(snoozeMenu->addAction(i18n("15 minutes")), &QAction::triggered, this, [this]() { Q_EMIT snoozeRequested(15); });
    connect(snoozeMenu->addAction(i18n("1 hour")), &QAction::triggered, this, [this]() { Q_EMIT snoozeRequested(60); });
    connect(snoozeMenu->addAction(i18n("8 hours")), &QAction::triggered, this, [this]() { Q_EMIT snoozeRequested(480); });

    m_clearSnoozeAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("notifications")), i18n("Resume Notifications"));
    m_clearSnoozeAction->setVisible(m_snoozed);
    connect(m_clearSnoozeAction, &QAction::triggered, this, &TrayController::clearSnoozeRequested);

    m_menu->addSeparator();

    auto *settingsAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure ThatsApp..."));
    connect(settingsAction, &QAction::triggered, this, &TrayController::settingsRequested);

    auto *quitAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayController::quitRequested);

    m_sni->setContextMenu(m_menu);

    connect(m_sni, &KStatusNotifierItem::activateRequested, this, [this](bool, const QPoint &) {
        Q_EMIT toggleWindowRequested(m_sni ? m_sni->providedToken() : QString());
    });

    setUnreadCount(m_unreadCount);
    updateToolTip();
}

void TrayController::destroyTray()
{
    if (m_sni) {
        m_sni->deleteLater();
        m_sni = nullptr;
    }
    if (m_menu) {
        m_menu->deleteLater();
        m_menu = nullptr;
    }
    m_hideNotifAction = nullptr;
    m_clearSnoozeAction = nullptr;
}

void TrayController::updateToolTip()
{
    if (!m_sni) {
        return;
    }
    QString subTitle;
    if (m_unreadCount > 0) {
        subTitle = i18np("%1 unread chat", "%1 unread chats", m_unreadCount);
    } else {
        subTitle = i18n("WhatsApp Desktop Client");
    }
    if (m_hidden) {
        subTitle += i18n(" (Notifications hidden)");
    } else if (m_snoozed) {
        subTitle += i18n(" (Notifications snoozed)");
    }
    m_sni->setToolTip(QStringLiteral("io.github.hjr555.thatsapp"), i18n("ThatsApp"), subTitle);
}

} // namespace thatsapp
