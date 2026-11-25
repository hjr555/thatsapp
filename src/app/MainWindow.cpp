#include "app/MainWindow.h"
#include "app/BadgeController.h"
#include "app/NotificationController.h"
#include "app/SettingsDialog.h"
#include "app/TrayController.h"
#include "app/WhatsAppPage.h"
#include "app/WhatsAppView.h"
#include "core/ProtocolRouter.h"

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KWindowSystem>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QWebEngineDownloadRequest>
#include <QWebEngineNotification>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>

namespace thatsapp {

namespace {
constexpr auto kWhatsAppWebUrl = "https://web.whatsapp.com/";
}

MainWindow::MainWindow(AppConfig &config, QWidget *parent)
    : KMainWindow(parent)
    , m_config(config)
{
    // KMainWindow enables WA_DeleteOnClose by default; disable it because MainWindow is stack-owned in main()
    setAttribute(Qt::WA_DeleteOnClose, false);

    setWindowTitle(i18n("ThatsApp"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.hjr555.thatsapp"),
                                   QIcon(QStringLiteral(":/icons/io.github.hjr555.thatsapp.svg"))));
    resize(1120, 760);

    m_badgeController = new BadgeController(this);
    m_notificationController = new NotificationController(m_config, this);
    m_trayController = new TrayController(this);

    setupWebEngine();
    setupMenuBarAndShortcuts();

    connect(m_notificationController, &NotificationController::activateMainWindowRequested,
            this, &MainWindow::activateAndFocus);
    connect(m_notificationController, &NotificationController::notificationStateChanged,
            this, [this]() {
                updateNotificationMenuState();
                updateWindowTitle();
            });

    connect(m_badgeController, &BadgeController::unreadCountChanged, this, [this](int count) {
        m_trayController->setUnreadCount(count);
        updateWindowTitle();
    });

    connect(m_trayController, &TrayController::toggleWindowRequested, this, [this](const QString &token) {
        if (isVisible() && !isMinimized() && isActiveWindow()) {
            hide();
        } else {
            activateAndFocus(token);
        }
    });
    connect(m_trayController, &TrayController::toggleHideNotificationsRequested,
            m_notificationController, &NotificationController::setHideNotifications);
    connect(m_trayController, &TrayController::snoozeRequested, this, [this](int minutes) {
        m_notificationController->snoozeFor(std::chrono::minutes(minutes));
    });
    connect(m_trayController, &TrayController::clearSnoozeRequested,
            m_notificationController, &NotificationController::clearSnooze);
    connect(m_trayController, &TrayController::settingsRequested, this, &MainWindow::openSettingsDialog);
    connect(m_trayController, &TrayController::quitRequested, this, &MainWindow::requestQuit);

    m_trayController->setEnabled(m_config.enableTrayIcon());
    m_sidebarCollapsed = m_config.sidebarCollapsed();
    if (m_collapseSidebarAction) {
        const QSignalBlocker blocker(m_collapseSidebarAction);
        m_collapseSidebarAction->setChecked(m_sidebarCollapsed);
    }
    m_view->setSidebarCollapsedState(m_sidebarCollapsed);
    updateNotificationMenuState();
    updateWindowTitle();

    // Auto-save & restore window size/geometry via KConfig (~/.config/thatsapprc)
    setAutoSaveSettings(QStringLiteral("MainWindow"), true);

    qApp->installEventFilter(this);

    m_view->setUrl(QUrl(QString::fromLatin1(kWhatsAppWebUrl)));
}

MainWindow::~MainWindow()
{
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    if (m_devToolsView) {
        if (m_page) {
            m_page->setDevToolsPage(nullptr);
        }
        delete m_devToolsView;
        m_devToolsView = nullptr;
    }
    if (m_view) {
        m_view->setPage(nullptr);
        delete m_view;
        m_view = nullptr;
    }
    if (m_page) {
        delete m_page;
        m_page = nullptr;
    }
    if (m_profile) {
        delete m_profile;
        m_profile = nullptr;
    }
}

QString MainWindow::buildCleanUserAgent(const QString &defaultUserAgent)
{
    // Keep the exact shipped Chromium version and Linux platform, removing only the QtWebEngine token
    static const QRegularExpression qtWebEngineToken(QStringLiteral(R"(\s*QtWebEngine/\S+)"));
    QString ua = defaultUserAgent;
    ua.remove(qtWebEngineToken);
    return ua.simplified();
}

void MainWindow::setupWebEngine()
{
    m_profile = new QWebEngineProfile(QStringLiteral("thatsapp"), this);

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/webengine");
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/webengine");
    QDir().mkpath(dataDir);
    QDir().mkpath(cacheDir);

    m_profile->setPersistentStoragePath(dataDir);
    m_profile->setCachePath(cacheDir);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    m_profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    m_profile->setHttpUserAgent(buildCleanUserAgent(m_profile->httpUserAgent()));

    m_profile->setNotificationPresenter([this](std::unique_ptr<QWebEngineNotification> notification) {
        m_notificationController->presentWebNotification(std::move(notification));
    });

    connect(m_profile, &QWebEngineProfile::downloadRequested, this, &MainWindow::onDownloadRequested);

    applyWebProfileSettings();
    installSidebarCollapseScript();

    m_page = new WhatsAppPage(m_profile, m_config, this);
    m_view = new WhatsAppView(m_config, this);
    m_view->setPage(m_page);
    m_view->applySavedZoom();

    connect(m_page, &WhatsAppPage::internalWhatsAppUrlRequested, m_view, &QWebEngineView::setUrl);
    connect(m_page, &WhatsAppPage::sidebarCollapsedChangedFromPage, this, [this](bool collapsed) {
        setSidebarCollapsed(collapsed, false);
    });
    connect(m_view, &QWebEngineView::titleChanged, m_badgeController, &BadgeController::updateFromDocumentTitle);
    connect(m_view, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok && m_sidebarCollapsed) {
            setSidebarCollapsed(true, true);
        }
    });
    connect(m_view, &WhatsAppView::inspectElementRequested, this, &MainWindow::toggleDevTools);
    connect(m_view, &WhatsAppView::toggleMenuBarRequested, this, &MainWindow::toggleMenuBar);
    connect(m_view, &WhatsAppView::toggleSidebarRequested, this, &MainWindow::toggleSidebar);
    connect(m_view, &WhatsAppView::openSettingsRequested, this, &MainWindow::openSettingsDialog);
    connect(m_view, &WhatsAppView::toggleHideNotificationsRequested, this, [this]() {
        m_notificationController->setHideNotifications(!m_notificationController->isHidden());
    });

    setCentralWidget(m_view);
}

