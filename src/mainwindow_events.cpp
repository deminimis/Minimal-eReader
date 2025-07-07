#include "mainwindow.h"
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QShowEvent>
#include <QVariantMap>
#include <QLabel>

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_isInitialShow) {
        m_isInitialShow = false;
        loadAppSettings();
        QTimer::singleShot(0, this, &MainWindow::restoreLastTabs);
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

        if (m_resizeEdge & Qt::LeftEdge) newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        if (m_resizeEdge & Qt::RightEdge) newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        if (m_resizeEdge & Qt::TopEdge) newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        if (m_resizeEdge & Qt::BottomEdge) newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());

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

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonDblClick && obj == m_customTitleBar) {
        isMaximized() ? showNormal() : showMaximized();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == m_pageLabel) {
            promptForPageNumber();
            return true;
        }
        if (obj == m_zoomLabel) {
            promptForZoomLevel();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->modifiers() == Qt::ControlModifier) {
            if (keyEvent->key() == Qt::Key_Minus) zoomOut();
            else if (keyEvent->key() == Qt::Key_Equal) zoomIn();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
