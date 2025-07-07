#include "mainwindow.h"
#include "viewerwidget.h"
#include "document.h"

#include <QTabWidget>
#include <QStatusBar>
#include <QInputDialog>
#include <QWheelEvent>
#include <QToolButton>

void MainWindow::renderActivePage()
{
    clearSelectionState();
    if (auto* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->currentWidget())) {
        viewer->clearHighlight();
    }

    int index = m_tabWidget->currentIndex();
    if (index < 0) {
        updateStatusBarActions();
        return;
    }

    Document* doc = m_documents.at(index);
    int pageNum = doc->getCurrentPage();
    ViewerWidget* viewer = qobject_cast<ViewerWidget*>(m_tabWidget->widget(index));
    if (!viewer) return;

    QString cacheKey = QString("%1:%2:%3:%4")
                           .arg(doc->getFilepath())
                           .arg(pageNum)
                           .arg(m_settings.zoomFactor)
                           .arg(m_settings.invertPageColors);

    if (QImage* cachedImage = m_pageCache.object(cacheKey)) {
        viewer->setPageImage(*cachedImage);
        QVector<QRectF> charRects = doc->getPageCharRects(pageNum, m_settings.zoomFactor);
        viewer->setCharRects(charRects);
    } else {
        QImage image = doc->renderCurrentPage(m_settings.zoomFactor, m_settings.invertPageColors);
        if (!image.isNull()) {
            m_pageCache.insert(cacheKey, new QImage(image), image.sizeInBytes());
            viewer->setPageImage(image);
            QVector<QRectF> charRects = doc->getPageCharRects(pageNum, m_settings.zoomFactor);
            viewer->setCharRects(charRects);
        } else {
            viewer->setPageImage(QImage());
        }
    }

    updateStatusBar();
    updateStatusBarActions();
}

void MainWindow::updateStatusBarActions()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) {
        m_prevPageButton->setEnabled(false);
        m_nextPageButton->setEnabled(false);
        return;
    }

    Document* doc = m_documents.at(index);
    m_prevPageButton->setEnabled(doc->getCurrentPage() > 0);
    m_nextPageButton->setEnabled(doc->getCurrentPage() < doc->getPageCount() - 1);
}

void MainWindow::promptForZoomLevel()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;

    bool ok;
    int currentZoomInt = static_cast<int>(m_settings.zoomFactor * 100);
    int zoom = QInputDialog::getInt(this, "Set Zoom", "Enter zoom level (%):",
                                    currentZoomInt, 10, 1000, 1, &ok);

    if (ok && zoom != currentZoomInt) {
        m_settings.zoomFactor = zoom / 100.0;
        m_pageCache.clear();
        renderActivePage();
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
    m_pageCache.clear();
    renderActivePage();
}

void MainWindow::nextPage()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    m_documents.at(index)->goToNextPage();
    renderActivePage();
}

void MainWindow::prevPage()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    m_documents.at(index)->goToPrevPage();
    renderActivePage();
}

void MainWindow::zoomIn()
{
    m_settings.zoomFactor *= 1.1;
    m_pageCache.clear();
    renderActivePage();
}

void MainWindow::zoomOut()
{
    m_settings.zoomFactor /= 1.1;
    m_pageCache.clear();
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

    m_pageCache.clear();
    renderActivePage();
}

void MainWindow::promptForPageNumber()
{
    int index = m_tabWidget->currentIndex();
    if (index < 0) return;
    Document* doc = m_documents.at(index);

    bool ok;
    int page = QInputDialog::getInt(this, "Go to Page", "Enter page number:",
                                    doc->getCurrentPage() + 1,
                                    1,
                                    doc->getPageCount(),
                                    1, &ok);
    if (ok && (page - 1) != doc->getCurrentPage()) {
        doc->goToPage(page - 1);
        renderActivePage();
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