void MainWindow::installSidebarCollapseScript()
{
    static const QString kSidebarScript = QStringLiteral(R"JS(
(() => {
    if (window.__thatsappSidebarInstalled) return;
    window.__thatsappSidebarInstalled = true;

    const STYLE_ID = 'thatsapp-sidebar-style';
    const BTN_ID = 'thatsapp-sidebar-toggle-btn';

    function ensureStyle() {
        if (document.getElementById(STYLE_ID)) return;
        const style = document.createElement('style');
        style.id = STYLE_ID;
        style.textContent = `
            html.thatsapp-sidebar-collapsed [data-thatsapp-sidebar="true"] {
                display: none !important;
                width: 0 !important;
                min-width: 0 !important;
                max-width: 0 !important;
                flex: 0 0 0 !important;
            }
            html.thatsapp-sidebar-collapsed [data-thatsapp-split-container="true"] {
                display: flex !important;
                grid-template-columns: 1fr !important;
            }
            html.thatsapp-sidebar-collapsed [data-thatsapp-main-pane="true"] {
                flex: 1 1 0% !important;
                width: auto !important;
                max-width: 100% !important;
                min-width: 0 !important;
                grid-column: 1 / -1 !important;
                left: 0 !important;
                border-left: none !important;
            }
            html.thatsapp-sidebar-collapsed [data-thatsapp-divider-overlay="true"] {
                border-left-color: transparent !important;
                border-right-color: transparent !important;
            }
            html.thatsapp-sidebar-collapsed [data-thatsapp-shell="true"] {
                max-width: 100% !important;
                width: 100% !important;
                height: 100% !important;
                top: 0 !important;
                margin: 0 !important;
            }
            #${BTN_ID} {
                display: none;
                position: fixed;
                left: 0;
                top: 50%;
                transform: translateY(-50%);
                z-index: 2147483647;
                width: 18px;
                height: 52px;
                border: 1px solid rgba(134, 150, 160, 0.35);
                border-left: none;
                border-radius: 0 8px 8px 0;
                background: rgba(32, 44, 51, 0.65);
                color: #e9edef;
                font-size: 15px;
                font-weight: 600;
                line-height: 1;
                cursor: pointer;
                align-items: center;
                justify-content: center;
                padding: 0;
                opacity: 0.42;
                transition: opacity 0.15s ease, width 0.15s ease, background 0.15s ease;
                backdrop-filter: blur(6px);
                user-select: none;
            }
            #${BTN_ID}:hover {
                opacity: 0.98;
                width: 24px;
                background: rgba(32, 44, 51, 0.92);
            }
            #${BTN_ID}[data-ready="true"] {
                display: flex;
            }
        `;
        (document.head || document.documentElement).appendChild(style);
    }

    function ensureToggleButton() {
        let btn = document.getElementById(BTN_ID);
        if (btn) return btn;
        if (!document.body) return null;
        btn = document.createElement('button');
        btn.id = BTN_ID;
        btn.type = 'button';
        btn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const isCollapsed = document.documentElement.classList.contains('thatsapp-sidebar-collapsed');
            window.__thatsappSetSidebarCollapsed(!isCollapsed, true);
        });
        document.body.appendChild(btn);
        return btn;
    }

    function clearTags() {
        document.querySelectorAll('[data-thatsapp-sidebar]').forEach(el => el.removeAttribute('data-thatsapp-sidebar'));
        document.querySelectorAll('[data-thatsapp-main-pane]').forEach(el => el.removeAttribute('data-thatsapp-main-pane'));
        document.querySelectorAll('[data-thatsapp-split-container]').forEach(el => el.removeAttribute('data-thatsapp-split-container'));
        document.querySelectorAll('[data-thatsapp-shell]').forEach(el => el.removeAttribute('data-thatsapp-shell'));
        document.querySelectorAll('[data-thatsapp-divider-overlay]').forEach(el => el.removeAttribute('data-thatsapp-divider-overlay'));
    }

    function findSidebarElement() {
        return document.getElementById('side')
            || document.getElementById('pane-side')
            || document.querySelector('[aria-label="Chat list"]');
    }

    function isFlowChild(el) {
        const pos = window.getComputedStyle(el).position;
        return pos !== 'absolute' && pos !== 'fixed';
    }

    function updateButtonVisuals(mainCol) {
        const btn = ensureToggleButton();
        if (!btn) return;
        const isCollapsed = document.documentElement.classList.contains('thatsapp-sidebar-collapsed');
        if (!mainCol || !mainCol.isConnected) {
            btn.removeAttribute('data-ready');
            return;
        }
        btn.setAttribute('data-ready', 'true');
        if (isCollapsed) {
            btn.style.left = '0px';
            btn.textContent = '\u203A';
            btn.title = 'Show chat sidebar (F9 / Ctrl+B)';
            btn.setAttribute('aria-label', 'Show chat sidebar');
        } else {
            const rect = mainCol.getBoundingClientRect();
            const leftPx = Math.max(0, Math.round(rect.left));
            btn.style.left = `${leftPx}px`;
            btn.textContent = '\u2039';
            btn.title = 'Collapse chat sidebar (F9 / Ctrl+B)';
            btn.setAttribute('aria-label', 'Collapse chat sidebar');
        }
    }

    function findSplitAndColumns(side) {
        const main = document.getElementById('main');
        if (main) {
            const sideAncestors = new Set();
            for (let el = side; el; el = el.parentElement) {
                sideAncestors.add(el);
            }
            let lca = main.parentElement;
            while (lca && !sideAncestors.has(lca)) {
                lca = lca.parentElement;
            }
            if (lca) {
                const mainCol = Array.from(lca.children).find(ch => ch === main || ch.contains(main));
                const sideCol = Array.from(lca.children).find(ch => ch === side || ch.contains(side));
                if (mainCol && sideCol && mainCol !== sideCol) {
                    return { lca, sideCol, mainCol };
                }
            }
        }

        // Fallback when no chat is selected yet: locate the split container directly from #side
        for (let col = side; col && col.parentElement && col.parentElement.id !== 'app'; col = col.parentElement) {
            const parent = col.parentElement;
            if (!isFlowChild(col)) continue;
            const colRect = col.getBoundingClientRect();
            if (colRect.width < 120) continue;

            const rightPane = Array.from(parent.children).find(ch => {
                if (ch === col || !isFlowChild(ch)) return false;
                const r = ch.getBoundingClientRect();
                return r.width >= 200 && r.height >= 200 && r.left >= colRect.right - 12;
            });
            if (rightPane) {
                return { lca: parent, sideCol: col, mainCol: rightPane };
            }
        }
        return null;
    }

    function tagLayout(forceRetag) {
        const side = findSidebarElement();
        if (!side) {
            updateButtonVisuals(null);
            return;
        }

        const existingMain = document.querySelector('[data-thatsapp-main-pane="true"]');
        const existingSide = document.querySelector('[data-thatsapp-sidebar="true"]');
        if (!forceRetag && existingMain && existingSide && existingMain.isConnected && existingSide.isConnected) {
            updateButtonVisuals(existingMain);
            return;
        }

        const wasCollapsed = document.documentElement.classList.contains('thatsapp-sidebar-collapsed');
        if (wasCollapsed) {
            document.documentElement.classList.remove('thatsapp-sidebar-collapsed');
        }

        const parts = findSplitAndColumns(side);
        if (!parts) {
            if (wasCollapsed) {
                document.documentElement.classList.add('thatsapp-sidebar-collapsed');
            }
            updateButtonVisuals(null);
            return;
        }

        const { lca, sideCol, mainCol } = parts;
        clearTags();
        lca.setAttribute('data-thatsapp-split-container', 'true');
        mainCol.setAttribute('data-thatsapp-main-pane', 'true');
        sideCol.setAttribute('data-thatsapp-sidebar', 'true');

        // Hide any left-side flow sibling navigation rail (e.g. the 64px icon <header>)
        // and suppress phantom column-divider borders on pointer-events:none overlay layers
        const mainLeft = mainCol.getBoundingClientRect().left;
        for (const ch of Array.from(lca.children)) {
            if (ch === mainCol || ch === sideCol) continue;
            const chCs = window.getComputedStyle(ch);
            if (!isFlowChild(ch)) {
                if (chCs.pointerEvents === 'none') {
                    for (const d of ch.querySelectorAll('div')) {
                        const dCs = window.getComputedStyle(d);
                        const hasSideBorder = (parseFloat(dCs.borderLeftWidth) > 0 && dCs.borderLeftStyle !== 'none')
                            || (parseFloat(dCs.borderRightWidth) > 0 && dCs.borderRightStyle !== 'none');
                        if (hasSideBorder && dCs.pointerEvents === 'none') {
                            d.setAttribute('data-thatsapp-divider-overlay', 'true');
                        }
                    }
                }
                continue;
            }
            const rect = ch.getBoundingClientRect();
            if (rect.width > 0 && rect.right <= mainLeft + 12) {
                ch.setAttribute('data-thatsapp-sidebar', 'true');
            }
        }

        let cur = lca;
        for (let depth = 0; depth < 3 && cur && cur.parentElement && cur.id !== 'app'; depth++) {
            const parent = cur.parentElement;
            const curLeft = cur.getBoundingClientRect().left;
            for (const sib of Array.from(parent.children)) {
                if (sib === cur || !isFlowChild(sib)) continue;
                const rect = sib.getBoundingClientRect();
                if (rect.width > 0 && rect.width <= 140 && rect.right <= curLeft + 12) {
                    sib.setAttribute('data-thatsapp-sidebar', 'true');
                }
            }
            const cs = window.getComputedStyle(cur);
            if (cs.maxWidth && cs.maxWidth !== 'none') {
                cur.setAttribute('data-thatsapp-shell', 'true');
            }
            cur = parent;
        }

        if (wasCollapsed) {
            document.documentElement.classList.add('thatsapp-sidebar-collapsed');
        }
        updateButtonVisuals(mainCol);
    }

    let scheduled = false;
    function scheduleLayoutUpdate() {
        if (scheduled) return;
        scheduled = true;
        requestAnimationFrame(() => {
            scheduled = false;
            ensureStyle();
            ensureToggleButton();
            tagLayout(false);
        });
    }

    window.__thatsappSetSidebarCollapsed = function(collapsed, notifyHost) {
        ensureStyle();
        ensureToggleButton();
        const isCollapsed = Boolean(collapsed);
        if (isCollapsed) {
            tagLayout(true);
            document.documentElement.classList.add('thatsapp-sidebar-collapsed');
        } else {
            document.documentElement.classList.remove('thatsapp-sidebar-collapsed');
            tagLayout(false);
        }
        const mainCol = document.querySelector('[data-thatsapp-main-pane="true"]');
        updateButtonVisuals(mainCol);
        if (notifyHost) {
            console.debug(isCollapsed ? '__thatsapp:sidebar=1' : '__thatsapp:sidebar=0');
        }
    };

    const observer = new MutationObserver(() => {
        scheduleLayoutUpdate();
    });

    function startObserver() {
        ensureStyle();
        ensureToggleButton();
        const target = document.getElementById('app') || document.body || document.documentElement;
        if (target) {
            observer.observe(target, { childList: true, subtree: true });
        }
        window.addEventListener('resize', scheduleLayoutUpdate, { passive: true });
        scheduleLayoutUpdate();
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', startObserver, { once: true });
    } else {
        startObserver();
    }
})();
)JS");

    QWebEngineScript script;
    script.setName(QStringLiteral("thatsapp-sidebar-collapse"));
    script.setSourceCode(kSidebarScript);
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(false);
    m_profile->scripts()->insert(script);
}

