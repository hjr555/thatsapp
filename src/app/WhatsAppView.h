#pragma once

#include "core/AppConfig.h"

#include <QPointer>
#include <QWebEngineView>

class QWebEngineDownloadRequest;

namespace hatsapp {

class WhatsAppView : public QWebEngineView {
    Q_OBJECT
public:
    explicit WhatsAppView(AppConfig &config, QWidget *parent = nullptr);

    void zoomInStep();
    void zoomOutStep();
    void resetZoomStep();
    void applySavedZoom();

    void setMenuBarVisibleState(bool visible);
    void setSidebarCollapsedState(bool collapsed);

Q_SIGNALS:
    void zoomFactorChanged(double zoom);
    void inspectElementRequested();
    void toggleMenuBarRequested();
    void toggleSidebarRequested();
    void openSettingsRequested();
    void toggleHideNotificationsRequested();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;

private:
    void attachFocusProxyFilter();
    bool handleAppShortcutKey(int key, Qt::KeyboardModifiers mods);

    AppConfig &m_config;
    QPointer<QObject> m_installedFocusProxy;
    bool m_menuBarVisible = false;
    bool m_sidebarCollapsed = false;
};

} // namespace hatsapp
