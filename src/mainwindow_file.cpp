#include "mainwindow.h"
#include "viewerwidget.h"
#include "favoritesdialog.h"
#include "document.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QVariantMap>
#include <QTabWidget>
#include <QStatusBar>
#include <QMenu>
#include <QDir>
#include <QPushButton>

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

void MainWindow::openFile()
{
    const QString docFilter = "Documents (*.pdf *.epub *.xps *.cbz *.fb2);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Open Document", "", docFilter);
    if (!filePath.isEmpty()) {
        openFileFromPath(filePath);
    }
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
    populateToc();
    populateNotes();
}

void MainWindow::openRecentFile()
{
    if (auto *action = qobject_cast<QAction*>(sender())) {
        openFileFromPath(action->data().toString());
    }
}

void MainWindow::openFavoriteFile()
{
    if (auto *action = qobject_cast<QAction*>(sender())) {
        openFileFromPath(action->data().toString());
    }
}

void MainWindow::addCurrentFileToFavorites()
{
    const int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    const QString currentFile = m_documents.at(index)->getFilepath();
    if (!m_settings.favoriteFiles.contains(currentFile)) {
        m_settings.favoriteFiles.append(currentFile);
        updateFavoritesMenu();
    }
}

void MainWindow::manageFavorites()
{
    FavoritesDialog dialog(m_settings.favoriteFiles, this);
    dialog.exec();

    updateFavoritesMenu();
}

void MainWindow::setNotesDirectory()
{
    QMessageBox prompt(this);
    prompt.setWindowTitle("Set Notes Directory");
    prompt.setText("Choose where to save new notes.");
    prompt.setInformativeText("You can choose a custom folder, or reset to the default of saving notes alongside each book file.");
    QPushButton *customButton = prompt.addButton("Choose Custom Folder...", QMessageBox::ActionRole);
    QPushButton *defaultButton = prompt.addButton("Reset to Default", QMessageBox::ActionRole);
    prompt.addButton(QMessageBox::Cancel);
    prompt.exec();

    if (prompt.clickedButton() == customButton) {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Notes Directory", QDir::homePath());
        if (!dir.isEmpty()) {
            m_settings.notesDirectory = dir;
            m_settings.save();
        }
    } else if (prompt.clickedButton() == defaultButton) {
        m_settings.notesDirectory = "";
        m_settings.save();
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

void MainWindow::updateFavoritesMenu()
{
    m_favoritesMenu->clear();

    QAction *addFavAction = new QAction("Add Current to Favorites", this);
    connect(addFavAction, &QAction::triggered, this, &MainWindow::addCurrentFileToFavorites);

    const int currentIndex = m_tabWidget->currentIndex();
    bool canFavorite = false;
    if (currentIndex >= 0) {
        const QString currentFile = m_documents.at(currentIndex)->getFilepath();
        canFavorite = !m_settings.favoriteFiles.contains(currentFile);
    }
    addFavAction->setEnabled(canFavorite);
    m_favoritesMenu->addAction(addFavAction);

    QAction *manageFavAction = new QAction("Manage Favorites...", this);
    connect(manageFavAction, &QAction::triggered, this, &MainWindow::manageFavorites);
    m_favoritesMenu->addAction(manageFavAction);

    if (!m_settings.favoriteFiles.isEmpty()) {
        m_favoritesMenu->addSeparator();
        for (const QString &filePath : m_settings.favoriteFiles) {
            QAction *action = new QAction(QFileInfo(filePath).fileName(), this);
            action->setData(filePath);
            connect(action, &QAction::triggered, this, &MainWindow::openFavoriteFile);
            m_favoritesMenu->addAction(action);
        }
    }
}


void MainWindow::onTabChanged(int index)
{
    clearSelectionState();
    updateStatusBarActions();

    if (index >= 0) {
        renderActivePage();
        clearSearch();
        updateFavoritesMenu();
        populateToc();
        populateNotes();
    } else {
        m_statusBar->clearMessage();
        updateFavoritesMenu();
        populateToc();
        populateNotes();
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

void MainWindow::closeCurrentTab()
{
    if (m_tabWidget->count() > 0) {
        onTabCloseRequested(m_tabWidget->currentIndex());
    }
}

QString MainWindow::findNotesPathFor(const QString& bookPath) const
{
    const QFileInfo bookInfo(bookPath);
    const QString notesFileName = bookInfo.completeBaseName() + "_NOTES.txt";

    if (!m_settings.notesDirectory.isEmpty()) {
        const QString customPath = QDir(m_settings.notesDirectory).filePath(notesFileName);
        if (QFile::exists(customPath)) {
            return customPath;
        }
    }

    const QString adjacentPath = bookInfo.dir().filePath(notesFileName);
    if (QFile::exists(adjacentPath)) {
        return adjacentPath;
    }

    return QString();
}

QString MainWindow::getNewNotesPathFor(const QString& bookPath) const
{
    const QFileInfo bookInfo(bookPath);
    const QString notesFileName = bookInfo.completeBaseName() + "_NOTES.txt";

    if (!m_settings.notesDirectory.isEmpty()) {
        return QDir(m_settings.notesDirectory).filePath(notesFileName);
    } else {
        return bookInfo.dir().filePath(notesFileName);
    }
}