void MainWindow::applyWebProfileSettings()
{
    m_profile->setSpellCheckEnabled(m_config.spellCheckEnabled());
    m_profile->setSpellCheckLanguages({m_config.spellCheckLanguage()});

    auto *settings = m_profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, m_config.allowScreenCapture());
}

void MainWindow::setupMenuBarAndShortcuts()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(i18n("&File"));

    auto *reloadAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("&Reload"));
    reloadAction->setShortcuts({QKeySequence::Refresh, QKeySequence(Qt::Key_F5)});
    connect(reloadAction, &QAction::triggered, m_view, &QWebEngineView::reload);
    addAction(reloadAction);

    auto *clearCacheAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-clear-history")),
                                                 i18n("Clear &Cache"));
    connect(clearCacheAction, &QAction::triggered, this, [this]() {
        m_profile->clearHttpCache();
    });

    fileMenu->addSeparator();

    auto *closeWinAction = KStandardAction::close(this, &QWidget::close, this);
    fileMenu->addAction(closeWinAction);
    addAction(closeWinAction);

    auto *quitAction = KStandardAction::quit(this, &MainWindow::requestQuit, this);
    fileMenu->addAction(quitAction);
    addAction(quitAction);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(i18n("&View"));

    auto *zoomInAction = KStandardAction::zoomIn(m_view, &WhatsAppView::zoomInStep, this);
    viewMenu->addAction(zoomInAction);
    addAction(zoomInAction);

    auto *zoomOutAction = KStandardAction::zoomOut(m_view, &WhatsAppView::zoomOutStep, this);
    viewMenu->addAction(zoomOutAction);
    addAction(zoomOutAction);

    auto *zoomResetAction = KStandardAction::actualSize(m_view, &WhatsAppView::resetZoomStep, this);
    zoomResetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    viewMenu->addAction(zoomResetAction);
    addAction(zoomResetAction);

    viewMenu->addSeparator();

    m_collapseSidebarAction = viewMenu->addAction(
        QIcon::fromTheme(QStringLiteral("sidebar-collapse-left")),
        i18n("Collapse Chat &Sidebar"));
    m_collapseSidebarAction->setCheckable(true);
    m_collapseSidebarAction->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_B), QKeySequence(Qt::Key_F9)});
    connect(m_collapseSidebarAction, &QAction::toggled, this, [this](bool collapsed) {
        setSidebarCollapsed(collapsed, true);
    });
    addAction(m_collapseSidebarAction);

    auto *fullScreenAction = KStandardAction::fullScreen(this, [this](bool on) {
        if (on) {
            showFullScreen();
        } else {
            showNormal();
        }
    }, this, this);
    viewMenu->addAction(fullScreenAction);
    addAction(fullScreenAction);

    m_devToolsAction = viewMenu->addAction(QIcon::fromTheme(QStringLiteral("tools-report-bug")),
                                           i18n("Developer &Tools"));
    m_devToolsAction->setShortcut(QKeySequence(Qt::Key_F12));
    m_devToolsAction->setVisible(m_config.developerMode());
    connect(m_devToolsAction, &QAction::triggered, this, &MainWindow::toggleDevTools);
    addAction(m_devToolsAction);

    // Notifications menu
    QMenu *notifMenu = menuBar()->addMenu(i18n("&Notifications"));

    m_hideNotifAction = notifMenu->addAction(QIcon::fromTheme(QStringLiteral("notifications-disabled")),
                                             i18n("&Hide Notifications"));
    m_hideNotifAction->setCheckable(true);
    m_hideNotifAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(m_hideNotifAction, &QAction::toggled,
            m_notificationController, &NotificationController::setHideNotifications);
    addAction(m_hideNotifAction);

    QMenu *snoozeMenu = notifMenu->addMenu(QIcon::fromTheme(QStringLiteral("chronometer")), i18n("&Snooze"));
    connect(snoozeMenu->addAction(i18n("15 minutes")), &QAction::triggered, this, [this]() {
        m_notificationController->snoozeFor(std::chrono::minutes(15));
    });
    connect(snoozeMenu->addAction(i18n("1 hour")), &QAction::triggered, this, [this]() {
        m_notificationController->snoozeFor(std::chrono::hours(1));
    });
    connect(snoozeMenu->addAction(i18n("8 hours")), &QAction::triggered, this, [this]() {
        m_notificationController->snoozeFor(std::chrono::hours(8));
    });

    m_clearSnoozeAction = notifMenu->addAction(QIcon::fromTheme(QStringLiteral("notifications")),
                                               i18n("&Resume Notifications"));
    connect(m_clearSnoozeAction, &QAction::triggered,
            m_notificationController, &NotificationController::clearSnooze);

    notifMenu->addSeparator();
    auto *kdeNotifAction = notifMenu->addAction(QIcon::fromTheme(QStringLiteral("preferences-desktop-notification-bell")),
                                                i18n("Configure in KDE System Settings..."));
    connect(kdeNotifAction, &QAction::triggered, this, []() {
        QProcess::startDetached(QStringLiteral("kcmshell6"), {QStringLiteral("kcm_notifications")});
    });

    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(i18n("&Settings"));

    m_showMenuBarAction = KStandardAction::showMenubar(this, [this](bool visible) {
        setMenuBarVisibleWithHint(visible, true);
    }, this);
    settingsMenu->addAction(m_showMenuBarAction);
    addAction(m_showMenuBarAction);

    auto *prefsAction = KStandardAction::preferences(this, &MainWindow::openSettingsDialog, this);
    settingsMenu->addAction(prefsAction);
    addAction(prefsAction);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(i18n("&Help"));
    auto *aboutAction = KStandardAction::aboutApp(this, [this]() {
        auto *dlg = new KAboutApplicationDialog(KAboutData::applicationData(), this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }, this);
    helpMenu->addAction(aboutAction);

    // Menu bar hidden by default (KDE standard Ctrl+M or right-click toggles visibility)
    setMenuBarVisibleWithHint(m_config.showMenuBar(), false);
}

