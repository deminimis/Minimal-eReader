#include "mainwindow.h"
#include "document.h"
#include "qpushbutton.h"
#include "viewerwidget.h"
#include "searchpanel.h" // Though unused in the new system, good to keep for now

#include <QApplication>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QTabWidget>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QPalette>
#include <QMouseEvent>
#include <QShortcut>
#include <QCloseEvent>
#include <QStyle>
#include <QWheelEvent>
#include <QFileInfo>
#include <QClipboard>
#include <QInputDialog>
#include <QTextStream>
#include <QDateTime>
#include <QVariantMap>
#include <QColorDialog>
#include <QTimer>
#include <QDockWidget>
#include <QListWidget>
#include <QLineEdit>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_customTitleBar(nullptr),
    m_menuButton(nullptr),
    m_mainMenu(nullptr),
    m_recentFilesMenu(nullptr),
    m_tabWidget(nullptr),
    m_statusBar(nullptr),
    m_pageLabel(nullptr),
    m_zoomLabel(nullptr),
    m_searchDockWidget(nullptr),
    m_searchResultsList(nullptr),
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
    // Create the single MuPDF context for the entire application
    m_mupdfContext = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_mupdfContext) {
        throw std::runtime_error("Failed to create MuPDF context.");
    }

    // Register document handlers once
    fz_try(m_mupdfContext) {
        fz_register_document_handlers(m_mupdfContext);
    } fz_catch(m_mupdfContext) {
        fz_drop_context(m_mupdfContext);
        m_mupdfContext = nullptr;
        throw std::runtime_error("Failed to register MuPDF document handlers.");
    }

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

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_isInitialShow) {
        m_isInitialShow = false;
        loadAppSettings();
        QTimer::singleShot(0, this, &MainWindow::restoreLastTabs);
    }
}

