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

void ViewerWidget::setHighlights(const QVector<QRectF>& allRects, const QRectF& currentRect, qreal zoomFactor)
{
    QVector<QRectF> scaledAllRects;
    scaledAllRects.reserve(allRects.size());
    for(const QRectF& rect : allRects) {
        scaledAllRects.append(QRectF(
            rect.x() * zoomFactor, rect.y() * zoomFactor,
            rect.width() * zoomFactor, rect.height() * zoomFactor
            ));
    }

    QRectF scaledCurrentRect = QRectF(
        currentRect.x() * zoomFactor,
        currentRect.y() * zoomFactor,
        currentRect.width() * zoomFactor,
        currentRect.height() * zoomFactor
        );

    m_imageLabel->setSearchHighlights(scaledAllRects, scaledCurrentRect);
    ensureVisible(scaledCurrentRect.center().x(), scaledCurrentRect.center().y(), 100, 100);
}

void ViewerWidget::clearHighlight()
{
    m_imageLabel->clearSearchHighlight();
}