void MainWindow::toggleMenuBar()
{
    setMenuBarVisibleWithHint(!menuBar()->isVisible(), true);
}

void MainWindow::setMenuBarVisibleWithHint(bool visible, bool showHideHint)
{
    menuBar()->setVisible(visible);
    if (m_showMenuBarAction) {
        const QSignalBlocker blocker(m_showMenuBarAction);
        m_showMenuBarAction->setChecked(visible);
    }
    m_view->setMenuBarVisibleState(visible);
    m_config.setShowMenuBar(visible);
    m_config.sync();

    if (!visible && showHideHint) {
        KMessageBox::information(
            this,
            i18n("The menu bar is now hidden. You can bring it back at any time by pressing Ctrl+M or right-clicking anywhere in the window."),
            i18n("Menu Bar Hidden"),
            QStringLiteral("HideMenuBarWarning"));
    }
}

void MainWindow::toggleSidebar()
{
    setSidebarCollapsed(!m_sidebarCollapsed, true);
}

void MainWindow::setSidebarCollapsed(bool collapsed, bool pushToWeb)
{
    m_sidebarCollapsed = collapsed;
    if (m_collapseSidebarAction) {
        const QSignalBlocker blocker(m_collapseSidebarAction);
        m_collapseSidebarAction->setChecked(collapsed);
    }
    m_view->setSidebarCollapsedState(collapsed);
    m_config.setSidebarCollapsed(collapsed);
    m_config.sync();

    if (pushToWeb && m_page) {
        const QString js = collapsed
            ? QStringLiteral("if (window.__thatsappSetSidebarCollapsed) window.__thatsappSetSidebarCollapsed(true, false);")
            : QStringLiteral("if (window.__thatsappSetSidebarCollapsed) window.__thatsappSetSidebarCollapsed(false, false);");
        m_page->runJavaScript(js);
    }
}