void MainWindow::restoreLastTabs()
{
    for (const QVariant &tabData : m_settings.lastOpenTabs) {
        QVariantMap map = tabData.toMap();
        QString filePath = map.value("filePath").toString();
        int pageNum = map.value("pageNum").toInt();
        if (!filePath.isEmpty()) {
            openFileFromPath(filePath, pageNum);
        }
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

    if (m_settings.isMaximized) {
        showMaximized();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings.isMaximized = isMaximized();
    if (!isMaximized()) {
        m_settings.windowSize = size();
        m_settings.windowPosition = pos();
    }

    m_settings.lastOpenTabs.clear();
    for(Document* doc : m_documents) {
        QVariantMap tabData;
        tabData["filePath"] = doc->getFilepath();
        tabData["pageNum"] = doc->getCurrentPage();
        m_settings.lastOpenTabs.append(tabData);
    }

    m_settings.save();
    event->accept();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        event->accept();
    } else {
        QMainWindow::wheelEvent(event);
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setMouseTracking(true);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);
    createCustomTitleBar();
    mainLayout->addWidget(m_customTitleBar);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    mainLayout->addWidget(m_tabWidget);
    setCentralWidget(centralWidget);

    m_statusBar = new QStatusBar(this);
    m_pageLabel = new QLabel(this);
    m_pageLabel->setToolTip("Click to jump to a page");
    m_pageLabel->installEventFilter(this);
    m_zoomLabel = new QLabel(this);
    m_statusBar->addPermanentWidget(m_pageLabel, 1);
    m_statusBar->addPermanentWidget(m_zoomLabel, 0);

    mainLayout->addWidget(m_statusBar);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    createSearchDockWidget();
}

void MainWindow::createCustomTitleBar()
{
    m_customTitleBar = new QWidget(this);
    m_customTitleBar->setMouseTracking(true);
    m_customTitleBar->setObjectName("customTitleBar");
    m_customTitleBar->setFixedHeight(32);
    m_customTitleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_customTitleBar->installEventFilter(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(m_customTitleBar);
    titleLayout->setContentsMargins(5, 0, 5, 0);
    titleLayout->setSpacing(5);
    m_menuButton = new QToolButton(this);
    m_menuButton->setText(QString::fromUtf8("\u2630"));
    m_mainMenu = new QMenu(this);
    m_menuButton->setMenu(m_mainMenu);
    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menuButton->setArrowType(Qt::NoArrow);
    titleLayout->addWidget(m_menuButton);
    QLabel* titleLabel = new QLabel("Lightweight E-Reader", this);
    titleLabel->setMouseTracking(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(titleLabel, 1);
    QToolButton* minimizeButton = new QToolButton(this);
    minimizeButton->setText("_");
    minimizeButton->setObjectName("minimizeButton");
    connect(minimizeButton, &QToolButton::clicked, this, &MainWindow::showMinimized);
    QToolButton* maximizeButton = new QToolButton(this);
    maximizeButton->setText("[]");
    maximizeButton->setObjectName("maximizeButton");
    connect(maximizeButton, &QToolButton::clicked, this, [this](){ isMaximized() ? showNormal() : showMaximized(); });
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setText("X");
    closeButton->setObjectName("closeButton");
    connect(closeButton, &QToolButton::clicked, this, &MainWindow::close);
    titleLayout->addWidget(minimizeButton);
    titleLayout->addWidget(maximizeButton);
    titleLayout->addWidget(closeButton);
}

void MainWindow::createActions()
{
    m_openAction = new QAction("&Open Document...", this);
    m_copyAction = new QAction("&Copy", this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setEnabled(false);
    m_searchAction = new QAction("&Search...", this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_goToPageAction = new QAction("&Go to Page...", this);
    m_toggleStatusBarAction = new QAction("Toggle Status &Bar", this);
    m_toggleStatusBarAction->setCheckable(true);
    m_invertColorsAction = new QAction("&Invert Page Colors", this);
    m_invertColorsAction->setCheckable(true);
    m_exitAction = new QAction("E&xit", this);

    m_mainMenu->addAction(m_openAction);
    m_recentFilesMenu = m_mainMenu->addMenu("Open &Recent");
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_searchAction);
    m_mainMenu->addAction(m_goToPageAction);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_invertColorsAction);
    m_mainMenu->addAction(m_toggleStatusBarAction);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_exitAction);

    updateRecentFilesMenu();

    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_searchAction, &QAction::triggered, this, [this](){
        m_searchDockWidget->show();
        m_searchInput->setFocus();
        m_searchInput->selectAll();
    });
    connect(m_goToPageAction, &QAction::triggered, this, &MainWindow::promptForPageNumber);

    connect(m_copyAction, &QAction::triggered, this, [this] {
        if (m_lastSelectionRect.isValid()) {
            Document* doc = m_documents.at(m_tabWidget->currentIndex());
            QString selectedText = doc->getSelectedText(m_lastSelectionRect, m_settings.zoomFactor);
            copySelection(selectedText);
        }
    });
    connect(m_toggleStatusBarAction, &QAction::triggered, this, &MainWindow::toggleStatusBar);
    connect(m_invertColorsAction, &QAction::triggered, this, &MainWindow::invertPageColors);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::close);

    new QShortcut(QKeySequence::Open, this, SLOT(openFile()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, SLOT(closeCurrentTab()));
    new QShortcut(QKeySequence::Quit, this, SLOT(close()));
    new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(nextPage()));
    new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(prevPage()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this, SLOT(fitToWindow()));

    auto* zoomInShortcut = new QShortcut(this);
    zoomInShortcut->setKey(Qt::CTRL | Qt::Key_Equal);
    connect(zoomInShortcut, &QShortcut::activated, this, &MainWindow::zoomIn);

    auto* zoomOutShortcut = new QShortcut(this);
    zoomOutShortcut->setKey(Qt::CTRL | Qt::Key_Minus);
    connect(zoomOutShortcut, &QShortcut::activated, this, &MainWindow::zoomOut);

    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this, SLOT(onSavePassageShortcut()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_J), this, SLOT(onSaveCommentShortcut()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this, SLOT(onSavePageNoteShortcut()));
    new QShortcut(QKeySequence(Qt::Key_I), this, SLOT(invertPageColors()));

    // Add F11 shortcut for fullscreen
    QShortcut *fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullScreen);
}


