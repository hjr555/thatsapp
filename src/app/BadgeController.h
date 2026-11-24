#pragma once

#include <QObject>
#include <QString>

namespace hatsapp {

class BadgeController : public QObject {
    Q_OBJECT
public:
    explicit BadgeController(QObject *parent = nullptr);

    void updateFromDocumentTitle(const QString &title);
    void setUnreadCount(int count);
    [[nodiscard]] int unreadCount() const { return m_unreadCount; }

Q_SIGNALS:
    void unreadCountChanged(int count);

private:
    void publishLauncherBadge(int count);

    int m_unreadCount = 0;
};

} // namespace hatsapp
