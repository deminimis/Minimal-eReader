#pragma once

#include <QLabel>
#include <QRect>
#include <QPoint>
#include <QVector>

class QMouseEvent;
class QPaintEvent;

class SelectionLabel : public QLabel
{
    Q_OBJECT

public:
    explicit SelectionLabel(QWidget* parent = nullptr);

    void clearSelection();
    void setCharRects(const QVector<QRectF>& charRects);
    bool hasSelection() const;

    // New methods for search highlighting
    void setSearchHighlight(const QRectF& rect);
    void clearSearchHighlight();

signals:
    void selectionMade(const QRect& selectionRect);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int charIndexAt(const QPoint& pos);
    void updateHighlightRects();

    QPoint m_origin;
    QPoint m_anchorPoint;
    bool m_isSelecting;

    QVector<QRectF> m_allCharRects;
    QVector<QRectF> m_highlightRects; // For user text selection
    QRectF m_searchHighlight;      // For search result highlight

    int m_startIndex;
    int m_endIndex;
};
