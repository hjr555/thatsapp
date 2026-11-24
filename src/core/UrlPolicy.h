#pragma once

#include <QUrl>

namespace hatsapp {

class UrlPolicy {
public:
    static bool isInternalWhatsAppUrl(const QUrl &url);
    static bool isSupportedExternalScheme(const QUrl &url);
};

} // namespace hatsapp
