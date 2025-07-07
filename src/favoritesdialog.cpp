#include "favoritesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>

FavoritesDialog::FavoritesDialog(QStringList& favorites, QWidget* parent)
    : QDialog(parent), m_favorites(favorites)
{
    setWindowTitle("Manage Favorites");
    setMinimumSize(400, 300);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // List widget
    m_listWidget = new QListWidget(this);
    mainLayout->addWidget(m_listWidget);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Add...", this);
    m_removeButton = new QPushButton("Remove", this);
    m_closeButton = new QPushButton("Close", this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_addButton, &QPushButton::clicked, this, &FavoritesDialog::addFavorite);
    connect(m_removeButton, &QPushButton::clicked, this, &FavoritesDialog::removeFavorite);
    connect(m_closeButton, &QPushButton::clicked, this, &FavoritesDialog::accept);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this](){
        m_removeButton->setEnabled(!m_listWidget->selectedItems().isEmpty());
    });


    // Initial state
    populateList();
    m_removeButton->setEnabled(false);
}

void FavoritesDialog::populateList()
{
    m_listWidget->clear();
    for (const QString& fav : std::as_const(m_favorites)) {
        // Use the full path as the data, but show only the filename
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(fav).fileName());
        item->setData(Qt::UserRole, fav);
        m_listWidget->addItem(item);
    }
}

void FavoritesDialog::addFavorite()
{
    const QString docFilter = "Documents (*.pdf *.epub *.xps *.cbz *.fb2);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Add Favorite", "", docFilter);

    if (!filePath.isEmpty() && !m_favorites.contains(filePath)) {
        m_favorites.append(filePath);
        populateList(); // Refresh the list widget
    }
}

void FavoritesDialog::removeFavorite()
{
    QList<QListWidgetItem*> selectedItems = m_listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    // Get the full file path stored in the item's data
    QString filePathToRemove = selectedItems.first()->data(Qt::UserRole).toString();
    m_favorites.removeAll(filePathToRemove);

    // Remove from the visual list and re-populate
    populateList();
}