void MainWindow::activateAndFocus(const QString &xdgActivationToken)
{
    if (!xdgActivationToken.isEmpty()) {
        KWindowSystem::setCurrentXdgActivationToken(xdgActivationToken);
    }
    if (isMinimized()) {
        setWindowState(windowState() & ~Qt::WindowMinimized);
    }
    show();
    raise();
    activateWindow();
    if (windowHandle()) {
        KWindowSystem::activateWindow(windowHandle());
    }
}

void MainWindow::handleExternalUri(const QString &rawUri)
{
    const QUrl uri(rawUri.trimmed());
    if (!ProtocolRouter::isWhatsAppProtocol(uri)) {
        return;
    }
    const QUrl webUrl = ProtocolRouter::toWebWhatsAppUrl(uri);
    if (webUrl.isValid()) {
        m_view->setUrl(webUrl);
    }
}

void MainWindow::requestQuit()
{
    m_quitting = true;
    close();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_quitting && m_config.enableTrayIcon() && m_config.closeToTray()) {
        hide();
        event->ignore();
        return;
    }
    KMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    KMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowStateChange) {
        m_notificationController->setWindowFocused(isActiveWindow() && !isMinimized() && isVisible());
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
        && isActiveWindow()
        && QApplication::activeModalWidget() == nullptr) {
        auto *ke = static_cast<QKeyEvent *>(event);
        const Qt::KeyboardModifiers mods = ke->modifiers()
            & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);

        const bool isMenuBar = (mods == Qt::ControlModifier && ke->key() == Qt::Key_M);
        const bool isSidebar = (mods == Qt::ControlModifier && ke->key() == Qt::Key_B)
            || (mods == Qt::NoModifier && ke->key() == Qt::Key_F9);
        const bool isSettings = (mods == Qt::ControlModifier && ke->key() == Qt::Key_Comma);
        const bool isMuteNotif = (mods == (Qt::ControlModifier | Qt::ShiftModifier) && ke->key() == Qt::Key_M);

        if (isMenuBar || isSidebar || isSettings || isMuteNotif) {
            if (event->type() == QEvent::KeyPress && !ke->isAutoRepeat()) {
                if (isMenuBar) {
                    toggleMenuBar();
                } else if (isSidebar) {
                    toggleSidebar();
                } else if (isSettings) {
                    openSettingsDialog();
                } else if (isMuteNotif) {
                    m_notificationController->setHideNotifications(!m_notificationController->isHidden());
                }
            }
            ke->accept();
            return true;
        }
    }
    return KMainWindow::eventFilter(watched, event);
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dlg(m_config, this);
    connect(&dlg, &SettingsDialog::settingsApplied, this, &MainWindow::onSettingsApplied);
    connect(&dlg, &SettingsDialog::clearCacheRequested, this, [this]() {
        m_profile->clearHttpCache();
    });
    dlg.exec();
}

