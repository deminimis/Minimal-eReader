#include "mainwindow.h"
#include "viewerwidget.h"

#include <QApplication>
#include <QStatusBar>
#include <QLabel>
#include <stdexcept>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_customTitleBar(nullptr),
    m_menuButton(nullptr),
    m_mainMenu(nullptr),
    m_recentFilesMenu(nullptr),
    m_favoritesMenu(nullptr),
    m_tabWidget(nullptr),
    m_statusBar(nullptr),
    m_pageLabel(nullptr),
    m_zoomLabel(nullptr),
    m_searchDockWidget(nullptr),
    m_searchResultsList(nullptr),
    m_searchInput(nullptr),
    m_openAction(nullptr),
    m_copyAction(nullptr),
    m_searchAction(nullptr),
    m_goToPageAction(nullptr),
    m_invertColorsAction(nullptr),
    m_toggleStatusBarAction(nullptr),
    m_exitAction(nullptr),
    m_resizeStartGeometry(),
    m_isDragging(false),
    m_dragStartPosition(),
    m_isResizing(false),
    m_resizeEdge(Qt::Edge(0)),
    m_isInitialShow(true),
    m_mupdfContext(nullptr)
{
    m_mupdfContext = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_mupdfContext) {
        throw std::runtime_error("Failed to create MuPDF context.");
    }

    fz_try(m_mupdfContext) {
        fz_register_document_handlers(m_mupdfContext);
    } fz_catch(m_mupdfContext) {
        fz_drop_context(m_mupdfContext);
        m_mupdfContext = nullptr;
        throw std::runtime_error("Failed to register MuPDF document handlers.");
    }

    m_pageCache.setMaxCost(100 * 1024 * 1024); // Set cache limit to 100 MB

    setWindowFlags(Qt::FramelessWindowHint);
    setMouseTracking(true);
    setWindowIcon(QIcon(":/appicon.ico"));
    setupUI();
    createActions();
    setupTheme();
    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_documents);
    if (m_mupdfContext) {
        fz_drop_context(m_mupdfContext);
    }
}

void MainWindow::loadAppSettings()
{
    m_settings.load();
    resize(m_settings.windowSize);
    move(m_settings.windowPosition);
    m_statusBar->setVisible(m_settings.isStatusBarVisible);
    m_toggleStatusBarAction->setChecked(m_settings.isStatusBarVisible);
    m_invertColorsAction->setChecked(m_settings.invertPageColors);
    updateFavoritesMenu();

    if (m_settings.isMaximized) {
        showMaximized();
    }
}

void MainWindow::updateStatusBar()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0 || !m_statusBar->isVisible()) {
        if(m_pageLabel) m_pageLabel->clear();
        if(m_zoomLabel) m_zoomLabel->clear();
        return;
    }
    const Document* doc = m_documents.at(index);
    m_pageLabel->setText(QStringLiteral("Page %1 of %2").arg(doc->getCurrentPage() + 1).arg(doc->getPageCount()));
    m_zoomLabel->setText(QStringLiteral("Zoom: %1%").arg(static_cast<int>(m_settings.zoomFactor * 100)));
}

void MainWindow::clearSelectionState()
{
    m_lastSelectionRect = QRect();
    m_copyAction->setEnabled(false);
    int index = m_tabWidget->currentIndex();
    if (index >= 0) {
        if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index))) {
            viewer->clearSelection();
        }
    }
}
