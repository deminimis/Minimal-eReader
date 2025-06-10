#include "viewerwidget.h"
#include "selectionlabel.h"
#include <QPixmap>
#include <QScrollBar>

ViewerWidget::ViewerWidget(QWidget *parent) : QScrollArea(parent)
{
    m_imageLabel = new SelectionLabel;
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);

    setWidget(m_imageLabel);
    setBackgroundRole(QPalette::Dark);
    setAlignment(Qt::AlignCenter);
    setWidgetResizable(false);

    connect(m_imageLabel, &SelectionLabel::selectionMade, this, &ViewerWidget::textSelected);
}

void ViewerWidget::clearSelection()
{
    m_imageLabel->clearSelection();
}

void ViewerWidget::setCharRects(const QVector<QRectF>& charRects)
{
    m_imageLabel->setCharRects(charRects);
}

bool ViewerWidget::hasSelection() const
{
    return m_imageLabel->hasSelection();
}

void ViewerWidget::setPageImage(const QImage &image)
{
    if (image.isNull()) {
        m_imageLabel->clear();
        return;
    }
    m_imageLabel->setPixmap(QPixmap::fromImage(image));
    m_imageLabel->resize(image.size());
}

void ViewerWidget::scrollToTop()
{
    if (verticalScrollBar()) {
        verticalScrollBar()->setValue(verticalScrollBar()->minimum());
    }
}

void ViewerWidget::scrollToBottom()
{
    if (verticalScrollBar()) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
}

void ViewerWidget::setHighlight(const QRectF& highlightRect, qreal zoomFactor)
{
    // Scale the rectangle from page coordinates to zoomed-image coordinates
    QRectF scaledRect = QRectF(
        highlightRect.x() * zoomFactor,
        highlightRect.y() * zoomFactor,
        highlightRect.width() * zoomFactor,
        highlightRect.height() * zoomFactor
        );

    m_imageLabel->setSearchHighlight(scaledRect);
    ensureVisible(scaledRect.center().x(), scaledRect.center().y(), 100, 100);
}

void ViewerWidget::clearHighlight()
{
    m_imageLabel->clearSearchHighlight();
}
