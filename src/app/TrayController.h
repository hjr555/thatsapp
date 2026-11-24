#pragma once

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPointer>

class KStatusNotifierItem;

namespace thatsapp {

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(QObject *parent = nullptr);
    ~TrayController() override;

    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;

    void setUnreadCount(int count);
    void setNotificationMutedOrSnoozed(bool hidden, bool snoozed);

Q_SIGNALS:
    void toggleWindowRequested(const QString &xdgActivationToken);
    void toggleHideNotificationsRequested(bool hide);
    void snoozeRequested(int minutes);
    void clearSnoozeRequested();
    void settingsRequested();
    void quitRequested();

private:
    void createTray();
    void destroyTray();
    void updateToolTip();

    QPointer<KStatusNotifierItem> m_sni;
    QPointer<QMenu> m_menu;
    QPointer<QAction> m_hideNotifAction;
    QPointer<QAction> m_clearSnoozeAction;
    int m_unreadCount = 0;
    bool m_hidden = false;
    bool m_snoozed = false;
};

} // namespace thatsapp
