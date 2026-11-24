#include "core/ProtocolRouter.h"

#include <QUrlQuery>
#include <QtTest/QtTest>

using hatsapp::ProtocolRouter;

class TestProtocolRouter : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void recognizesWhatsAppSchemesOnly()
    {
        QVERIFY(ProtocolRouter::isWhatsAppProtocol(QUrl(QStringLiteral("whatsapp://send?phone=447123456789"))));
        QVERIFY(ProtocolRouter::isWhatsAppProtocol(QUrl(QStringLiteral("whatsapp-smb://send?phone=447123456789"))));
        QVERIFY(!ProtocolRouter::isWhatsAppProtocol(QUrl(QStringLiteral("tel:+447123456789"))));
        QVERIFY(!ProtocolRouter::isWhatsAppProtocol(QUrl(QStringLiteral("https://web.whatsapp.com/"))));
    }

    void mapsSendUriAndCleansPhone()
    {
        const QUrl out = ProtocolRouter::toWebWhatsAppUrl(
            QUrl(QStringLiteral("whatsapp://send?phone=+44 7123-456789&text=Hello%20KDE")));
        QCOMPARE(out.scheme(), QStringLiteral("https"));
        QCOMPARE(out.host(), QStringLiteral("web.whatsapp.com"));
        QCOMPARE(out.path(), QStringLiteral("/send"));

        const QUrlQuery q(out);
        QCOMPARE(q.queryItemValue(QStringLiteral("phone")), QStringLiteral("447123456789"));
        QCOMPARE(q.queryItemValue(QStringLiteral("text"), QUrl::FullyDecoded), QStringLiteral("Hello KDE"));
    }

    void mapsGroupInviteCode()
    {
        const QUrl out = ProtocolRouter::toWebWhatsAppUrl(
            QUrl(QStringLiteral("whatsapp://chat?code=AbCdEf12345")));
        QCOMPARE(out.toString(), QStringLiteral("https://web.whatsapp.com/accept?code=AbCdEf12345"));
    }

    void fallsBackSafelyToWebRoot()
    {
        const QUrl out = ProtocolRouter::toWebWhatsAppUrl(QUrl(QStringLiteral("whatsapp://unknown/path")));
        QCOMPARE(out.toString(), QStringLiteral("https://web.whatsapp.com/"));

        const QUrl invalid = ProtocolRouter::toWebWhatsAppUrl(QUrl(QStringLiteral("https://evil.example/send")));
        QVERIFY(!invalid.isValid());
    }
};

QTEST_MAIN(TestProtocolRouter)
#include "test_protocol_router.moc"
