#include "mainwindow.h"
#include <QDockWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

// A recursive helper function to populate the QTreeWidget
void populateTree(QTreeWidgetItem* parentItem, const QVector<TocItem>& items)
{
    for (const TocItem& item : items) {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(parentItem);
        treeItem->setText(0, item.title);
        treeItem->setData(0, Qt::UserRole, item.pageNum);
        if (!item.children.isEmpty()) {
            populateTree(treeItem, item.children);
        }
    }
}
void populateTree(QTreeWidget* treeWidget, const QVector<TocItem>& items)
{
    for (const TocItem& item : items) {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem(treeWidget);
        treeItem->setText(0, item.title);
        treeItem->setData(0, Qt::UserRole, item.pageNum);
        if (!item.children.isEmpty()) {
            populateTree(treeItem, item.children);
        }
    }
}


void MainWindow::createTocDockWidget()
{
    m_tocDockWidget = new QDockWidget("Table of Contents", this);
    m_tocDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_tocTreeWidget = new QTreeWidget(m_tocDockWidget);
    m_tocTreeWidget->setHeaderHidden(true);
    m_tocDockWidget->setWidget(m_tocTreeWidget);

    addDockWidget(Qt::LeftDockWidgetArea, m_tocDockWidget);
    m_tocDockWidget->hide();

    connect(m_tocTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onTocItemClicked);
}

void MainWindow::populateToc()
{
    m_tocTreeWidget->clear();

    int index = m_tabWidget->currentIndex();
    if (index < 0) {
        m_tocAction->setEnabled(false);
        return;
    }

    Document* doc = m_documents.at(index);
    QVector<TocItem> toc = doc->getTableOfContents();

    if (toc.isEmpty()) {
        m_tocAction->setEnabled(false);
        m_tocDockWidget->hide();
    } else {
        m_tocAction->setEnabled(true);
        populateTree(m_tocTreeWidget, toc);
    }
}

void MainWindow::onTocItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;

    bool ok;
    int pageNum = item->data(0, Qt::UserRole).toInt(&ok);
    if (ok) {
        int index = m_tabWidget->currentIndex();
        if (index < 0) return;

        m_documents.at(index)->goToPage(pageNum);
        renderActivePage();
    }
}

void MainWindow::showTableOfContents()
{
    m_tocDockWidget->setVisible(!m_tocDockWidget->isVisible());
}
