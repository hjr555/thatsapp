#pragma once

#include <QUrl>

namespace thatsapp {

class ProtocolRouter {
public:
    static bool isWhatsAppProtocol(const QUrl &url);
    static QUrl toWebWhatsAppUrl(const QUrl &url);
};

} // namespace thatsapp