void MainWindow::onSettingsApplied()
{
    applyWebProfileSettings();
    m_view->applySavedZoom();
    m_trayController->setEnabled(m_config.enableTrayIcon());
    m_notificationController->reloadConfig();
    if (m_devToolsAction) {
        m_devToolsAction->setVisible(m_config.developerMode());
    }
    if (!m_config.developerMode() && m_devToolsView) {
        m_devToolsView->close();
    }
    updateNotificationMenuState();
    updateWindowTitle();
}

void MainWindow::onDownloadRequested(QWebEngineDownloadRequest *download)
{
    if (!download) {
        return;
    }

    const QString initialDir = m_config.lastDownloadDir();
    const QString suggestedPath = QDir(initialDir).filePath(download->suggestedFileName());

    const QString chosenPath = QFileDialog::getSaveFileName(
        this,
        i18n("Save File"),
        suggestedPath);

    if (chosenPath.isEmpty()) {
        download->cancel();
        return;
    }

    const QFileInfo info(chosenPath);
    m_config.setLastDownloadDir(info.absolutePath());
    m_config.sync();

    download->setDownloadDirectory(info.absolutePath());
    download->setDownloadFileName(info.fileName());

    connect(download, &QWebEngineDownloadRequest::stateChanged, this,
            [this, chosenPath](QWebEngineDownloadRequest::DownloadState state) {
                if (state == QWebEngineDownloadRequest::DownloadCompleted) {
                    m_notificationController->showDownloadComplete(chosenPath);
                }
            });

    download->accept();
}