void MainWindow::setupTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(30, 31, 34));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(20, 21, 23));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    qApp->setPalette(palette);

    QString style = R"(
        QToolButton {
            border: none;
            padding: 2px;
            background-color: transparent;
            color: white;
        }
        QToolButton:hover {
            background-color: #3a3a3a;
        }
        /* This rule will hide the dropdown arrow on the menu button */
        QToolButton::menu-indicator {
            image: none;
        }
        QToolButton#minimizeButton:hover {
            background-color: #226622;
        }
        QToolButton#maximizeButton:hover {
            background-color: #226622;
        }
        QToolButton#closeButton:hover, QToolButton#dockWidgetCloseButton:hover {
            background-color: #882222; /* Red on hover for both close buttons */
        }
        QToolButton#dockWidgetCloseButton {
            font-weight: bold;
        }
        QPushButton#searchNavButton {
            background-color: transparent;
            color: white;
            border: 1px solid #555555;
            padding: 2px;
            min-width: 25px;
        }
        QPushButton#searchNavButton:hover {
            background-color: #3a3a3a;
            border-color: #777777;
        }
        QMenu {
            background-color: #2a2a2a;
            color: white;
        }
        QMenu::item:selected {
            background-color: #444444;
        }
        #customTitleBar {
            background-color: #2a2a2a;
        }
        QDockWidget {
            color: white;
        }
        QDockWidget::title {
            background-color: #3a3a3a; /* Made title slightly different for clarity */
            text-align: left;
            padding-left: 5px;
        }
    )";

    setStyleSheet(style);
}


void MainWindow::openFileFromPath(const QString &filePath, int pageNum)
{
    for(int i = 0; i < m_documents.count(); ++i) {
        if(m_documents.at(i)->getFilepath() == filePath) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }
    Document *doc = new Document(m_mupdfContext, filePath);
    if (!doc->load()) {
        QMessageBox::critical(this, "Error", "Failed to load the document.");
        delete doc;
        return;
    }

    doc->goToPage(pageNum);

    m_documents.append(doc);
    m_settings.recentFiles.removeAll(filePath);
    m_settings.recentFiles.prepend(filePath);
    while(m_settings.recentFiles.size() > 15) m_settings.recentFiles.removeLast();
    updateRecentFilesMenu();
    ViewerWidget *viewer = new ViewerWidget(this);
    viewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(viewer, &ViewerWidget::customContextMenuRequested, this, &MainWindow::showPageContextMenu);
    connect(viewer, &ViewerWidget::textSelected, this, &MainWindow::onTextSelected);
    int newTabIndex = m_tabWidget->addTab(viewer, QFileInfo(filePath).fileName());
    m_tabWidget->setCurrentIndex(newTabIndex);
    renderActivePage();
}

void MainWindow::openRecentFile()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        openFileFromPath(action->data().toString());
    }
}

void MainWindow::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();
    if (m_settings.recentFiles.isEmpty()) {
        m_recentFilesMenu->setEnabled(false);
        return;
    }
    m_recentFilesMenu->setEnabled(true);
    for (const QString &filePath : m_settings.recentFiles) {
        QAction *action = new QAction(QFileInfo(filePath).fileName(), this);
        action->setData(filePath);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
        m_recentFilesMenu->addAction(action);
    }
}

void MainWindow::renderActivePage()
{
    clearSelectionState();
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }

    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    Document* doc = m_documents.at(index);
    if (ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index))) {
        QImage pageImage = doc->renderCurrentPage(m_settings.zoomFactor, m_settings.invertPageColors);
        viewer->setPageImage(pageImage);
        QVector<QRectF> charRects = doc->getPageCharRects(doc->getCurrentPage(), m_settings.zoomFactor);
        viewer->setCharRects(charRects);
        viewer->scrollToTop();
    }
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0 || !m_statusBar->isVisible()) {
        if(m_pageLabel) m_pageLabel->clear();
        if(m_zoomLabel) m_zoomLabel->clear();
        return;
    }
    Document* doc = m_documents.at(index);
    m_pageLabel->setText(QString("Page %1 of %2").arg(doc->getCurrentPage() + 1).arg(doc->getPageCount()));
    m_zoomLabel->setText(QString("Zoom: %1%").arg(static_cast<int>(m_settings.zoomFactor * 100)));
}

