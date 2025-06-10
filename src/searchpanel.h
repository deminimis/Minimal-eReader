#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class SearchPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SearchPanel(QWidget *parent = nullptr);
    void focusInput();

signals:
    void searchTriggered(const QString& text);
    void findNext();
    void findPrev();
    void closePanel();

public slots:
    void updateResultsLabel(int current, int total);

private:
    QLineEdit* m_searchEdit;
    QPushButton* m_nextButton;
    QPushButton* m_prevButton;
    QPushButton* m_closeButton;
    QLabel* m_resultsLabel;
};
