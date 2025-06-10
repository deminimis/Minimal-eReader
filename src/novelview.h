#pragma once

#include <QWidget>

/**
 * @class NovelViewWidget
 * @brief A widget designed specifically for reflowable ebook formats.
 *
 * This widget will be responsible for laying out and drawing a "story"
 * (a stream of text and images from an EPUB, FB2, etc.) based on
 * user-configurable font sizes and view dimensions.
 */
class NovelViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NovelViewWidget(QWidget *parent = nullptr);
    ~NovelViewWidget();

protected:
    // We will use paintEvent to custom-draw the reflowed text.
    void paintEvent(QPaintEvent *event) override;

private:
    // In the future, this will hold the MuPDF story object.
};
