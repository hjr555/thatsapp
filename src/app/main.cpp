#include "app/MainWindow.h"
#include "core/AppConfig.h"

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("hatsapp"));

    KAboutData aboutData(
        QStringLiteral("hatsapp"),
        i18n("HatsApp"),
        QStringLiteral(HATSAPP_VERSION),
        i18n("Native KDE Plasma 6 desktop client for WhatsApp Web"),
        KAboutLicense::MIT,
        i18n("© 2026 Hywel"));
    aboutData.setOrganizationDomain(QByteArrayLiteral("hjr555.github.io"));
    aboutData.setDesktopFileName(QStringLiteral("io.github.hjr555.hatsapp"));
    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.hjr555.hatsapp"));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    const QCommandLineOption minimizedOption(
        QStringLiteral("minimized"),
        i18n("Start minimised to the system tray (when system tray is enabled)"));
    parser.addOption(minimizedOption);
    parser.addPositionalArgument(
        QStringLiteral("url"),
        i18n("Optional whatsapp:// URI to open"),
        QStringLiteral("[url]"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // Enforce single instance via KDE D-Bus (with automatic Wayland xdg_activation_v1 handoff)
    KDBusService dbusService(KDBusService::Unique);

    hatsapp::AppConfig config;
    hatsapp::MainWindow window(config);

    QObject::connect(&dbusService, &KDBusService::activateRequested,
                     &window, [&window](const QStringList &arguments, const QString &) {
                         window.activateAndFocus();
                         for (int i = 1; i < arguments.size(); ++i) {
                             if (arguments.at(i).startsWith(QStringLiteral("whatsapp"), Qt::CaseInsensitive)) {
                                 window.handleExternalUri(arguments.at(i));
                                 break;
                             }
                         }
                     });

    const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        window.handleExternalUri(positional.first());
    }

    const bool startInTray = config.enableTrayIcon()
        && (parser.isSet(minimizedOption) || config.startMinimizedToTray());

    if (!startInTray) {
        window.show();
    }

    return app.exec();
}
