#include "mainwindow.h"
#include "viewerwidget.h"
#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextEdit>

struct Note {
    enum NoteType { Passage, Comment, PageNote };
    NoteType type;
    int pageNum = -1;
    QString dateTime;
    QString content;
    QString comment;
    QRectF location;
    qint64 filePos = -1;
    qint64 fileEndPos = -1;
};
Q_DECLARE_METATYPE(Note)

QVector<Note> MainWindow::parseNotesFile(const QString& notesPath) const
{
    QVector<Note> notes;
    QFile file(notesPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return notes;
    }

    QTextStream in(&file);
    const QString content = in.readAll();
    file.close();

    QRegularExpression headerRegex(R"(\n\nPage (\d+)( NOTE)? \((.*?)\)(?: \[([\d\.-]+),([\d\.-]+),([\d\.-]+),([\d\.-]+)\])?\n)");
    QRegularExpressionMatchIterator i = headerRegex.globalMatch(content);
    QVector<QRegularExpressionMatch> matches;
    while(i.hasNext()){
        matches.append(i.next());
    }

    for(int j = 0; j < matches.size(); ++j) {
        const QRegularExpressionMatch& match = matches[j];
        Note note;
        note.filePos = match.capturedStart();
        note.pageNum = match.captured(1).toInt();
        note.dateTime = match.captured(3);

        if (!match.captured(4).isNull()) {
            note.location = QRectF(match.captured(4).toDouble(), match.captured(5).toDouble(),
                                   match.captured(6).toDouble(), match.captured(7).toDouble());
        }

        qint64 contentStart = match.capturedEnd();
        qint64 contentEnd = (j + 1 < matches.size()) ? matches[j+1].capturedStart() : content.length();
        note.fileEndPos = contentEnd;

        QString noteBody = content.mid(contentStart, contentEnd - contentStart).trimmed();

        if (match.captured(2).contains("NOTE")) {
            note.type = Note::PageNote;
            note.content = noteBody;
        } else if (noteBody.contains("\n\nCOMMENT: ")) {
            note.type = Note::Comment;
            note.content = noteBody.section("\n\nCOMMENT: ", 0, 0);
            note.comment = noteBody.section("\n\nCOMMENT: ", 1, 1);
        } else {
            note.type = Note::Passage;
            note.content = noteBody;
        }
        notes.append(note);
    }
    return notes;
}

void MainWindow::createNotesDockWidget()
{
    m_notesDockWidget = new QDockWidget("Notes", this);
    m_notesDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* mainWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(mainWidget);

    m_notesListWidget = new QListWidget(m_notesDockWidget);
    layout->addWidget(m_notesListWidget);

    m_noteEditor = new QTextEdit(this);
    m_noteEditor->setPlaceholderText("Select a Page Note or Comment to view and edit its text here...");
    m_noteEditor->setVisible(false);
    layout->addWidget(m_noteEditor);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_saveNoteButton = new QPushButton("Save Changes", this);
    m_saveNoteButton->setVisible(false);
    QPushButton* deleteButton = new QPushButton("Delete Selected Note", this);
    buttonLayout->addWidget(m_saveNoteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(deleteButton);
    layout->addLayout(buttonLayout);


    m_notesDockWidget->setWidget(mainWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_notesDockWidget);
    m_notesDockWidget->hide();

    connect(m_notesListWidget, &QListWidget::itemClicked, this, &MainWindow::onNoteClicked);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedNote);
    connect(m_saveNoteButton, &QPushButton::clicked, this, &MainWindow::saveNoteChanges);
}

