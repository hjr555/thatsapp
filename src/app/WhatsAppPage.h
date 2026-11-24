#pragma once

#include "core/AppConfig.h"

#include <QWebEnginePage>
#include <QWebEnginePermission>

class QWebEngineProfile;

namespace thatsapp {

class WhatsAppPage : public QWebEnginePage {
    Q_OBJECT
public:
    WhatsAppPage(QWebEngineProfile *profile, AppConfig &config, QObject *parent = nullptr);

Q_SIGNALS:
    void internalWhatsAppUrlRequested(const QUrl &url);
    void sidebarCollapsedChangedFromPage(bool collapsed);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    QWebEnginePage *createWindow(WebWindowType type) override;
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override;

private Q_SLOTS:
    void onPermissionRequested(QWebEnginePermission permission);

private:
    AppConfig &m_config;
};

} // namespace thatsapp
