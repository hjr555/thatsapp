#include "app/SettingsDialog.h"
#include "core/ZoomLevels.h"

#include <KLocalizedString>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <cmath>

namespace thatsapp {

SettingsDialog::SettingsDialog(AppConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle(i18n("Configure ThatsApp"));
    setMinimumWidth(460);

    auto *mainLayout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs);

    // --- General tab ---
    auto *generalTab = new QWidget(tabs);
    auto *generalLayout = new QVBoxLayout(generalTab);

    m_enableTrayCheck = new QCheckBox(i18n("Show system tray icon (Plasma StatusNotifier)"), generalTab);
    m_closeToTrayCheck = new QCheckBox(i18n("Minimise to system tray when closing the window"), generalTab);
    m_startMinimizedCheck = new QCheckBox(i18n("Start minimised to system tray"), generalTab);
    m_autostartCheck = new QCheckBox(i18n("Launch ThatsApp automatically at login"), generalTab);

    generalLayout->addWidget(m_enableTrayCheck);
    generalLayout->addWidget(m_closeToTrayCheck);
    generalLayout->addWidget(m_startMinimizedCheck);
    generalLayout->addSpacing(8);
    generalLayout->addWidget(m_autostartCheck);
    generalLayout->addStretch();
    tabs->addTab(generalTab, QIcon::fromTheme(QStringLiteral("configure")), i18n("General"));

    connect(m_enableTrayCheck, &QCheckBox::toggled, this, &SettingsDialog::updateDependentControls);

    // --- Notifications tab ---
    auto *notifTab = new QWidget(tabs);
    auto *notifLayout = new QVBoxLayout(notifTab);

    m_hideNotifCheck = new QCheckBox(i18n("Hide all desktop notifications"), notifTab);
    m_suppressFocusedCheck = new QCheckBox(i18n("Suppress popups while ThatsApp is focused"), notifTab);
    m_coalesceCheck = new QCheckBox(i18n("Replace active popup for the same chat instead of stacking"), notifTab);

    auto *cooldownRow = new QFormLayout();
    m_cooldownSpin = new QSpinBox(notifTab);
    m_cooldownSpin->setRange(0, 300);
    m_cooldownSpin->setSuffix(i18n(" seconds"));
    m_cooldownSpin->setSpecialValueText(i18n("Off (0 s)"));
    cooldownRow->addRow(i18n("Per-chat popup cooldown:"), m_cooldownSpin);

    auto *kdeNotifBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("preferences-desktop-notification-bell")),
                                        i18n("Configure Sounds & Popups in KDE System Settings..."),
                                        notifTab);
    connect(kdeNotifBtn, &QPushButton::clicked, this, &SettingsDialog::openKdeNotificationSettings);

    auto *soundHint = new QLabel(
        i18n("KDE notifications are silent by default so WhatsApp Web keeps its own tones without double-dinging."),
        notifTab);
    soundHint->setWordWrap(true);
    soundHint->setEnabled(false);

    notifLayout->addWidget(m_hideNotifCheck);
    notifLayout->addWidget(m_suppressFocusedCheck);
    notifLayout->addWidget(m_coalesceCheck);
    notifLayout->addLayout(cooldownRow);
    notifLayout->addSpacing(8);
    notifLayout->addWidget(kdeNotifBtn);
    notifLayout->addWidget(soundHint);
    notifLayout->addStretch();
    tabs->addTab(notifTab, QIcon::fromTheme(QStringLiteral("notifications")), i18n("Notifications"));

    // --- Privacy tab ---
    auto *privacyTab = new QWidget(tabs);
    auto *privacyLayout = new QVBoxLayout(privacyTab);

    auto *privacyHint = new QLabel(
        i18n("Hardware permissions are denied by default. Enable only what you need for voice notes or calls."),
        privacyTab);
    privacyHint->setWordWrap(true);
    privacyHint->setEnabled(false);

    m_allowMicCheck = new QCheckBox(i18n("Allow microphone access (voice notes & audio calls)"), privacyTab);
    m_allowCamCheck = new QCheckBox(i18n("Allow camera access (video calls)"), privacyTab);
    m_allowGeoCheck = new QCheckBox(i18n("Allow geolocation access"), privacyTab);
    m_allowScreenCheck = new QCheckBox(i18n("Allow screen capture"), privacyTab);

    auto *clearCacheBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear-history")),
                                          i18n("Clear Browser Cache (keeps login)"),
                                          privacyTab);
    connect(clearCacheBtn, &QPushButton::clicked, this, [this]() {
        Q_EMIT clearCacheRequested();
        QMessageBox::information(this, i18n("Cache Cleared"), i18n("HTTP cache has been cleared. Your WhatsApp session was kept."));
    });

    privacyLayout->addWidget(privacyHint);
    privacyLayout->addWidget(m_allowMicCheck);
    privacyLayout->addWidget(m_allowCamCheck);
    privacyLayout->addWidget(m_allowGeoCheck);
    privacyLayout->addWidget(m_allowScreenCheck);
    privacyLayout->addSpacing(12);
    privacyLayout->addWidget(clearCacheBtn);
    privacyLayout->addStretch();
    tabs->addTab(privacyTab, QIcon::fromTheme(QStringLiteral("security-high")), i18n("Privacy"));

    // --- Web tab ---
    auto *webTab = new QWidget(tabs);
    auto *webLayout = new QVBoxLayout(webTab);
    auto *webForm = new QFormLayout();

    m_spellCheckCheck = new QCheckBox(i18n("Enable British English (en_GB) spellcheck"), webTab);
    m_zoomCombo = new QComboBox(webTab);
    for (double factor : kZoomLevels) {
        const int pct = static_cast<int>(std::lround(factor * 100.0));
        m_zoomCombo->addItem(QStringLiteral("%1%").arg(pct), factor);
    }
    webForm->addRow(i18n("Default zoom level:"), m_zoomCombo);

    m_devModeCheck = new QCheckBox(i18n("Enable Developer Mode (Inspect Element & F12 DevTools)"), webTab);

    webLayout->addWidget(m_spellCheckCheck);
    webLayout->addLayout(webForm);
    webLayout->addSpacing(8);
    webLayout->addWidget(m_devModeCheck);
    webLayout->addStretch();
    tabs->addTab(webTab, QIcon::fromTheme(QStringLiteral("internet-web-browser")), i18n("Web"));

    // Dialog buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::applyAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    loadFromConfig();
}

