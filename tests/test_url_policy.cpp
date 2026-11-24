#include "core/UrlPolicy.h"

#include <QtTest/QtTest>

using hatsapp::UrlPolicy;

class TestUrlPolicy : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void internalWhatsAppUrlsAllowed()
    {
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://web.whatsapp.com/"))));
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://web.whatsapp.com/send?phone=447000000000"))));
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://mmg.whatsapp.net/v/t62.7118-24/file"))));
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://static.whatsapp.net/rsrc.php"))));
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("blob:https://web.whatsapp.com/1234-5678"))));
        QVERIFY(UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("about:blank"))));
    }

    void externalAndUnsafeUrlsRejectedAsInternal()
    {
        QVERIFY(!UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://example.com/"))));
        QVERIFY(!UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://evilwhatsapp.com/"))));
        QVERIFY(!UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("https://web.whatsapp.com.evil.example/"))));
        QVERIFY(!UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("http://web.whatsapp.com/"))));
        QVERIFY(!UrlPolicy::isInternalWhatsAppUrl(QUrl(QStringLiteral("javascript:alert(1)"))));
    }

    void externalSchemeFiltering()
    {
        QVERIFY(UrlPolicy::isSupportedExternalScheme(QUrl(QStringLiteral("https://kde.org"))));
        QVERIFY(UrlPolicy::isSupportedExternalScheme(QUrl(QStringLiteral("http://example.com"))));
        QVERIFY(UrlPolicy::isSupportedExternalScheme(QUrl(QStringLiteral("mailto:test@example.com"))));
        QVERIFY(!UrlPolicy::isSupportedExternalScheme(QUrl(QStringLiteral("file:///etc/passwd"))));
        QVERIFY(!UrlPolicy::isSupportedExternalScheme(QUrl(QStringLiteral("javascript:void(0)"))));
    }
};

QTEST_MAIN(TestUrlPolicy)
#include "test_url_policy.moc"
