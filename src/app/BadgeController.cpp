#include "app/BadgeController.h"
#include "core/UnreadBadgeParser.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

namespace hatsapp {

BadgeController::BadgeController(QObject *parent)
    : QObject(parent)
{
}

void BadgeController::updateFromDocumentTitle(const QString &title)
{
    setUnreadCount(UnreadBadgeParser::parseUnreadCount(title));
}

void BadgeController::setUnreadCount(int count)
{
    const int clamped = qMax(0, count);
    if (clamped == m_unreadCount) {
        return;
    }
    m_unreadCount = clamped;
    publishLauncherBadge(m_unreadCount);
    Q_EMIT unreadCountChanged(m_unreadCount);
}

void BadgeController::publishLauncherBadge(int count)
{
    QDBusMessage signal = QDBusMessage::createSignal(
        QStringLiteral("/io/github/hjr555/hatsapp"),
        QStringLiteral("com.canonical.Unity.LauncherEntry"),
        QStringLiteral("Update"));

    QVariantMap props;
    props.insert(QStringLiteral("count"), static_cast<qint64>(count));
    props.insert(QStringLiteral("count-visible"), count > 0);

    signal << QStringLiteral("application://io.github.hjr555.hatsapp.desktop") << props;
    QDBusConnection::sessionBus().send(signal);
}

} // namespace hatsapp