void MainWindow::onTabChanged(int index)
{
    clearSelectionState();
    if (index >= 0) {
        renderActivePage();
        clearSearch();
    } else {
        m_statusBar->clearMessage();
    }
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0) return;

    QWidget* tabWidget = m_tabWidget->widget(index);
    if (tabWidget) {
        m_tabWidget->removeTab(index);
        delete tabWidget;
    }

    if (index < m_documents.count()) {
        Document* doc = m_documents.takeAt(index);
        delete doc;
    }
    clearSearch();
}

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Document", "", "Documents (*.pdf *.epub *.xps *.cbz *.fb2);;All Files (*)");
    if (!filePath.isEmpty()) {
        openFileFromPath(filePath);
    }
}

void MainWindow::toggleStatusBar()
{
    m_settings.isStatusBarVisible = !m_statusBar->isVisible();
    m_statusBar->setVisible(m_settings.isStatusBarVisible);
    m_toggleStatusBarAction->setChecked(m_settings.isStatusBarVisible);
    updateStatusBar();
}

void MainWindow::invertPageColors()
{
    m_settings.invertPageColors = !m_settings.invertPageColors;
    m_invertColorsAction->setChecked(m_settings.invertPageColors);
    renderActivePage();
}

void MainWindow::closeCurrentTab()
{
    if (m_tabWidget->count() > 0) {
        onTabCloseRequested(m_tabWidget->currentIndex());
    }
}


void MainWindow::nextPage()
{
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }
    clearSelectionState();
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    m_documents.at(index)->goToNextPage();
    renderActivePage();
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index))) {
        viewer->scrollToTop();
    }
}


void MainWindow::prevPage()
{
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }
    clearSelectionState();
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    m_documents.at(index)->goToPrevPage();
    renderActivePage();
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index))) {
        viewer->scrollToBottom();
    }
}

void MainWindow::zoomIn()
{
    m_settings.zoomFactor *= 1.1;
    renderActivePage();
}

void MainWindow::zoomOut()
{
    m_settings.zoomFactor /= 1.1;
    renderActivePage();
}

void MainWindow::fitToWindow()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    Document* doc = m_documents.at(index);
    ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index));
    if (!doc || !viewer) return;

    QSizeF pageSize = doc->getOriginalPageSize(doc->getCurrentPage());
    if (pageSize.isEmpty()) return;

    QRectF viewportRect = viewer->viewport()->rect();
    if (viewportRect.isEmpty()) return;

    qreal xZoom = viewportRect.width() / pageSize.width();
    qreal yZoom = viewportRect.height() / pageSize.height();
    m_settings.zoomFactor = std::min(xZoom, yZoom);
    renderActivePage();
}

void MainWindow::promptForPageNumber()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    Document* doc = m_documents.at(index);

    bool ok;
    int page = QInputDialog::getInt(this, "Go to Page", "Enter page number:",
                                    doc->getCurrentPage() + 1, // current
                                    1,                         // min
                                    doc->getPageCount(),       // max
                                    1, &ok);
    if (ok && (page - 1) != doc->getCurrentPage()) {
        doc->goToPage(page - 1); // Document uses 0-based index
        renderActivePage();
    }
}