void SettingsDialog::loadFromConfig()
{
    m_enableTrayCheck->setChecked(m_config.enableTrayIcon());
    m_closeToTrayCheck->setChecked(m_config.closeToTray());
    m_startMinimizedCheck->setChecked(m_config.startMinimizedToTray());
    m_autostartCheck->setChecked(m_config.autostart());

    m_hideNotifCheck->setChecked(m_config.hideNotifications());
    m_suppressFocusedCheck->setChecked(m_config.suppressNotificationsWhenFocused());
    m_coalesceCheck->setChecked(m_config.coalesceNotificationsPerChat());
    m_cooldownSpin->setValue(m_config.notificationCooldownSeconds());

    m_allowMicCheck->setChecked(m_config.allowMicrophone());
    m_allowCamCheck->setChecked(m_config.allowCamera());
    m_allowGeoCheck->setChecked(m_config.allowGeolocation());
    m_allowScreenCheck->setChecked(m_config.allowScreenCapture());

    m_spellCheckCheck->setChecked(m_config.spellCheckEnabled());
    m_devModeCheck->setChecked(m_config.developerMode());

    const double savedZoom = m_config.zoomFactor();
    int bestIndex = 2; // 100%
    double bestDiff = 100.0;
    for (int i = 0; i < m_zoomCombo->count(); ++i) {
        const double diff = std::abs(m_zoomCombo->itemData(i).toDouble() - savedZoom);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIndex = i;
        }
    }
    m_zoomCombo->setCurrentIndex(bestIndex);

    updateDependentControls();
}

void SettingsDialog::updateDependentControls()
{
    const bool trayOn = m_enableTrayCheck->isChecked();
    m_closeToTrayCheck->setEnabled(trayOn);
    m_startMinimizedCheck->setEnabled(trayOn);
}

void SettingsDialog::openKdeNotificationSettings()
{
    QProcess::startDetached(QStringLiteral("kcmshell6"), {QStringLiteral("kcm_notifications")});
}

void SettingsDialog::applyAndAccept()
{
    const bool trayOn = m_enableTrayCheck->isChecked();
    m_config.setEnableTrayIcon(trayOn);
    m_config.setCloseToTray(trayOn && m_closeToTrayCheck->isChecked());
    m_config.setStartMinimizedToTray(trayOn && m_startMinimizedCheck->isChecked());
    m_config.setAutostart(m_autostartCheck->isChecked());

    m_config.setHideNotifications(m_hideNotifCheck->isChecked());
    m_config.setSuppressNotificationsWhenFocused(m_suppressFocusedCheck->isChecked());
    m_config.setCoalesceNotificationsPerChat(m_coalesceCheck->isChecked());
    m_config.setNotificationCooldownSeconds(m_cooldownSpin->value());

    m_config.setAllowMicrophone(m_allowMicCheck->isChecked());
    m_config.setAllowCamera(m_allowCamCheck->isChecked());
    m_config.setAllowGeolocation(m_allowGeoCheck->isChecked());
    m_config.setAllowScreenCapture(m_allowScreenCheck->isChecked());

    m_config.setSpellCheckEnabled(m_spellCheckCheck->isChecked());
    m_config.setZoomFactor(m_zoomCombo->currentData().toDouble());
    m_config.setDeveloperMode(m_devModeCheck->isChecked());

    m_config.sync();
    Q_EMIT settingsApplied();
    accept();
}

} // namespace thatsapp
