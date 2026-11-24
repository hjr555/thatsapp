#pragma once

#include <QString>

namespace thatsapp {

class UnreadBadgeParser {
public:
    static int parseUnreadCount(const QString &documentTitle);
};

} // namespace thatsapp
