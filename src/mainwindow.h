#pragma once

#include <QMainWindow>
#include <QList>
#include <QRect>
#include <QTimer>
#include <QDockWidget>
#include <QListWidget>
#include <QCache>
#include <QImage>

#include "settings.h"
#include "document.h"
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
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidgetItem;
class QTextEdit;
class QPushButton;

struct Note;

class ViewerWidget;

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
    void openFavoriteFile();
    void addCurrentFileToFavorites();
    void manageFavorites();
    void showTableOfContents();
    void onTocItemClicked(QTreeWidgetItem* item, int column);
    void setNotesDirectory();
    void showNotes();
    void onNoteClicked(QListWidgetItem* item);
    void deleteSelectedNote();
    void saveNoteChanges();
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
    void promptForZoomLevel();
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
    void createTocDockWidget();
    void createNotesDockWidget();
    void populateToc();
    void populateNotes();
    QVector<Note> parseNotesFile(const QString& notesPath) const;
    void setupTheme();
    void updateStatusBar();
    void updateStatusBarActions();
    void loadAppSettings();
    void openFileFromPath(const QString &filePath, int pageNum = 0);
    void updateRecentFilesMenu();
    void updateFavoritesMenu();
    void clearSelectionState();
    void updateResizeCursor(const QPoint& pos);
    QString findNotesPathFor(const QString& bookPath) const;
    QString getNewNotesPathFor(const QString& bookPath) const;

    QWidget* m_customTitleBar;
    QToolButton* m_menuButton;
    QMenu* m_mainMenu;
    QMenu* m_recentFilesMenu;
    QMenu* m_favoritesMenu;
    QTabWidget* m_tabWidget;
    QStatusBar* m_statusBar;
    QLabel* m_pageLabel;
    QLabel* m_zoomLabel;
    QToolButton* m_prevPageButton;
    QToolButton* m_nextPageButton;


    QDockWidget* m_searchDockWidget;
    QListWidget* m_searchResultsList;
    QLineEdit* m_searchInput;
    QDockWidget* m_tocDockWidget;
    QTreeWidget* m_tocTreeWidget;
    QDockWidget* m_notesDockWidget;
    QListWidget* m_notesListWidget;
    QTextEdit* m_noteEditor;
    QPushButton* m_saveNoteButton;
    QListWidgetItem* m_currentNoteItem;

    QAction* m_openAction;
    QAction* m_copyAction;
    QAction* m_searchAction;
    QAction* m_goToPageAction;
    QAction* m_invertColorsAction;
    QAction* m_toggleStatusBarAction;
    QAction* m_tocAction;
    QAction* m_notesAction;
    QAction* m_exitAction;

    QList<Document*> m_documents;
    AppSettings m_settings;
    QCache<QString, QImage> m_pageCache;
    QRect m_lastSelectionRect;
    QRect m_resizeStartGeometry;

    bool m_isDragging;
    QPoint m_dragStartPosition;
    bool m_isResizing;
    QFlags<Qt::Edge> m_resizeEdge;
    bool m_isInitialShow;

    fz_context* m_mupdfContext;
};
