#include "core/UnreadBadgeParser.h"
#include "core/ZoomLevels.h"

#include <QtTest/QtTest>

using hatsapp::UnreadBadgeParser;

class TestUnreadBadgeParser : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void parsesWhatsAppTitles()
    {
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("WhatsApp")), 0);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("(0) WhatsApp")), 0);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("(3) WhatsApp")), 3);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("(42) WhatsApp")), 42);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("(99+) WhatsApp")), 99);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("  (7) WhatsApp - Chat")), 7);
        QCOMPARE(UnreadBadgeParser::parseUnreadCount(QStringLiteral("WhatsApp (3)")), 0);
    }

    void zoomStepBounds()
    {
        QCOMPARE(hatsapp::nextZoomIn(1.0), 1.1);
        QCOMPARE(hatsapp::nextZoomOut(1.0), 0.9);
        QCOMPARE(hatsapp::nextZoomIn(1.5), 1.5);
        QCOMPARE(hatsapp::nextZoomOut(0.8), 0.8);
        QCOMPARE(hatsapp::clampZoom(5.0), 1.5);
        QCOMPARE(hatsapp::clampZoom(0.1), 0.8);
    }
};

QTEST_MAIN(TestUnreadBadgeParser)
#include "test_unread_badge_parser.moc"