void MainWindow::updateResizeCursor(const QPoint &pos)
{
    if (isMaximized() || isFullScreen()) {
        unsetCursor();
        return;
    }

    const int borderWidth = 5;
    Qt::Edge top = (pos.y() < borderWidth) ? Qt::TopEdge : Qt::Edge(0);
    Qt::Edge bottom = (pos.y() > height() - borderWidth) ? Qt::BottomEdge : Qt::Edge(0);
    Qt::Edge left = (pos.x() < borderWidth) ? Qt::LeftEdge : Qt::Edge(0);
    Qt::Edge right = (pos.x() > width() - borderWidth) ? Qt::RightEdge : Qt::Edge(0);

    m_resizeEdge = top | bottom | left | right;

    if (m_resizeEdge == (Qt::TopEdge | Qt::LeftEdge) || m_resizeEdge == (Qt::BottomEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (m_resizeEdge == (Qt::TopEdge | Qt::RightEdge) || m_resizeEdge == (Qt::BottomEdge | Qt::LeftEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (m_resizeEdge & (Qt::LeftEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else if (m_resizeEdge & (Qt::TopEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        unsetCursor();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !isMaximized()) {
        if (m_resizeEdge != 0) {
            m_isResizing = true;
            m_dragStartPosition = event->globalPosition().toPoint();
            m_resizeStartGeometry = geometry();
        } else if (m_customTitleBar->underMouse()) {
            m_isDragging = true;
            m_dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isResizing) {
        QPoint delta = event->globalPosition().toPoint() - m_dragStartPosition;
        QRect newGeometry = m_resizeStartGeometry;

        if (m_resizeEdge & Qt::LeftEdge) {
            newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        }
        if (m_resizeEdge & Qt::RightEdge) {
            newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        }
        if (m_resizeEdge & Qt::TopEdge) {
            newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        }
        if (m_resizeEdge & Qt::BottomEdge) {
            newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());
        }

        if (newGeometry.width() >= minimumWidth() && newGeometry.height() >= minimumHeight()) {
            setGeometry(newGeometry);
        }

    } else if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStartPosition);
    } else {
        updateResizeCursor(event->pos());
    }
    event->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_isResizing = false;
        unsetCursor();
        event->accept();
    }
}

void MainWindow::onTextSelected(const QRect& rect)
{
    if (rect.isNull()) {
        clearSelectionState();
        return;
    }

    Document* doc = m_documents.at(m_tabWidget->currentIndex());
    QString selectedText = doc->getSelectedText(rect, m_settings.zoomFactor);

    if (!selectedText.trimmed().isEmpty()) {
        m_lastSelectionRect = rect;
        m_copyAction->setEnabled(true);
    } else {
        clearSelectionState();
    }
}

void MainWindow::copySelection(const QString& selectedText)
{
    if (!selectedText.isEmpty()) {
        QApplication::clipboard()->setText(selectedText);
    }
}

void MainWindow::savePassage(const QString& selectedText)
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    if (selectedText.trimmed().isEmpty()) return;

    Document* doc = m_documents.at(index);
    QFileInfo docInfo(doc->getFilepath());
    QString notesPath = docInfo.dir().filePath(docInfo.completeBaseName() + "_NOTES.txt");
    QFile notesFile(notesPath);
    if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not write to notes file.");
        return;
    }

    QTextStream out(&notesFile);
    QString location = QString("Page %1").arg(doc->getCurrentPage() + 1);
    QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    out << "\n\n" << location << " (" << dateTime << ")\n" << selectedText;

    clearSelectionState();
}

void MainWindow::saveComment(const QString& selectedText)
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    if (selectedText.trimmed().isEmpty()) return;

    bool ok;
    QString commentText = QInputDialog::getText(this, "Add Comment", "Enter your note:", QLineEdit::Normal, "", &ok);

    if (ok && !commentText.isEmpty()) {
        Document* doc = m_documents.at(index);
        QFileInfo docInfo(doc->getFilepath());
        QString notesPath = docInfo.dir().filePath(docInfo.completeBaseName() + "_NOTES.txt");
        QFile notesFile(notesPath);
        if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Could not write to notes file.");
            return;
        }

        QTextStream out(&notesFile);
        QString location = QString("Page %1").arg(doc->getCurrentPage() + 1);
        QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        out << "\n\n" << location << " (" << dateTime << ")\n" << selectedText;
        out << "\n\nCOMMENT: " << commentText;
    }

    clearSelectionState();
}

