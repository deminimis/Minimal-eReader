#include "mainwindow.h"

#include <QApplication>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QTabWidget>
#include <QStatusBar>
#include <QShortcut>
#include <QFileInfo>
#include <QDockWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <QActionGroup>

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
    setStatusBar(m_statusBar);

    // Left-aligned Widgets
    m_pageLabel = new QLabel(this);
    m_pageLabel->setToolTip(QStringLiteral("Click to jump to a page"));
    m_pageLabel->installEventFilter(this);
    m_statusBar->addWidget(m_pageLabel);

    // Centered Widgets
    m_prevPageButton = new QToolButton(this);
    m_prevPageButton->setText(QStringLiteral("\u25C0"));
    m_prevPageButton->setToolTip("Previous Page");
    m_nextPageButton = new QToolButton(this);
    m_nextPageButton->setText(QStringLiteral("\u25B6"));
    m_nextPageButton->setToolTip("Next Page");

    m_statusBar->addWidget(new QWidget(), 1); // Left stretch
    m_statusBar->addWidget(m_prevPageButton);
    m_statusBar->addWidget(m_nextPageButton);
    m_statusBar->addWidget(new QWidget(), 1); // Right stretch

    // Right-aligned (Permanent) Widgets
    m_zoomLabel = new QLabel(this);
    m_zoomLabel->setToolTip("Click to set zoom level");
    m_zoomLabel->installEventFilter(this);
    m_statusBar->addPermanentWidget(m_zoomLabel);

    connect(m_prevPageButton, &QToolButton::clicked, this, &MainWindow::prevPage);
    connect(m_nextPageButton, &QToolButton::clicked, this, &MainWindow::nextPage);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    createTocDockWidget();
    createSearchDockWidget();
    createNotesDockWidget();
}

