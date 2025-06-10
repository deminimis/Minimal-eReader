#pragma once

#include <QMainWindow>
#include <QList>
#include <QRect>
#include <QTimer>
#include <QDockWidget>
#include <QListWidget>

#include "settings.h"
#include "document.h" // Includes SearchResult struct
#include <mupdf/fitz.h>

class QTabWidget;
class QStatusBar;
class QMenu;
class QPoint;
class QToolButton;
class QWheelEvent;
class QCloseEvent;
class QMouseEvent;
class QShowEvent;
class QLabel;
class QLineEdit; // Forward declaration

class ViewerWidget;
class SearchPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void openFile();
    void openRecentFile();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void closeCurrentTab();
    void toggleStatusBar();
    void invertPageColors();
    void renderActivePage();
    void nextPage();
    void prevPage();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void promptForPageNumber();
    void toggleFullScreen();

    void onTextSelected(const QRect& rect);
    void copySelection(const QString& selectedText);
    void savePassage(const QString& selectedText);
    void saveComment(const QString& selectedText);
    void showPageContextMenu(const QPoint& pos);
    void savePageNote();

    void onSavePassageShortcut();
    void onSaveCommentShortcut();
    void onSavePageNoteShortcut();
    void restoreLastTabs();

    // New search system slots
    void executeSearch(const QString& text);
    void onSearchResultClicked(QListWidgetItem* item);
    void onSearchResultChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void findNextSearchResult();
    void findPrevSearchResult();
    void clearSearch();

private:
    void setupUI();
    void createActions();
    void createCustomTitleBar();
    void createSearchDockWidget();
    void setupTheme();
    void updateStatusBar();
    void loadAppSettings();
    void openFileFromPath(const QString &filePath, int pageNum = 0);
    void updateRecentFilesMenu();
    void clearSelectionState();
    void updateResizeCursor(const QPoint& pos);

    QWidget* m_customTitleBar;
    QToolButton* m_menuButton;
    QMenu* m_mainMenu;
    QMenu* m_recentFilesMenu;
    QTabWidget* m_tabWidget;
    QStatusBar* m_statusBar;
    QLabel* m_pageLabel;
    QLabel* m_zoomLabel;

    // Search UI Elements
    QDockWidget* m_searchDockWidget;
    QListWidget* m_searchResultsList;
    QLineEdit* m_searchInput;

    QAction* m_openAction;
    QAction* m_copyAction;
    QAction* m_searchAction;
    QAction* m_goToPageAction;
    QAction* m_invertColorsAction;
    QAction* m_toggleStatusBarAction;
    QAction* m_exitAction;

    QList<Document*> m_documents;
    AppSettings m_settings;
    QRect m_lastSelectionRect;
    QRect m_resizeStartGeometry;

    bool m_isDragging;
    QPoint m_dragStartPosition;
    bool m_isResizing;
    QFlags<Qt::Edge> m_resizeEdge;
    bool m_isInitialShow;

    fz_context* m_mupdfContext;
};