void MainWindow::showPageContextMenu(const QPoint& pos)
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index));
    if (!viewer) return;

    QMenu contextMenu(this);

    if (viewer->hasSelection()) {
        Document* doc = m_documents.at(index);
        QString selectedText = doc->getSelectedText(m_lastSelectionRect, m_settings.zoomFactor);

        if (!selectedText.trimmed().isEmpty()) {
            QAction* copyAct = contextMenu.addAction("Copy - ctrl + c");
            QAction* highlightAct = contextMenu.addAction("Save Passage - ctrl + h");
            QAction* commentAct = contextMenu.addAction("Save Comment - ctrl + j");
            contextMenu.addSeparator();
            connect(copyAct, &QAction::triggered, this, [this, selectedText]{ copySelection(selectedText); });
            connect(highlightAct, &QAction::triggered, this, [this, selectedText]{ savePassage(selectedText); });
            connect(commentAct, &QAction::triggered, this, [this, selectedText]{ saveComment(selectedText); });
        }
    }

    QAction* noteAction = contextMenu.addAction("Save Page Note - ctrl + k");
    connect(noteAction, &QAction::triggered, this, &MainWindow::savePageNote);
    contextMenu.exec(viewer->mapToGlobal(pos));
}

void MainWindow::savePageNote()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    bool ok;
    QString commentText = QInputDialog::getText(this, "Add Page Note", "Enter your note for this page:", QLineEdit::Normal, "", &ok);
    if (ok && !commentText.isEmpty()) {
        Document* doc = m_documents.at(index);
        QFileInfo docInfo(doc->getFilepath());
        QString notesPath = docInfo.dir().filePath(docInfo.completeBaseName() + "_NOTES.txt");
        QFile notesFile(notesPath);
        if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Could not write to notes file.");
            return;
        }

        QTextStream out(&notesFile);
        QString location = QString("Page %1").arg(doc->getCurrentPage() + 1);
        QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        out << "\n\n" << location << " NOTE (" << dateTime << ")\n";
        out << commentText;
    }
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

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    // Handle double-click on the title bar to maximize/restore
    if (obj == m_customTitleBar && event->type() == QEvent::MouseButtonDblClick) {
        isMaximized() ? showNormal() : showMaximized();
        return true; // Event handled
    }

    // Handle click on the page label to jump to a page
    if (obj == m_pageLabel && event->type() == QEvent::MouseButtonPress) {
        promptForPageNumber();
        return true; // Event handled
    }

    // Handle Ctrl+Plus/Minus for zooming
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            if (keyEvent->key() == Qt::Key_Minus)
                zoomOut();
            else if (keyEvent->key() == Qt::Key_Equal)
                zoomIn();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::onSavePassageShortcut()
{
    if (m_lastSelectionRect.isValid()) {
        Document* doc = m_documents.at(m_tabWidget->currentIndex());
        if (!doc) return;
        QString selectedText = doc->getSelectedText(m_lastSelectionRect, m_settings.zoomFactor);
        savePassage(selectedText);
    }
}

void MainWindow::onSaveCommentShortcut()
{
    if (m_lastSelectionRect.isValid()) {
        Document* doc = m_documents.at(m_tabWidget->currentIndex());
        if (!doc) return;
        QString selectedText = doc->getSelectedText(m_lastSelectionRect, m_settings.zoomFactor);
        saveComment(selectedText);
    }
}

void MainWindow::onSavePageNoteShortcut()
{
    savePageNote();
}

void MainWindow::createSearchDockWidget()
{
    m_searchDockWidget = new QDockWidget(this);
    m_searchDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // Create a custom title bar for the dock widget
    QWidget* dockTitleBar = new QWidget();
    QHBoxLayout* titleLayout = new QHBoxLayout(dockTitleBar);
    titleLayout->setContentsMargins(5, 0, 5, 0);
    titleLayout->addWidget(new QLabel("Search"));
    titleLayout->addStretch();
    QToolButton* dockCloseButton = new QToolButton();
    dockCloseButton->setText("X");
    dockCloseButton->setObjectName("dockWidgetCloseButton"); // Name for styling
    connect(dockCloseButton, &QToolButton::clicked, m_searchDockWidget, &QDockWidget::close);
    titleLayout->addWidget(dockCloseButton);
    m_searchDockWidget->setTitleBarWidget(dockTitleBar);

    // Create the main content widget for the dock
    QWidget* searchWidget = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(searchWidget);
    mainLayout->setContentsMargins(4,4,4,4);
    mainLayout->setSpacing(4);

    QHBoxLayout* inputLayout = new QHBoxLayout;
    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText("Search document...");
    inputLayout->addWidget(m_searchInput);

    QPushButton* prevButton = new QPushButton("\u25B2");
    prevButton->setObjectName("searchNavButton");
    prevButton->setToolTip("Previous result");
    inputLayout->addWidget(prevButton);

    QPushButton* nextButton = new QPushButton("\u25BC");
    nextButton->setObjectName("searchNavButton");
    nextButton->setToolTip("Next result");
    inputLayout->addWidget(nextButton);

    m_searchResultsList = new QListWidget;

    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(m_searchResultsList);
    m_searchDockWidget->setWidget(searchWidget);

    addDockWidget(Qt::RightDockWidgetArea, m_searchDockWidget);
    m_searchDockWidget->hide();

    // --- Connect Signals and Slots ---
    connect(m_searchInput, &QLineEdit::returnPressed, this, [this](){
        executeSearch(m_searchInput->text());
    });
    connect(prevButton, &QPushButton::clicked, this, &MainWindow::findPrevSearchResult);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::findNextSearchResult);
    connect(m_searchResultsList, &QListWidget::itemClicked, this, &MainWindow::onSearchResultClicked);
    connect(m_searchResultsList, &QListWidget::currentItemChanged, this, &MainWindow::onSearchResultChanged);
}