void MainWindow::createCustomTitleBar()
{
    m_customTitleBar = new QWidget(this);
    m_customTitleBar->setMouseTracking(true);
    m_customTitleBar->setObjectName(QStringLiteral("customTitleBar"));
    m_customTitleBar->setFixedHeight(32);
    m_customTitleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_customTitleBar->installEventFilter(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(m_customTitleBar);
    titleLayout->setContentsMargins(5, 0, 5, 0);
    titleLayout->setSpacing(5);
    m_menuButton = new QToolButton(this);
    m_menuButton->setText(QStringLiteral("\u2630"));
    m_mainMenu = new QMenu(this);
    m_menuButton->setMenu(m_mainMenu);
    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menuButton->setArrowType(Qt::NoArrow);
    titleLayout->addWidget(m_menuButton);
    QLabel* titleLabel = new QLabel(QStringLiteral("Lightweight E-Reader"), this);
    titleLabel->setMouseTracking(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(titleLabel, 1);
    QToolButton* minimizeButton = new QToolButton(this);
    minimizeButton->setText(QStringLiteral("_"));
    minimizeButton->setObjectName(QStringLiteral("minimizeButton"));
    connect(minimizeButton, &QToolButton::clicked, this, &MainWindow::showMinimized);
    QToolButton* maximizeButton = new QToolButton(this);
    maximizeButton->setText(QStringLiteral("[]"));
    maximizeButton->setObjectName(QStringLiteral("maximizeButton"));
    connect(maximizeButton, &QToolButton::clicked, this, [this](){ isMaximized() ? showNormal() : showMaximized(); });
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setText(QStringLiteral("X"));
    closeButton->setObjectName(QStringLiteral("closeButton"));
    connect(closeButton, &QToolButton::clicked, this, &MainWindow::close);
    titleLayout->addWidget(minimizeButton);
    titleLayout->addWidget(maximizeButton);
    titleLayout->addWidget(closeButton);
}

void MainWindow::createActions()
{
    m_openAction = new QAction(QStringLiteral("&Open Document..."), this);
    m_copyAction = new QAction(QStringLiteral("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setEnabled(false);
    m_tocAction = new QAction(QStringLiteral("Table of &Contents\tCtrl+Shift+T"), this);
    m_tocAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    m_tocAction->setEnabled(false);
    m_notesAction = new QAction(QStringLiteral("View &Notes\tCtrl+Shift+N"), this);
    m_notesAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    m_notesAction->setEnabled(false);
    m_searchAction = new QAction(QStringLiteral("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_goToPageAction = new QAction(QStringLiteral("&Go to Page..."), this);
    m_toggleStatusBarAction = new QAction(QStringLiteral("Toggle Status &Bar"), this);
    m_toggleStatusBarAction->setCheckable(true);

    m_invertColorsAction = new QAction(
        QStringLiteral("&Invert Page Colors\tCtrl+I"), this);
    m_invertColorsAction->setShortcut(QKeySequence("Ctrl+I"));
    m_invertColorsAction->setCheckable(true);

    m_darkThemeAction = new QAction(QStringLiteral("&Dark"), this);
    m_darkThemeAction->setCheckable(true);
    m_darkThemeAction->setData(QStringLiteral("dark"));

    m_lightThemeAction = new QAction(QStringLiteral("&Light"), this);
    m_lightThemeAction->setCheckable(true);
    m_lightThemeAction->setData(QStringLiteral("light"));

    QActionGroup* themeActionGroup = new QActionGroup(this);
    themeActionGroup->setExclusive(true);
    themeActionGroup->addAction(m_darkThemeAction);
    themeActionGroup->addAction(m_lightThemeAction);

    m_exitAction = new QAction(QStringLiteral("E&xit"), this);
    QAction* setNotesDirAction = new QAction(QStringLiteral("Set Notes Directory..."), this);


    m_mainMenu->addAction(m_openAction);
    m_recentFilesMenu = m_mainMenu->addMenu(QStringLiteral("Open &Recent"));
    m_favoritesMenu = m_mainMenu->addMenu(QStringLiteral("&Favorites"));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_tocAction);
    m_mainMenu->addAction(m_notesAction);
    m_mainMenu->addAction(m_searchAction);
    m_mainMenu->addAction(m_goToPageAction);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_invertColorsAction);
    m_mainMenu->addAction(m_toggleStatusBarAction);

    QMenu* themeMenu = m_mainMenu->addMenu(QStringLiteral("&Theme"));
    themeMenu->addAction(m_darkThemeAction);
    themeMenu->addAction(m_lightThemeAction);

    m_mainMenu->addAction(setNotesDirAction);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_exitAction);

    updateRecentFilesMenu();
    updateFavoritesMenu();

    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_tocAction, &QAction::triggered, this, &MainWindow::showTableOfContents);
    connect(m_notesAction, &QAction::triggered, this, &MainWindow::showNotes);
    connect(setNotesDirAction, &QAction::triggered, this, &MainWindow::setNotesDirectory);
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
        connect(m_toggleStatusBarAction, &QAction::triggered,
            this, &MainWindow::toggleStatusBar);
    connect(m_invertColorsAction, &QAction::triggered,
            this, &MainWindow::invertPageColors);

    connect(m_darkThemeAction, &QAction::triggered, this, [this] {
        setTheme(QStringLiteral("dark"));
    });

    connect(m_lightThemeAction, &QAction::triggered, this, [this] {
        setTheme(QStringLiteral("light"));
    });

    connect(m_exitAction, &QAction::triggered,
            this, &MainWindow::close);

    new QShortcut(QKeySequence::Open, this, SLOT(openFile()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, SLOT(closeCurrentTab()));
    new QShortcut(QKeySequence::Quit, this, SLOT(close()));
    new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(nextPage()));
    new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(prevPage()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this, SLOT(fitToWindow()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this, SLOT(onSavePassageShortcut()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_J), this, SLOT(onSaveCommentShortcut()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this, SLOT(onSavePageNoteShortcut()));

    auto* zoomInShortcut = new QShortcut(this);
    zoomInShortcut->setKey(Qt::CTRL | Qt::Key_Equal);
    connect(zoomInShortcut, &QShortcut::activated, this, &MainWindow::zoomIn);

    auto* zoomOutShortcut = new QShortcut(this);
    zoomOutShortcut->setKey(Qt::CTRL | Qt::Key_Minus);
    connect(zoomOutShortcut, &QShortcut::activated, this, &MainWindow::zoomOut);

    QShortcut *fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullScreen);
}

void MainWindow::setupTheme()
{
    const bool useLightTheme =
        m_settings.theme.compare(QStringLiteral("light"),
                                 Qt::CaseInsensitive) == 0;

    if (m_darkThemeAction) {
        m_darkThemeAction->setChecked(!useLightTheme);
    }

    if (m_lightThemeAction) {
        m_lightThemeAction->setChecked(useLightTheme);
    }

    if (!useLightTheme) {
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(30, 31, 34));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(20, 21, 23));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, QColor(42, 42, 42));
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::Highlight, QColor(65, 105, 75));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));
        palette.setColor(QPalette::Disabled, QPalette::WindowText,
                         QColor(110, 110, 110));
        palette.setColor(QPalette::Disabled, QPalette::Text,
                         QColor(110, 110, 110));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                         QColor(110, 110, 110));

        qApp->setPalette(palette);

        const QString style = QStringLiteral(R"(
            QMainWindow,
            QWidget {
                color: white;
            }

            QStatusBar {
                background-color: #1e1f22;
                color: lightgray;
                border-top: 1px solid #3a3a3a;
            }

            QStatusBar QToolButton {
                border: none;
                background-color: transparent;
                color: lightgray;
                font-size: 14px;
            }

            QStatusBar QToolButton:hover {
                color: white;
                background-color: #3a3a3a;
            }

            QStatusBar QToolButton:disabled {
                color: #555555;
            }

            QToolButton {
                border: none;
                padding: 2px;
                background-color: transparent;
                color: white;
            }

            QToolButton:hover {
                background-color: #3a3a3a;
            }

            QToolButton:pressed {
                background-color: #484848;
            }

            QToolButton::menu-indicator {
                image: none;
            }

            QToolButton#minimizeButton:hover,
            QToolButton#maximizeButton:hover {
                background-color: #226622;
            }

            QToolButton#closeButton:hover,
            QToolButton#dockWidgetCloseButton:hover {
                background-color: #882222;
                color: white;
            }

            QToolButton#dockWidgetCloseButton {
                font-weight: bold;
            }

            QPushButton#searchNavButton {
                background-color: transparent;
                color: white;
                border: 1px solid #555555;
                border-radius: 3px;
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
                border: 1px solid #444444;
            }

            QMenu::item {
                padding: 5px 24px 5px 24px;
            }

            QMenu::item:selected {
                background-color: #444444;
            }

            QMenu::separator {
                height: 1px;
                background-color: #555555;
                margin: 4px 8px;
            }

            #customTitleBar {
                background-color: #2a2a2a;
                border-bottom: 1px solid #444444;
            }

            QDockWidget {
                color: white;
            }

            QDockWidget::title {
                background-color: #3a3a3a;
                border-bottom: 1px solid #4a4a4a;
                text-align: left;
                padding-left: 5px;
            }

            QTabWidget::pane {
                border: 1px solid #444444;
                background-color: #1e1f22;
            }

            QTabBar::tab {
                background-color: #353535;
                color: #cccccc;
                border: 1px solid #4a4a4a;
                border-bottom: none;
                padding: 6px 10px;
            }

            QTabBar::tab:selected {
                background-color: #202123;
                color: white;
            }

            QTabBar::tab:hover:!selected {
                background-color: #414141;
            }

            QLineEdit,
            QTextEdit,
            QListWidget,
            QTreeWidget {
                background-color: #141517;
                color: white;
                border: 1px solid #4a4a4a;
                selection-background-color: #41694b;
                selection-color: white;
            }

            QScrollArea {
                background-color: #252629;
                border: none;
            }

            QScrollBar:vertical,
            QScrollBar:horizontal {
                background-color: #252629;
                border: none;
            }

            QScrollBar::handle:vertical,
            QScrollBar::handle:horizontal {
                background-color: #555555;
                border-radius: 4px;
                min-height: 24px;
                min-width: 24px;
            }

            QScrollBar::handle:vertical:hover,
            QScrollBar::handle:horizontal:hover {
                background-color: #707070;
            }
        )");

        setStyleSheet(style);
        return;
    }

    const QColor paperWhite(248, 245, 236);
    const QColor panelWhite(252, 250, 244);
    const QColor alternatePaper(241, 236, 224);
    const QColor borderColor(211, 203, 188);
    const QColor textColor(45, 43, 38);
    const QColor mutedText(103, 98, 88);
    const QColor selectionColor(198, 218, 197);
    const QColor selectionText(28, 45, 30);

    QPalette palette;
    palette.setColor(QPalette::Window, paperWhite);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Base, panelWhite);
    palette.setColor(QPalette::AlternateBase, alternatePaper);
    palette.setColor(QPalette::ToolTipBase, panelWhite);
    palette.setColor(QPalette::ToolTipText, textColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Button, alternatePaper);
    palette.setColor(QPalette::ButtonText, textColor);
    palette.setColor(QPalette::BrightText, QColor(150, 35, 35));
    palette.setColor(QPalette::Highlight, selectionColor);
    palette.setColor(QPalette::HighlightedText, selectionText);
    palette.setColor(QPalette::Light, QColor(255, 254, 250));
    palette.setColor(QPalette::Midlight, QColor(232, 226, 213));
    palette.setColor(QPalette::Mid, borderColor);
    palette.setColor(QPalette::Dark, QColor(224, 217, 203));
    palette.setColor(QPalette::Shadow, QColor(165, 156, 141));
    palette.setColor(QPalette::PlaceholderText, mutedText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(145, 139, 128));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(145, 139, 128));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(145, 139, 128));

    qApp->setPalette(palette);

    const QString style = QStringLiteral(R"(
        QMainWindow,
        QWidget {
            color: #2d2b26;
        }

        QStatusBar {
            background-color: #f1ece0;
            color: #676258;
            border-top: 1px solid #d3cbbc;
        }

        QStatusBar QToolButton {
            border: none;
            background-color: transparent;
            color: #676258;
            font-size: 14px;
        }

        QStatusBar QToolButton:hover {
            background-color: #e3eadf;
            color: #263b29;
        }

        QStatusBar QToolButton:disabled {
            color: #aaa397;
        }

        QToolButton {
            border: none;
            padding: 2px;
            background-color: transparent;
            color: #2d2b26;
        }

        QToolButton:hover {
            background-color: #e3eadf;
        }

        QToolButton:pressed {
            background-color: #cfddcb;
        }

        QToolButton::menu-indicator {
            image: none;
        }

        QToolButton#minimizeButton:hover,
        QToolButton#maximizeButton:hover {
            background-color: #dbe8d7;
        }

        QToolButton#closeButton:hover,
        QToolButton#dockWidgetCloseButton:hover {
            background-color: #c95c54;
            color: white;
        }

        QToolButton#dockWidgetCloseButton {
            font-weight: bold;
        }

        QPushButton#searchNavButton {
            background-color: #fcfaf4;
            color: #2d2b26;
            border: 1px solid #d3cbbc;
            border-radius: 3px;
            padding: 2px;
            min-width: 25px;
        }

        QPushButton#searchNavButton:hover {
            background-color: #e3eadf;
            border-color: #9caf9a;
        }

        QMenu {
            background-color: #fcfaf4;
            color: #2d2b26;
            border: 1px solid #d3cbbc;
        }

        QMenu::item {
            padding: 5px 24px 5px 24px;
        }

        QMenu::item:selected {
            background-color: #c6dac5;
            color: #1c2d1e;
        }

        QMenu::separator {
            height: 1px;
            background-color: #d3cbbc;
            margin: 4px 8px;
        }

        #customTitleBar {
            background-color: #f1ece0;
            border-bottom: 1px solid #d3cbbc;
        }

        QDockWidget {
            color: #2d2b26;
        }

        QDockWidget::title {
            background-color: #e9e3d6;
            border-bottom: 1px solid #d3cbbc;
            text-align: left;
            padding-left: 5px;
        }

        QTabWidget::pane {
            border: 1px solid #d3cbbc;
            background-color: #f8f5ec;
        }

        QTabBar::tab {
            background-color: #e9e3d6;
            color: #575248;
            border: 1px solid #d3cbbc;
            border-bottom: none;
            padding: 6px 10px;
        }

        QTabBar::tab:selected {
            background-color: #fcfaf4;
            color: #2d2b26;
        }

        QTabBar::tab:hover:!selected {
            background-color: #f3eee3;
        }

        QLineEdit,
        QTextEdit,
        QListWidget,
        QTreeWidget {
            background-color: #fcfaf4;
            color: #2d2b26;
            border: 1px solid #d3cbbc;
            selection-background-color: #c6dac5;
            selection-color: #1c2d1e;
        }

        QScrollArea {
            background-color: #e0d9cb;
            border: none;
        }

        QScrollBar:vertical,
        QScrollBar:horizontal {
            background-color: #eee9de;
            border: none;
        }

        QScrollBar::handle:vertical,
        QScrollBar::handle:horizontal {
            background-color: #bcb4a6;
            border-radius: 4px;
            min-height: 24px;
            min-width: 24px;
        }

        QScrollBar::handle:vertical:hover,
        QScrollBar::handle:horizontal:hover {
            background-color: #9f978a;
        }
    )");

        setStyleSheet(style);
}

