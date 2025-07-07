#include "document.h"
#include <algorithm>
#include <QDebug>
#include <QPainter>

void traverseOutline(fz_context* ctx, fz_document* doc, fz_outline* outline, QVector<TocItem>& items)
{
    for (fz_outline* entry = outline; entry; entry = entry->next) {
        int pageNum = -1;
        if (entry->uri) {
            fz_location loc = fz_resolve_link(ctx, doc, entry->uri, nullptr, nullptr);
            pageNum = fz_page_number_from_location(ctx, doc, loc);
        }

        if (pageNum != -1) {
            TocItem tocItem;
            tocItem.title = QString::fromUtf8(entry->title ? entry->title : "");
            tocItem.pageNum = pageNum;

            if (entry->down) {
                traverseOutline(ctx, doc, entry->down, tocItem.children);
            }
            items.append(tocItem);
        }
    }
}

Document::Document(fz_context* ctx, const QString& filepath)
    : m_ctx(ctx),
    m_doc(nullptr),
    m_filepath(filepath),
    m_currentPage(0),
    m_pageCount(0)
{
}

Document::~Document() {
    if (m_doc) fz_drop_document(m_ctx, m_doc);
}

bool Document::load() {
    if (!m_ctx || m_filepath.isEmpty()) return false;
    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, m_filepath.toStdString().c_str());
        m_pageCount = fz_count_pages(m_ctx, m_doc);
    } fz_catch(m_ctx) {
        qWarning() << "Failed to load document:" << fz_caught_message(m_ctx);
        return false;
    }
    return true;
}

QImage Document::renderCurrentPage(qreal zoomFactor, bool invertColors) {
    if (!m_doc || m_currentPage < 0 || m_currentPage >= m_pageCount) return QImage();
    QImage renderedImage;
    fz_page* page = nullptr;
    fz_pixmap* pixmap = nullptr;
    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, m_currentPage);
        fz_matrix ctm = fz_scale(zoomFactor, zoomFactor);
        pixmap = fz_new_pixmap_from_page(m_ctx, page, ctm, fz_device_rgb(m_ctx), 0);
        if (invertColors) {
            fz_try(m_ctx) {
                fz_invert_pixmap(m_ctx, pixmap);
            } fz_catch(m_ctx) {
                qWarning() << "Failed to invert pixmap colors";
            }
        }
        renderedImage = QImage(pixmap->samples, pixmap->w, pixmap->h, pixmap->stride, QImage::Format_RGB888).copy();
    } fz_catch(m_ctx) {
        qWarning() << "Error rendering page" << m_currentPage << ":" << fz_caught_message(m_ctx);
        renderedImage = QImage();
    }
    if (pixmap) fz_drop_pixmap(m_ctx, pixmap);
    if (page) fz_drop_page(m_ctx, page);
    return renderedImage;
}

QVector<QRectF> Document::getPageCharRects(int pageNum, qreal zoomFactor) const
{
    QVector<QRectF> allRects;
    if (!m_doc || pageNum < 0 || pageNum >= m_pageCount) return allRects;

    fz_page* page = nullptr;
    fz_stext_page* stext_page = nullptr;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, pageNum);
        stext_page = fz_new_stext_page_from_page(m_ctx, page, NULL);
        fz_matrix ctm = fz_scale(zoomFactor, zoomFactor);

        for (fz_stext_block* block = stext_page->first_block; block; block = block->next) {
            if (block->type == FZ_STEXT_BLOCK_TEXT) {
                for (fz_stext_line* line = block->u.t.first_line; line; line = line->next) {
                    for (fz_stext_char* ch = line->first_char; ch; ch = ch->next) {
                        fz_rect char_rect = fz_rect_from_quad(ch->quad);
                        char_rect = fz_transform_rect(char_rect, ctm);
                        allRects.append(QRectF(char_rect.x0, char_rect.y0, char_rect.x1 - char_rect.x0, char_rect.y1 - char_rect.y0));
                    }
                }
            }
        }
    } fz_catch(m_ctx) {
        qWarning() << "Failed to extract char rects:" << fz_caught_message(m_ctx);
    }

    if (stext_page) fz_drop_stext_page(m_ctx, stext_page);
    if (page) fz_drop_page(m_ctx, page);

    return allRects;
}

QSizeF Document::getOriginalPageSize(int pageNum) const
{
    if (!m_doc || pageNum < 0 || pageNum >= m_pageCount) return QSizeF();

    fz_page* page = nullptr;
    QSizeF size;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, pageNum);
        fz_rect bounds = fz_bound_page(m_ctx, page);
        size = QSizeF(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);
    } fz_catch(m_ctx) {
        qWarning() << "Failed to get page bounds:" << fz_caught_message(m_ctx);
    }

    if (page) fz_drop_page(m_ctx, page);
    return size;
}

