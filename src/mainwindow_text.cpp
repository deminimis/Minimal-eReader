#include "mainwindow.h"
#include "viewerwidget.h"
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QInputDialog>
#include <QMenu>
#include <QListWidget>
#include <QTabWidget>
#include <QDebug>
#include <QDir>

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
    const int index = m_tabWidget->currentIndex();
    if (index < 0 || selectedText.trimmed().isEmpty()) return;

    Document* doc = m_documents.at(index);
    QString notesPath = findNotesPathFor(doc->getFilepath());
    if (notesPath.isEmpty()) {
        notesPath = getNewNotesPathFor(doc->getFilepath());
    }

    QFile notesFile(notesPath);
    if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Could not write to notes file."));
        return;
    }

    QRectF unscaledRect(m_lastSelectionRect.x() / m_settings.zoomFactor,
                        m_lastSelectionRect.y() / m_settings.zoomFactor,
                        m_lastSelectionRect.width() / m_settings.zoomFactor,
                        m_lastSelectionRect.height() / m_settings.zoomFactor);

    QTextStream out(&notesFile);
    QString location = QString("Page %1").arg(doc->getCurrentPage() + 1);
    QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString rectString = QString("[%1,%2,%3,%4]")
                             .arg(unscaledRect.x()).arg(unscaledRect.y())
                             .arg(unscaledRect.width()).arg(unscaledRect.height());

    out << "\n\n" << location << " (" << dateTime << ") " << rectString << "\n" << selectedText;

    clearSelectionState();
    populateNotes();
}

void MainWindow::saveComment(const QString& selectedText)
{
    const int index = m_tabWidget->currentIndex();
    if (index < 0 || selectedText.trimmed().isEmpty()) return;

    bool ok;
    QString commentText = QInputDialog::getText(this, QStringLiteral("Add Comment"), QStringLiteral("Enter your note:"), QLineEdit::Normal, QString(), &ok);

    if (ok && !commentText.isEmpty()) {
        Document* doc = m_documents.at(index);
        QString notesPath = findNotesPathFor(doc->getFilepath());
        if (notesPath.isEmpty()) {
            notesPath = getNewNotesPathFor(doc->getFilepath());
        }

        QFile notesFile(notesPath);
        if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Could not write to notes file."));
            return;
        }

        QRectF unscaledRect(m_lastSelectionRect.x() / m_settings.zoomFactor,
                            m_lastSelectionRect.y() / m_settings.zoomFactor,
                            m_lastSelectionRect.width() / m_settings.zoomFactor,
                            m_lastSelectionRect.height() / m_settings.zoomFactor);

        QTextStream out(&notesFile);
        QString location = QString("Page %1").arg(doc->getCurrentPage() + 1);
        QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        QString rectString = QString("[%1,%2,%3,%4]")
                                 .arg(unscaledRect.x()).arg(unscaledRect.y())
                                 .arg(unscaledRect.width()).arg(unscaledRect.height());

        out << "\n\n" << location << " (" << dateTime << ") " << rectString << "\n" << selectedText;
        out << "\n\nCOMMENT: " << commentText;
    }

    clearSelectionState();
    populateNotes();
}

void MainWindow::savePageNote()
{
    const int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    bool ok;
    QString commentText = QInputDialog::getText(this, QStringLiteral("Add Page Note"), QStringLiteral("Enter your note for this page:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !commentText.isEmpty()) {
        Document* doc = m_documents.at(index);
        QString notesPath = findNotesPathFor(doc->getFilepath());
        if (notesPath.isEmpty()) {
            notesPath = getNewNotesPathFor(doc->getFilepath());
        }

        QFile notesFile(notesPath);
        if (!notesFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Could not write to notes file."));
            return;
        }

        QTextStream out(&notesFile);
        const QString location = QStringLiteral("Page %1").arg(doc->getCurrentPage() + 1);
        const QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
        out << "\n\n" << location << " NOTE (" << dateTime << ")\n";
        out << commentText;
    }
    populateNotes();
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
            QAction* copyAct = contextMenu.addAction(QStringLiteral("Copy - ctrl + c"));
            QAction* highlightAct = contextMenu.addAction(QStringLiteral("Save Passage - ctrl + h"));
            QAction* commentAct = contextMenu.addAction(QStringLiteral("Save Comment - ctrl + j"));
            contextMenu.addSeparator();
            connect(copyAct, &QAction::triggered, this, [this, selectedText]{ copySelection(selectedText); });
            connect(highlightAct, &QAction::triggered, this, [this, selectedText]{ savePassage(selectedText); });
            connect(commentAct, &QAction::triggered, this, [this, selectedText]{ saveComment(selectedText); });
        }
    }

    QAction* noteAction = contextMenu.addAction(QStringLiteral("Save Page Note - ctrl + k"));
    connect(noteAction, &QAction::triggered, this, &MainWindow::savePageNote);

    contextMenu.exec(viewer->mapToGlobal(pos));
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
        m_searchResultsList->addItem(QStringLiteral("No results found."));
    } else {
        for (const SearchResult& result : results) {
            QListWidgetItem* item = new QListWidgetItem(
                QStringLiteral("Page %1: %2").arg(result.pageNum + 1).arg(result.context),
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

    SearchResult currentResult = data.value<SearchResult>();
    int tabIndex = m_tabWidget->currentIndex();
    if (tabIndex < 0) return;

    Document* doc = m_documents.at(tabIndex);
    ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(tabIndex));
    if (!doc || !viewer) return;

    auto highlightAction = [=] {
        QVector<QRectF> allHighlightsOnPage;
        for (int i = 0; i < m_searchResultsList->count(); ++i) {
            QListWidgetItem* listItem = m_searchResultsList->item(i);
            if (auto listData = listItem->data(Qt::UserRole); listData.canConvert<SearchResult>()) {
                SearchResult r = listData.value<SearchResult>();
                if (r.pageNum == currentResult.pageNum) {
                    allHighlightsOnPage.append(r.location);
                }
            }
        }
        viewer->setHighlights(allHighlightsOnPage, currentResult.location, m_settings.zoomFactor);
    };

    if (doc->getCurrentPage() != currentResult.pageNum) {
        doc->goToPage(currentResult.pageNum);
        renderActivePage();
    }

    highlightAction();
}

void MainWindow::onSearchResultChanged(QListWidgetItem* current, QListWidgetItem* /*previous*/)
{
    onSearchResultClicked(current);
}

void MainWindow::findNextSearchResult()
{
    int count = m_searchResultsList->count();
    if (count <= 1) return;

    int nextRow = m_searchResultsList->currentRow() + 1;
    if (nextRow >= count) {
        nextRow = 0;
    }
    m_searchResultsList->setCurrentRow(nextRow);
}

void MainWindow::findPrevSearchResult()
{
    int count = m_searchResultsList->count();
    if (count <= 1) return;

    int prevRow = m_searchResultsList->currentRow() - 1;
    if (prevRow < 0) {
        prevRow = count - 1;
    }
    m_searchResultsList->setCurrentRow(prevRow);
}

void MainWindow::clearSearch()
{
    if (m_searchResultsList) {
        m_searchResultsList->clear();
    }
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }
}
