#include "app/WhatsAppPage.h"
#include "core/ProtocolRouter.h"
#include "core/UrlPolicy.h"

#include <QDesktopServices>
#include <QPointer>
#include <QWebEngineProfile>

namespace thatsapp {

namespace {

// Transient helper page that catches window.open() target URLs and opens external links in the default browser
class ExternalLinkInterceptorPage : public QWebEnginePage {
public:
    ExternalLinkInterceptorPage(QWebEngineProfile *profile, WhatsAppPage *owner)
        : QWebEnginePage(profile, owner)
        , m_owner(owner)
    {
    }

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType, bool) override
    {
        if (ProtocolRouter::isWhatsAppProtocol(url)) {
            const QUrl webUrl = ProtocolRouter::toWebWhatsAppUrl(url);
            if (webUrl.isValid() && m_owner) {
                Q_EMIT m_owner->internalWhatsAppUrlRequested(webUrl);
            }
        } else if (UrlPolicy::isInternalWhatsAppUrl(url)) {
            if (m_owner) {
                Q_EMIT m_owner->internalWhatsAppUrlRequested(url);
            }
        } else if (UrlPolicy::isSupportedExternalScheme(url)) {
            QDesktopServices::openUrl(url);
        }
        deleteLater();
        return false;
    }

private:
    QPointer<WhatsAppPage> m_owner;
};

} // namespace

WhatsAppPage::WhatsAppPage(QWebEngineProfile *profile, AppConfig &config, QObject *parent)
    : QWebEnginePage(profile, parent)
    , m_config(config)
{
    connect(this, &QWebEnginePage::permissionRequested, this, &WhatsAppPage::onPermissionRequested);
}

bool WhatsAppPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    if (ProtocolRouter::isWhatsAppProtocol(url)) {
        const QUrl mapped = ProtocolRouter::toWebWhatsAppUrl(url);
        if (mapped.isValid()) {
            Q_EMIT internalWhatsAppUrlRequested(mapped);
        }
        return false;
    }

    if (UrlPolicy::isInternalWhatsAppUrl(url)) {
        return true;
    }

    if (isMainFrame || type == NavigationTypeLinkClicked) {
        if (UrlPolicy::isSupportedExternalScheme(url)) {
            QDesktopServices::openUrl(url);
        }
        return false;
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

QWebEnginePage *WhatsAppPage::createWindow(WebWindowType type)
{
    Q_UNUSED(type);
    return new ExternalLinkInterceptorPage(profile(), this);
}

void WhatsAppPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                            const QString &message,
                                            int lineNumber,
                                            const QString &sourceID)
{
    if (message == QStringLiteral("__thatsapp:sidebar=0")) {
        Q_EMIT sidebarCollapsedChangedFromPage(false);
        return;
    }
    if (message == QStringLiteral("__thatsapp:sidebar=1")) {
        Q_EMIT sidebarCollapsedChangedFromPage(true);
        return;
    }
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}

void WhatsAppPage::onPermissionRequested(QWebEnginePermission permission)
{
    using PT = QWebEnginePermission::PermissionType;

    switch (permission.permissionType()) {
    case PT::Notifications:
    case PT::ClipboardReadWrite:
        permission.grant();
        return;

    case PT::MediaAudioCapture:
        if (m_config.allowMicrophone()) {
            permission.grant();
        } else {
            permission.deny();
        }
        return;

    case PT::MediaVideoCapture:
        if (m_config.allowCamera()) {
            permission.grant();
        } else {
            permission.deny();
        }
        return;

    case PT::MediaAudioVideoCapture:
        if (m_config.allowMicrophone() && m_config.allowCamera()) {
            permission.grant();
        } else {
            permission.deny();
        }
        return;

    case PT::Geolocation:
        if (m_config.allowGeolocation()) {
            permission.grant();
        } else {
            permission.deny();
        }
        return;

    case PT::DesktopVideoCapture:
    case PT::DesktopAudioVideoCapture:
        if (m_config.allowScreenCapture()) {
            permission.grant();
        } else {
            permission.deny();
        }
        return;

    default:
        permission.deny();
        return;
    }
}

} // namespace thatsapp
