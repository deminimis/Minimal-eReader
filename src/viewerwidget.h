#pragma once

#include <QScrollArea>
#include <QImage>
#include <QVector>
#include <QRectF>

class SelectionLabel;

class ViewerWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget *parent = nullptr);
    void setPageImage(const QImage &image);
    void clearSelection();
    void setCharRects(const QVector<QRectF>& charRects);
    void scrollToTop();
    void scrollToBottom();
    bool hasSelection() const;

    void setHighlights(const QVector<QRectF>& allRects, const QRectF& currentRect, qreal zoomFactor);
    void clearHighlight();

signals:
    void textSelected(const QRect& rect);

private:
    SelectionLabel *m_imageLabel;
};