void MainWindow::toggleDevTools()
{
    if (!m_config.developerMode()) {
        return;
    }
    if (m_devToolsView) {
        m_devToolsView->close();
        return;
    }
    m_devToolsView = new QWebEngineView();
    m_devToolsView->setAttribute(Qt::WA_DeleteOnClose);
    m_devToolsView->setWindowTitle(i18n("ThatsApp Developer Tools"));
    m_devToolsView->resize(1000, 700);
    m_page->setDevToolsPage(m_devToolsView->page());
    m_devToolsView->show();
}

void MainWindow::updateNotificationMenuState()
{
    const bool hidden = m_notificationController->isHidden();
    const bool snoozed = m_notificationController->isSnoozed();

    if (m_hideNotifAction) {
        const QSignalBlocker blocker(m_hideNotifAction);
        m_hideNotifAction->setChecked(hidden);
    }
    if (m_clearSnoozeAction) {
        m_clearSnoozeAction->setVisible(snoozed);
    }
    m_trayController->setNotificationMutedOrSnoozed(hidden, snoozed);
}

void MainWindow::updateWindowTitle()
{
    const int unread = m_badgeController->unreadCount();
    QString title = unread > 0
        ? i18n("(%1) ThatsApp", unread)
        : i18n("ThatsApp");

    if (m_notificationController->isHidden()) {
        title += i18n(" — Notifications Hidden");
    } else if (m_notificationController->isSnoozed()) {
        title += i18n(" — Snoozed");
    }
    setWindowTitle(title);
}

} // namespace thatsapp
