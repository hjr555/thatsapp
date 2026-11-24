#pragma once

#include "core/AppConfig.h"

#include <KMainWindow>
#include <QPointer>
#include <QUrl>

class QAction;
class QWebEngineDownloadRequest;
class QWebEngineProfile;
class QWebEngineView;

namespace thatsapp {

class BadgeController;
class NotificationController;
class TrayController;
class WhatsAppPage;
class WhatsAppView;

class MainWindow : public KMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppConfig &config, QWidget *parent = nullptr);
    ~MainWindow() override;

    void activateAndFocus(const QString &xdgActivationToken = {});
    void handleExternalUri(const QString &rawUri);
    void requestQuit();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void onSettingsApplied();
    void onDownloadRequested(QWebEngineDownloadRequest *download);
    void openSettingsDialog();
    void toggleDevTools();
    void toggleMenuBar();
    void setMenuBarVisibleWithHint(bool visible, bool showHideHint);
    void toggleSidebar();
    void setSidebarCollapsed(bool collapsed, bool pushToWeb = true);
    void updateWindowTitle();

private:
    void setupWebEngine();
    void installSidebarCollapseScript();
    void setupMenuBarAndShortcuts();
    void applyWebProfileSettings();
    void updateNotificationMenuState();
    static QString buildCleanUserAgent(const QString &defaultUserAgent);

    AppConfig &m_config;
    QWebEngineProfile *m_profile = nullptr;
    WhatsAppPage *m_page = nullptr;
    WhatsAppView *m_view = nullptr;
    QPointer<QWebEngineView> m_devToolsView;

    BadgeController *m_badgeController = nullptr;
    NotificationController *m_notificationController = nullptr;
    TrayController *m_trayController = nullptr;

    QAction *m_showMenuBarAction = nullptr;
    QAction *m_collapseSidebarAction = nullptr;
    QAction *m_hideNotifAction = nullptr;
    QAction *m_clearSnoozeAction = nullptr;
    QAction *m_devToolsAction = nullptr;

    bool m_quitting = false;
    bool m_sidebarCollapsed = false;
};

} // namespace thatsapp
