#include "core/ProtocolRouter.h"

#include <QUrlQuery>

namespace thatsapp {

namespace {
constexpr auto kWebWhatsAppOrigin = "https://web.whatsapp.com/";
}

bool ProtocolRouter::isWhatsAppProtocol(const QUrl &url)
{
    if (!url.isValid()) {
        return false;
    }
    const QString scheme = url.scheme().toLower();
    return scheme == QStringLiteral("whatsapp") || scheme == QStringLiteral("whatsapp-smb");
}

QUrl ProtocolRouter::toWebWhatsAppUrl(const QUrl &url)
{
    if (!isWhatsAppProtocol(url)) {
        return {};
    }

    QString action = url.host().toLower();
    if (action.isEmpty()) {
        QString path = url.path();
        while (path.startsWith(QLatin1Char('/'))) {
            path.remove(0, 1);
        }
        action = path.section(QLatin1Char('/'), 0, 0).toLower();
    }

    const QUrlQuery incomingQuery(url);

    if (action == QStringLiteral("send")) {
        QUrl target(QStringLiteral("https://web.whatsapp.com/send"));
        QUrlQuery outQuery;
        const QString phone = incomingQuery.queryItemValue(QStringLiteral("phone"), QUrl::FullyDecoded).trimmed();
        const QString text = incomingQuery.queryItemValue(QStringLiteral("text"), QUrl::FullyDecoded);
        if (!phone.isEmpty()) {
            QString cleanedPhone;
            cleanedPhone.reserve(phone.size());
            for (const QChar ch : phone) {
                if (ch.isDigit()) {
                    cleanedPhone.append(ch);
                }
            }
            if (!cleanedPhone.isEmpty()) {
                outQuery.addQueryItem(QStringLiteral("phone"), cleanedPhone);
            }
        }
        if (!text.isEmpty()) {
            outQuery.addQueryItem(QStringLiteral("text"), text);
        }
        if (!outQuery.isEmpty()) {
            target.setQuery(outQuery);
        }
        return target;
    }

    if (action == QStringLiteral("chat") || action == QStringLiteral("accept")) {
        const QString code = incomingQuery.queryItemValue(QStringLiteral("code"), QUrl::FullyDecoded).trimmed();
        if (!code.isEmpty()) {
            QUrl target(QStringLiteral("https://web.whatsapp.com/accept"));
            QUrlQuery outQuery;
            outQuery.addQueryItem(QStringLiteral("code"), code);
            target.setQuery(outQuery);
            return target;
        }
    }

    return QUrl(QString::fromLatin1(kWebWhatsAppOrigin));
}

} // namespace thatsapp
