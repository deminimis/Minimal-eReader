#pragma once

#include <QColor>
#include <QString>
#include <QSize>
#include <QPoint>
#include <QStringList>
#include <QVariantList>

class AppSettings
{
public:
    void load();
    void save();

    bool isStatusBarVisible;
    qreal zoomFactor;
    bool invertPageColors;

    QSize windowSize;
    QPoint windowPosition;
    bool isMaximized;

    QStringList recentFiles;
    QVariantList lastOpenTabs;
    QStringList favoriteFiles;
    QString notesDirectory;
};
