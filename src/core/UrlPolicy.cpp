#include "core/UrlPolicy.h"

namespace thatsapp {

bool UrlPolicy::isInternalWhatsAppUrl(const QUrl &url)
{
    if (!url.isValid()) {
        return false;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("blob") || scheme == QStringLiteral("data")) {
        return true;
    }
    if (scheme == QStringLiteral("about") && url.path().toLower() == QStringLiteral("blank")) {
        return true;
    }
    if (scheme != QStringLiteral("https")) {
        return false;
    }

    const QString host = url.host().toLower();
    if (host.isEmpty()) {
        return false;
    }

    return host == QStringLiteral("web.whatsapp.com")
        || host == QStringLiteral("whatsapp.com")
        || host.endsWith(QStringLiteral(".whatsapp.com"))
        || host == QStringLiteral("whatsapp.net")
        || host.endsWith(QStringLiteral(".whatsapp.net"));
}

bool UrlPolicy::isSupportedExternalScheme(const QUrl &url)
{
    if (!url.isValid()) {
        return false;
    }
    const QString scheme = url.scheme().toLower();
    return scheme == QStringLiteral("https")
        || scheme == QStringLiteral("http")
        || scheme == QStringLiteral("mailto");
}

} // namespace thatsapp