void MainWindow::setTheme(const QString& theme)
{
    const QString normalizedTheme = theme.toLower();

    if (normalizedTheme != QStringLiteral("dark") &&
        normalizedTheme != QStringLiteral("light")) {
        return;
    }

    if (m_settings.theme == normalizedTheme) {
        setupTheme();
        return;
    }

    m_settings.theme = normalizedTheme;
    setupTheme();
    m_settings.save();
}

void MainWindow::createSearchDockWidget()
{
    m_searchDockWidget = new QDockWidget(this);
    m_searchDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* dockTitleBar = new QWidget();
    QHBoxLayout* titleLayout = new QHBoxLayout(dockTitleBar);
    titleLayout->setContentsMargins(5, 0, 5, 0);
    titleLayout->addWidget(new QLabel(QStringLiteral("Search")));
    titleLayout->addStretch();
    QToolButton* dockCloseButton = new QToolButton();
    dockCloseButton->setText(QStringLiteral("X"));
    dockCloseButton->setObjectName(QStringLiteral("dockWidgetCloseButton"));
    connect(dockCloseButton, &QToolButton::clicked, m_searchDockWidget, &QDockWidget::close);
    titleLayout->addWidget(dockCloseButton);
    m_searchDockWidget->setTitleBarWidget(dockTitleBar);

    QWidget* searchWidget = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(searchWidget);
    mainLayout->setContentsMargins(4,4,4,4);
    mainLayout->setSpacing(4);

    QHBoxLayout* inputLayout = new QHBoxLayout;
    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText(QStringLiteral("Search document..."));
    inputLayout->addWidget(m_searchInput);

    QPushButton* prevButton = new QPushButton(QStringLiteral("\u25B2"));
    prevButton->setObjectName(QStringLiteral("searchNavButton"));
    prevButton->setToolTip(QStringLiteral("Previous result"));
    inputLayout->addWidget(prevButton);

    QPushButton* nextButton = new QPushButton(QStringLiteral("\u25BC"));
    nextButton->setObjectName(QStringLiteral("searchNavButton"));
    nextButton->setToolTip(QStringLiteral("Next result"));
    inputLayout->addWidget(nextButton);

    m_searchResultsList = new QListWidget;

    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(m_searchResultsList);
    m_searchDockWidget->setWidget(searchWidget);

    addDockWidget(Qt::RightDockWidgetArea, m_searchDockWidget);
    m_searchDockWidget->hide();

    connect(m_searchInput, &QLineEdit::returnPressed, this, [this](){ executeSearch(m_searchInput->text()); });
    connect(prevButton, &QPushButton::clicked, this, &MainWindow::findPrevSearchResult);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::findNextSearchResult);
    connect(m_searchResultsList, &QListWidget::itemClicked, this, &MainWindow::onSearchResultClicked);
    connect(m_searchResultsList, &QListWidget::currentItemChanged, this, &MainWindow::onSearchResultChanged);
}
