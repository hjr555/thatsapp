#include "app/WhatsAppView.h"
#include "core/ZoomLevels.h"

#include <KLocalizedString>
#include <QChildEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QWebEngineContextMenuRequest>
#include <QWebEnginePage>
#include <QWheelEvent>

namespace hatsapp {

WhatsAppView::WhatsAppView(AppConfig &config, QWidget *parent)
    : QWebEngineView(parent)
    , m_config(config)
{
    attachFocusProxyFilter();
}

void WhatsAppView::attachFocusProxyFilter()
{
    QWidget *proxy = focusProxy();
    if (proxy && proxy != m_installedFocusProxy) {
        if (m_installedFocusProxy) {
            m_installedFocusProxy->removeEventFilter(this);
        }
        proxy->installEventFilter(this);
        m_installedFocusProxy = proxy;
    }
}

bool WhatsAppView::event(QEvent *event)
{
    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildPolished) {
        attachFocusProxyFilter();
    }
    return QWebEngineView::event(event);
}

bool WhatsAppView::handleAppShortcutKey(int key, Qt::KeyboardModifiers mods)
{
    const Qt::KeyboardModifiers cleanMods = mods & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);

    if (cleanMods == Qt::ControlModifier && key == Qt::Key_M) {
        Q_EMIT toggleMenuBarRequested();
        return true;
    }
    if ((cleanMods == Qt::ControlModifier && key == Qt::Key_B)
        || (cleanMods == Qt::NoModifier && key == Qt::Key_F9)) {
        Q_EMIT toggleSidebarRequested();
        return true;
    }
    if (cleanMods == Qt::ControlModifier && key == Qt::Key_Comma) {
        Q_EMIT openSettingsRequested();
        return true;
    }
    if (cleanMods == (Qt::ControlModifier | Qt::ShiftModifier) && key == Qt::Key_M) {
        Q_EMIT toggleHideNotificationsRequested();
        return true;
    }
    return false;
}

bool WhatsAppView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_installedFocusProxy) {
        if (event->type() == QEvent::ShortcutOverride) {
            auto *ke = static_cast<QKeyEvent *>(event);
            const Qt::KeyboardModifiers mods = ke->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
            const bool isReserved =
                (mods == Qt::ControlModifier && (ke->key() == Qt::Key_M || ke->key() == Qt::Key_B || ke->key() == Qt::Key_Comma))
                || (mods == Qt::NoModifier && ke->key() == Qt::Key_F9)
                || (mods == (Qt::ControlModifier | Qt::ShiftModifier) && ke->key() == Qt::Key_M);
            if (isReserved) {
                ke->accept();
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);
            if (handleAppShortcutKey(ke->key(), ke->modifiers())) {
                ke->accept();
                return true;
            }
        }
    }
    return QWebEngineView::eventFilter(watched, event);
}

void WhatsAppView::setMenuBarVisibleState(bool visible)
{
    m_menuBarVisible = visible;
}

void WhatsAppView::setSidebarCollapsedState(bool collapsed)
{
    m_sidebarCollapsed = collapsed;
}

void WhatsAppView::zoomInStep()
{
    const double next = nextZoomIn(zoomFactor());
    setZoomFactor(next);
    m_config.setZoomFactor(next);
    m_config.sync();
    Q_EMIT zoomFactorChanged(next);
}

void WhatsAppView::zoomOutStep()
{
    const double next = nextZoomOut(zoomFactor());
    setZoomFactor(next);
    m_config.setZoomFactor(next);
    m_config.sync();
    Q_EMIT zoomFactorChanged(next);
}

void WhatsAppView::resetZoomStep()
{
    setZoomFactor(kDefaultZoom);
    m_config.setZoomFactor(kDefaultZoom);
    m_config.sync();
    Q_EMIT zoomFactorChanged(kDefaultZoom);
}

void WhatsAppView::applySavedZoom()
{
    setZoomFactor(m_config.zoomFactor());
}

