#include "core/UnreadBadgeParser.h"

#include <QRegularExpression>

namespace hatsapp {

int UnreadBadgeParser::parseUnreadCount(const QString &documentTitle)
{
    static const QRegularExpression re(QStringLiteral(R"(^\s*\((\d+)\+?\)\s*)"));
    const QRegularExpressionMatch match = re.match(documentTitle);
    if (!match.hasMatch()) {
        return 0;
    }
    bool ok = false;
    const int value = match.captured(1).toInt(&ok);
    return (ok && value > 0) ? value : 0;
}

} // namespace hatsapp
