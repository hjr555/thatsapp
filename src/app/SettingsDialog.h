#pragma once

#include "core/AppConfig.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;

namespace hatsapp {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(AppConfig &config, QWidget *parent = nullptr);

Q_SIGNALS:
    void settingsApplied();
    void clearCacheRequested();

private Q_SLOTS:
    void applyAndAccept();
    void updateDependentControls();
    void openKdeNotificationSettings();

private:
    void loadFromConfig();

    AppConfig &m_config;

    // General
    QCheckBox *m_enableTrayCheck = nullptr;
    QCheckBox *m_closeToTrayCheck = nullptr;
    QCheckBox *m_startMinimizedCheck = nullptr;
    QCheckBox *m_autostartCheck = nullptr;

    // Notifications
    QCheckBox *m_hideNotifCheck = nullptr;
    QCheckBox *m_suppressFocusedCheck = nullptr;
    QCheckBox *m_coalesceCheck = nullptr;
    QSpinBox *m_cooldownSpin = nullptr;

    // Privacy
    QCheckBox *m_allowMicCheck = nullptr;
    QCheckBox *m_allowCamCheck = nullptr;
    QCheckBox *m_allowGeoCheck = nullptr;
    QCheckBox *m_allowScreenCheck = nullptr;

    // Web
    QCheckBox *m_spellCheckCheck = nullptr;
    QComboBox *m_zoomCombo = nullptr;
    QCheckBox *m_devModeCheck = nullptr;
};

} // namespace hatsapp