void WhatsAppView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebEngineContextMenuRequest *req = lastContextMenuRequest();
    if (!req) {
        event->ignore();
        return;
    }

    QMenu menu(this);
    bool hasAny = false;

    // Spellcheck replacement suggestions (en_GB)
    if (!req->misspelledWord().isEmpty()) {
        const QStringList suggestions = req->spellCheckerSuggestions();
        if (suggestions.isEmpty()) {
            auto *none = menu.addAction(i18n("No spelling suggestions"));
            none->setEnabled(false);
        } else {
            const int limit = qMin(suggestions.size(), 6);
            for (int i = 0; i < limit; ++i) {
                const QString word = suggestions.at(i);
                auto *act = menu.addAction(word);
                connect(act, &QAction::triggered, this, [this, word]() {
                    page()->replaceMisspelledWord(word);
                });
            }
        }
        menu.addSeparator();
        hasAny = true;
    }

    // Editable text actions
    if (req->isContentEditable()) {
        if (auto *a = pageAction(QWebEnginePage::Undo); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::Redo); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (hasAny) {
            menu.addSeparator();
        }
        if (auto *a = pageAction(QWebEnginePage::Cut); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::Copy); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::Paste); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::PasteAndMatchStyle); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::SelectAll); a && a->isEnabled()) {
            menu.addSeparator();
            menu.addAction(a);
            hasAny = true;
        }
    } else if (!req->selectedText().isEmpty()) {
        if (auto *a = pageAction(QWebEnginePage::Copy); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
    }

    // Link actions
    if (req->linkUrl().isValid()) {
        if (hasAny) {
            menu.addSeparator();
        }
        if (auto *a = pageAction(QWebEnginePage::CopyLinkToClipboard); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
    }

    // Image / media actions
    if (req->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage) {
        if (hasAny) {
            menu.addSeparator();
        }
        if (auto *a = pageAction(QWebEnginePage::CopyImageToClipboard); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
        if (auto *a = pageAction(QWebEnginePage::DownloadImageToDisk); a && a->isEnabled()) {
            menu.addAction(a);
            hasAny = true;
        }
    }

    // HatsApp window/layout controls (always reachable even when the menu bar is hidden)
    if (hasAny) {
        menu.addSeparator();
    }

    auto *sidebarAct = menu.addAction(
        QIcon::fromTheme(m_sidebarCollapsed ? QStringLiteral("sidebar-expand-left") : QStringLiteral("sidebar-collapse-left")),
        m_sidebarCollapsed ? i18n("Show Chat Sidebar (Ctrl+B)") : i18n("Collapse Chat Sidebar (Ctrl+B)"));
    connect(sidebarAct, &QAction::triggered, this, &WhatsAppView::toggleSidebarRequested);

    auto *menuBarAct = menu.addAction(
        QIcon::fromTheme(QStringLiteral("show-menu")),
        m_menuBarVisible ? i18n("Hide Menu Bar (Ctrl+M)") : i18n("Show Menu Bar (Ctrl+M)"));
    connect(menuBarAct, &QAction::triggered, this, &WhatsAppView::toggleMenuBarRequested);

    if (!m_menuBarVisible) {
        auto *settingsAct = menu.addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure HatsApp..."));
        connect(settingsAct, &QAction::triggered, this, &WhatsAppView::openSettingsRequested);
    }

    // Optional Developer Tools entry when Developer Mode is enabled
    if (m_config.developerMode()) {
        menu.addSeparator();
        auto *inspectAct = menu.addAction(QIcon::fromTheme(QStringLiteral("tools-report-bug")), i18n("Inspect Element"));
        connect(inspectAct, &QAction::triggered, this, &WhatsAppView::inspectElementRequested);
    }

    menu.exec(event->globalPos());
    event->accept();
}

void WhatsAppView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int delta = event->angleDelta().y();
        if (delta > 0) {
            zoomInStep();
        } else if (delta < 0) {
            zoomOutStep();
        }
        event->accept();
        return;
    }
    QWebEngineView::wheelEvent(event);
}

} // namespace hatsapp
