#include "searchpanel.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStyle>

SearchPanel::SearchPanel(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search...");
    layout->addWidget(m_searchEdit);

    m_prevButton = new QPushButton("Previous", this);
    layout->addWidget(m_prevButton);

    m_nextButton = new QPushButton("Next", this);
    layout->addWidget(m_nextButton);

    m_resultsLabel = new QLabel(this);
    m_resultsLabel->setMinimumWidth(80);
    layout->addWidget(m_resultsLabel);

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName("searchPanelCloseButton");
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    layout->addWidget(m_closeButton);

    setLayout(layout);

    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this](){
        emit searchTriggered(m_searchEdit->text());
    });
    connect(m_nextButton, &QPushButton::clicked, this, &SearchPanel::findNext);
    connect(m_prevButton, &QPushButton::clicked, this, &SearchPanel::findPrev);
    connect(m_closeButton, &QPushButton::clicked, this, &SearchPanel::closePanel);
}

void SearchPanel::focusInput()
{
    m_searchEdit->selectAll();
    m_searchEdit->setFocus();
}

void SearchPanel::updateResultsLabel(int current, int total)
{
    if (total > 0) {
        m_resultsLabel->setText(QString("%1 of %2").arg(current).arg(total));
    } else {
        m_resultsLabel->setText("Not found");
    }
}
