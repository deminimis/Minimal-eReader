#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QPushButton;

class FavoritesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FavoritesDialog(QStringList& favorites, QWidget* parent = nullptr);

private slots:
    void addFavorite();
    void removeFavorite();

private:
    void populateList();

    QListWidget* m_listWidget;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_closeButton;

    // A reference to the main application's list of favorites.
    // Changes made here will directly affect the main list.
    QStringList& m_favorites;
};
