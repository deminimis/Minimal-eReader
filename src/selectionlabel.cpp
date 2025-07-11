#include "selectionlabel.h"
#include <QPen>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <algorithm>
#include <utility>

SelectionLabel::SelectionLabel(QWidget* parent)
    : QLabel(parent), m_isSelecting(false), m_startIndex(-1), m_endIndex(-1)
{
    setCursor(Qt::IBeamCursor);
}

void SelectionLabel::clearSelection()
{
    m_highlightRects.clear();
    m_startIndex = -1;
    m_endIndex = -1;
    update();
}

void SelectionLabel::setCharRects(const QVector<QRectF>& charRects)
{
    m_allCharRects = charRects;
}

bool SelectionLabel::hasSelection() const
{
    return !m_highlightRects.isEmpty();
}

int SelectionLabel::charIndexAt(const QPoint& pos)
{
    for (int i = 0; i < m_allCharRects.size(); ++i) {
        if (m_allCharRects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

void SelectionLabel::updateHighlightRects()
{
    m_highlightRects.clear();
    if (m_startIndex == -1 || m_endIndex == -1) {
        return;
    }

    int start = std::min(m_startIndex, m_endIndex);
    int end = std::max(m_startIndex, m_endIndex);

    for (int i = start; i <= end; ++i) {
        m_highlightRects.append(m_allCharRects[i]);
    }
    update();
}

void SelectionLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            m_isSelecting = true;
            m_endIndex = charIndexAt(event->pos());
            updateHighlightRects();
            mouseReleaseEvent(event);
        } else {
            m_isSelecting = true;
            m_anchorPoint = event->pos();
            m_startIndex = charIndexAt(event->pos());
            m_endIndex = m_startIndex;
            updateHighlightRects();
        }
    }
}

void SelectionLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isSelecting) {
        m_endIndex = charIndexAt(event->pos());
        updateHighlightRects();
    }
}

void SelectionLabel::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    if (m_isSelecting) {
        m_isSelecting = false;
        QRectF selectionBoundingRect;
        if (!m_highlightRects.isEmpty()) {
            for(const QRectF& rect : std::as_const(m_highlightRects)) {
                selectionBoundingRect = selectionBoundingRect.united(rect);
            }
        }
        emit selectionMade(selectionBoundingRect.toRect());
    }
}

void SelectionLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_searchHighlights.isEmpty()) {
        painter.setBrush(QColor(255, 255, 0, 70));
        painter.setPen(Qt::NoPen);
        for(const QRectF& rect : m_searchHighlights) {
            painter.drawRect(rect);
        }
    }

    if (!m_currentSearchHighlight.isNull()) {
        painter.setBrush(QColor(255, 140, 0, 90));
        painter.setPen(QPen(QColor(220, 20, 60), 2));
        painter.drawRect(m_currentSearchHighlight);
    }

    if (!m_highlightRects.isEmpty()) {
        painter.setBrush(QColor(0, 100, 255, 70));
        painter.setPen(Qt::NoPen);
        for(const QRectF& rect : m_highlightRects) {
            painter.drawRect(rect);
        }
    }
}

void SelectionLabel::setSearchHighlights(const QVector<QRectF>& allRects, const QRectF& currentRect)
{
    m_searchHighlights = allRects;
    m_currentSearchHighlight = currentRect;
    update();
}

void SelectionLabel::clearSearchHighlight()
{
    if (!m_searchHighlights.isEmpty() || !m_currentSearchHighlight.isNull()) {
        m_searchHighlights.clear();
        m_currentSearchHighlight = QRectF();
        update();
    }
}
