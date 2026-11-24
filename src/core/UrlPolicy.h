#pragma once

#include <QUrl>

namespace thatsapp {

class UrlPolicy {
public:
    static bool isInternalWhatsAppUrl(const QUrl &url);
    static bool isSupportedExternalScheme(const QUrl &url);
};

} // namespace thatsapp