void MainWindow::executeSearch(const QString& text)
{
    m_searchResultsList->clear();
    int index = m_tabWidget->currentIndex();
    if (index < 0 || text.isEmpty()) {
        return;
    }

    Document* doc = m_documents.at(index);
    QVector<SearchResult> results = doc->searchDocument(text);

    if (results.isEmpty()) {
        m_searchResultsList->addItem("No results found.");
    } else {
        for (const SearchResult& result : results) {
            QListWidgetItem* item = new QListWidgetItem(
                QString("Page %1: %2").arg(result.pageNum + 1).arg(result.context),
                m_searchResultsList
                );
            item->setData(Qt::UserRole, QVariant::fromValue(result));
        }
    }
    m_searchDockWidget->show();
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item)
{
    if (!item) return;

    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<SearchResult>()) return;

    SearchResult result = data.value<SearchResult>();
    int tabIndex = m_tabWidget->currentIndex();
    if (tabIndex < 0) return;

    qDebug() << "MainWindow: Attempting to highlight result at original page coordinates:" << result.location;

    Document* doc = m_documents.at(tabIndex);
    ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(tabIndex));
    if (!doc || !viewer) return;

    // This is the function that will perform the highlight and jump.
    // We define it here so we can call it immediately or with a slight delay.
    auto highlightAction = [viewer, result, this] {
        viewer->setHighlight(result.location, m_settings.zoomFactor);
    };

    // If we need to switch pages, render the page first, then use a 0ms timer.
    // This queues the highlight action to run immediately after the event loop
    // has finished processing the page change, ensuring it works reliably.
    if (doc->getCurrentPage() != result.pageNum) {
        doc->goToPage(result.pageNum);
        renderActivePage();
        QTimer::singleShot(0, this, highlightAction);
    } else {
        // If we are already on the correct page, we can highlight immediately.
        highlightAction();
    }
}

void MainWindow::onSearchResultChanged(QListWidgetItem* current, QListWidgetItem* /*previous*/)
{
    // This slot handles keyboard navigation (up/down arrows) in the results list.
    onSearchResultClicked(current);
}

void MainWindow::findNextSearchResult()
{
    int count = m_searchResultsList->count();
    if (count == 0) return;

    int nextRow = m_searchResultsList->currentRow() + 1;
    if (nextRow >= count) {
        nextRow = 0; // Wrap around to the top
    }
    m_searchResultsList->setCurrentRow(nextRow);
}

void MainWindow::findPrevSearchResult()
{
    int count = m_searchResultsList->count();
    if (count == 0) return;

    int prevRow = m_searchResultsList->currentRow() - 1;
    if (prevRow < 0) {
        prevRow = count - 1; // Wrap around to the bottom
    }
    m_searchResultsList->setCurrentRow(prevRow);
}

void MainWindow::clearSearch()
{
    // Clear the list of results in the side panel
    if (m_searchResultsList) {
        m_searchResultsList->clear();
    }
    // Clear any highlight on the current page
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}
