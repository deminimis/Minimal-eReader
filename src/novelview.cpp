#include "novelview.h"

#include <QPainter>
#include <QDebug>

NovelViewWidget::NovelViewWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set a solid background color.
    setAutoFillBackground(true);
}

NovelViewWidget::~NovelViewWidget()
{
    // Destructor for future use.
}

void NovelViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    // --- Placeholder Content ---
    // For now, we will just draw some text to show that this widget is working.
    // In the next steps, we will replace this with actual document rendering.
    painter.setPen(Qt::white);
    painter.setFont(QFont("Segoe UI", 14));
    painter.drawText(rect(), Qt::AlignCenter, "Novel View (for EPUB, etc.)");
}