void MainWindow::populateNotes()
{
    m_notesListWidget->clear();
    m_noteEditor->setVisible(false);
    m_saveNoteButton->setVisible(false);
    m_currentNoteItem = nullptr;

    int index = m_tabWidget->currentIndex();
    if (index < 0) {
        m_notesAction->setEnabled(false);
        return;
    }

    QString bookPath = m_documents.at(index)->getFilepath();
    QString notesPath = findNotesPathFor(bookPath);

    if (notesPath.isEmpty() || !QFile::exists(notesPath)) {
        m_notesAction->setEnabled(false);
        m_notesDockWidget->hide();
        return;
    }

    QVector<Note> notes = parseNotesFile(notesPath);
    if (notes.isEmpty()) {
        m_notesAction->setEnabled(false);
        m_notesDockWidget->hide();
        return;
    }

    m_notesAction->setEnabled(true);
    for (const Note& note : notes) {
        QString itemText;
        switch (note.type) {
        case Note::Passage:  itemText = QString("Passage (Page %1)").arg(note.pageNum); break;
        case Note::Comment:  itemText = QString("Comment (Page %1)").arg(note.pageNum); break;
        case Note::PageNote: itemText = QString("Page Note (Page %1)").arg(note.pageNum); break;
        }
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setToolTip(note.content.left(200) + "...");
        item->setData(Qt::UserRole, QVariant::fromValue(note));
        m_notesListWidget->addItem(item);
    }
}

void MainWindow::onNoteClicked(QListWidgetItem* item)
{
    if (!item) return;

    m_currentNoteItem = item;
    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<Note>()) return;

    Note note = data.value<Note>();
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    Document* doc = m_documents.at(index);
    if (doc->getCurrentPage() != note.pageNum - 1) {
        doc->goToPage(note.pageNum - 1);
        renderActivePage();
    }

    auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget());
    if (!viewer) return;
    viewer->clearHighlight();

    if (!note.location.isNull()) {
        viewer->setHighlights({note.location}, note.location, m_settings.zoomFactor);
    }

    if (note.type == Note::Comment) {
        m_noteEditor->setText(note.comment);
        m_noteEditor->setVisible(true);
        m_saveNoteButton->setVisible(true);
    } else if (note.type == Note::PageNote) {
        m_noteEditor->setText(note.content);
        m_noteEditor->setVisible(true);
        m_saveNoteButton->setVisible(true);
    } else {
        m_noteEditor->clear();
        m_noteEditor->setVisible(false);
        m_saveNoteButton->setVisible(false);
    }
}

void MainWindow::deleteSelectedNote()
{
    if (!m_currentNoteItem) {
        QMessageBox::warning(this, "No Note Selected", "Please select a note from the list to delete.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete Note", "Are you sure you want to delete this note permanently?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }

    QVariant data = m_currentNoteItem->data(Qt::UserRole);
    if (data.canConvert<Note>()) {
        Note note = data.value<Note>();
        QString bookPath = m_documents.at(m_tabWidget->currentIndex())->getFilepath();
        QString notesPath = findNotesPathFor(bookPath);

        QFile file(notesPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();

            content.remove(note.filePos, note.fileEndPos - note.filePos);

            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream out(&file);
                out << content;
                file.close();
            }
        }
        populateNotes();
    }
}

void MainWindow::saveNoteChanges()
{
    if (!m_currentNoteItem) return;

    QVariant data = m_currentNoteItem->data(Qt::UserRole);
    if (!data.canConvert<Note>()) return;

    Note note = data.value<Note>();
    QString newText = m_noteEditor->toPlainText();

    QString newNoteBlock;
    if (note.type == Note::PageNote) {
        note.content = newText;
        newNoteBlock = QString("\n\nPage %1 NOTE (%2)\n%3")
                           .arg(note.pageNum).arg(note.dateTime).arg(note.content);
    } else if (note.type == Note::Comment) {
        note.comment = newText;
        QString rectString = QString("[%1,%2,%3,%4]")
                                 .arg(note.location.x()).arg(note.location.y())
                                 .arg(note.location.width()).arg(note.location.height());
        newNoteBlock = QString("\n\nPage %1 (%2) %3\n%4\n\nCOMMENT: %5")
                           .arg(note.pageNum).arg(note.dateTime).arg(rectString).arg(note.content).arg(note.comment);
    } else {
        return;
    }

    QString bookPath = m_documents.at(m_tabWidget->currentIndex())->getFilepath();
    QString notesPath = findNotesPathFor(bookPath);

    QFile file(notesPath);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QString content = file.readAll();
        content.replace(note.filePos, note.fileEndPos - note.filePos, newNoteBlock);
        file.resize(0);
        QTextStream out(&file);
        out << content;
        file.close();
    }
    populateNotes();
}

void MainWindow::showNotes()
{
    m_notesDockWidget->setVisible(!m_notesDockWidget->isVisible());
}
