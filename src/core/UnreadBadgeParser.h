#pragma once

#include <QString>

namespace hatsapp {

class UnreadBadgeParser {
public:
    static int parseUnreadCount(const QString &documentTitle);
};

} // namespace hatsapp
