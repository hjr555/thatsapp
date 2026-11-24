#pragma once

#include "core/AppConfig.h"

#include <QPointer>
#include <QWebEngineView>

class QWebEngineDownloadRequest;

namespace thatsapp {

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

private:
    AppConfig &m_config;
    bool m_menuBarVisible = false;
    bool m_sidebarCollapsed = false;
};

} // namespace thatsapp
