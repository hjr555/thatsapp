#include "core/NotificationThrottler.h"

#include <QtTest/QtTest>

using thatsapp::NotificationThrottler;
using Action = NotificationThrottler::Action;

class TestNotificationThrottler : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void respectsHideAndSnooze()
    {
        NotificationThrottler throttler({
            .hideNotifications = true,
            .suppressWhenFocused = true,
            .coalescePerChat = true,
            .cooldownSeconds = 10,
        });
        const QDateTime t0 = QDateTime::fromSecsSinceEpoch(1'700'000'000, QTimeZone::UTC);

        QCOMPARE(throttler.evaluate(QStringLiteral("Alice"), false, false, t0), Action::Suppress);

        auto cfg = throttler.config();
        cfg.hideNotifications = false;
        throttler.setConfig(cfg);

        throttler.snoozeFor(std::chrono::minutes(15), t0);
        QVERIFY(throttler.isSnoozed(t0.addSecs(60)));
        QCOMPARE(throttler.evaluate(QStringLiteral("Alice"), false, false, t0.addSecs(60)), Action::Suppress);
        QVERIFY(!throttler.isSnoozed(t0.addSecs(15 * 60)));
        QCOMPARE(throttler.evaluate(QStringLiteral("Alice"), false, false, t0.addSecs(15 * 60)), Action::ShowNew);
    }

    void suppressesWhenFocusedAndEnforcesCooldownPerChat()
    {
        NotificationThrottler throttler({
            .hideNotifications = false,
            .suppressWhenFocused = true,
            .coalescePerChat = true,
            .cooldownSeconds = 10,
        });
        const QDateTime t0 = QDateTime::fromSecsSinceEpoch(1'700'000'000, QTimeZone::UTC);

        // Suppressed while window is focused
        QCOMPARE(throttler.evaluate(QStringLiteral("Group"), true, false, t0), Action::Suppress);

        // First unfocused message shows
        QCOMPARE(throttler.evaluate(QStringLiteral("Group"), false, false, t0), Action::ShowNew);

        // Same chat with active notification replaces in-place (no new popup spam)
        QCOMPARE(throttler.evaluate(QStringLiteral("Group"), false, true, t0.addSecs(2)), Action::ReplaceExisting);

        // Same chat after notification closed, still within 10s cooldown -> suppressed
        QCOMPARE(throttler.evaluate(QStringLiteral("Group"), false, false, t0.addSecs(5)), Action::Suppress);

        // Different chat is NOT blocked by Group's cooldown
        QCOMPARE(throttler.evaluate(QStringLiteral("Bob"), false, false, t0.addSecs(5)), Action::ShowNew);

        // Group after cooldown expires -> shows again
        QCOMPARE(throttler.evaluate(QStringLiteral("Group"), false, false, t0.addSecs(15)), Action::ShowNew);
    }
};

QTEST_MAIN(TestNotificationThrottler)
#include "test_notification_throttler.moc"