QString Document::getSelectedText(const QRectF& selectionRect, qreal zoomFactor) const
{
    if (!m_doc || m_currentPage < 0 || m_currentPage >= m_pageCount) return QString();

    QString selectedTextStr;
    fz_page* page = nullptr;
    fz_stext_page* stext_page = nullptr;
    char* text = nullptr;

    fz_try(m_ctx)
    {
        page = fz_load_page(m_ctx, m_doc, m_currentPage);
        stext_page = fz_new_stext_page_from_page(m_ctx, page, nullptr);

        fz_matrix scale_matrix = fz_scale(zoomFactor, zoomFactor);
        fz_matrix inverse_matrix = fz_invert_matrix(scale_matrix);

        fz_point p0 = { (float)selectionRect.left(), (float)selectionRect.top() };
        fz_point p1 = { (float)selectionRect.right(), (float)selectionRect.bottom() };

        p0 = fz_transform_point(p0, inverse_matrix);
        p1 = fz_transform_point(p1, inverse_matrix);

        float x0 = std::min(p0.x, p1.x);
        float y0 = std::min(p0.y, p1.y);
        float x1 = std::max(p0.x, p1.x);
        float y1 = std::max(p0.y, p1.y);

        fz_point a = { x0, y0 };
        fz_point b = { x1, y1 };

        text = fz_copy_selection(m_ctx, stext_page, a, b, 0);
        selectedTextStr = QString::fromUtf8(text);
    }
    fz_catch(m_ctx)
    {
        qWarning() << "Failed to get selected text:" << fz_caught_message(m_ctx);
        selectedTextStr = QString();
    }

    if (text) fz_free(m_ctx, text);
    if (stext_page) fz_drop_stext_page(m_ctx, stext_page);
    if (page) fz_drop_page(m_ctx, page);

    return selectedTextStr;
}

void Document::goToNextPage() {
    if (m_currentPage < m_pageCount - 1) m_currentPage++;
}

void Document::goToPrevPage() {
    if (m_currentPage > 0) m_currentPage--;
}

void Document::goToPage(int page) {
    if (page >= 0 && page < m_pageCount) m_currentPage = page;
}

QVector<SearchResult> Document::searchDocument(const QString& text) const
{
    QVector<SearchResult> allResults;
    if (!m_doc || text.isEmpty()) return allResults;

    const QString searchTerm = text.simplified();
    if (searchTerm.isEmpty()) return allResults;

    for (int i = 0; i < m_pageCount; ++i) {
        fz_page* page = nullptr;
        fz_stext_page* stext_page = nullptr;
        fz_try(m_ctx) {
            page = fz_load_page(m_ctx, m_doc, i);
            stext_page = fz_new_stext_page_from_page(m_ctx, page, nullptr);
            if (!stext_page) continue;

            QString pageText;
            QVector<fz_rect> charRects;

            for (fz_stext_block* block = stext_page->first_block; block; block = block->next) {
                if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
                for (fz_stext_line* line = block->u.t.first_line; line; line = line->next) {
                    for (fz_stext_char* ch = line->first_char; ch; ch = ch->next) {
                        pageText.append(QChar(ch->c));
                        charRects.append(fz_rect_from_quad(ch->quad));
                    }
                    pageText.append(' ');
                    charRects.append(fz_empty_rect);
                }
            }

            int from = 0;
            while ((from = pageText.indexOf(searchTerm, from, Qt::CaseInsensitive)) != -1) {
                int matchEnd = from + searchTerm.length();

                fz_rect combined_rect = fz_empty_rect;
                for (int j = from; j < matchEnd; ++j) {
                    if (!fz_is_empty_rect(charRects[j])) {
                        combined_rect = fz_is_empty_rect(combined_rect) ? charRects[j] : fz_union_rect(combined_rect, charRects[j]);
                    }
                }

                if (!fz_is_empty_rect(combined_rect)) {
                    int contextStart = std::max(0, from - 20);
                    int contextEnd = std::min((int)pageText.length(), matchEnd + 20);
                    QString context = pageText.mid(contextStart, contextEnd - contextStart).replace('\n', ' ').simplified();

                    SearchResult result;
                    result.pageNum = i;
                    result.context = "..." + context + "...";
                    result.location = QRectF(combined_rect.x0, combined_rect.y0, combined_rect.x1 - combined_rect.x0, combined_rect.y1 - combined_rect.y0);
                    allResults.append(result);
                }
                from = matchEnd;
            }
        } fz_catch(m_ctx) {
        }
        if (stext_page) fz_drop_stext_page(m_ctx, stext_page);
        if (page) fz_drop_page(m_ctx, page);
    }

    return allResults;
}

QVector<TocItem> Document::getTableOfContents() const
{
    QVector<TocItem> toc;
    if (!m_doc) return toc;

    fz_outline *outline = nullptr;
    fz_try(m_ctx) {
        outline = fz_load_outline(m_ctx, m_doc);
        if (outline) {
            traverseOutline(m_ctx, m_doc, outline, toc);
        }
    } fz_catch(m_ctx) {
        qWarning() << "Failed to load table of contents:" << fz_caught_message(m_ctx);
    }

    if (outline) {
        fz_drop_outline(m_ctx, outline);
    }
    return toc;
}


int Document::getCurrentPage() const { return m_currentPage; }
int Document::getPageCount() const { return m_pageCount; }
QString Document::getFilepath() const { return m_filepath; }
