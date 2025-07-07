#pragma once

#include <QMetaType>
#include <mupdf/fitz.h>
#include <QString>
#include <QImage>
#include <QSize>
#include <QRectF>
#include <QVector>

struct SearchResult {
    int pageNum;
    QRectF location;
    QString context;
};
Q_DECLARE_METATYPE(SearchResult)

struct TocItem {
    QString title;
    int pageNum;
    QVector<TocItem> children;
};

class Document {
public:
    explicit Document(fz_context* ctx, const QString& filepath);
    ~Document();
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) = delete;
    Document& operator=(Document&&) =delete;

    bool load();
    QImage renderCurrentPage(qreal zoomFactor, bool invertColors);
    void goToNextPage();
    void goToPrevPage();
    void goToPage(int page);
    int getCurrentPage() const;
    int getPageCount() const;
    QString getFilepath() const;

    QString getSelectedText(const QRectF& selectionRect, qreal zoomFactor) const;
    QVector<QRectF> getPageCharRects(int pageNum, qreal zoomFactor) const;
    QVector<SearchResult> searchDocument(const QString& text) const;
    QSizeF getOriginalPageSize(int pageNum) const;
    QVector<TocItem> getTableOfContents() const;

private:
    fz_context* m_ctx;
    fz_document* m_doc;
    QString m_filepath;
    int m_currentPage;
    int m_pageCount;
};
