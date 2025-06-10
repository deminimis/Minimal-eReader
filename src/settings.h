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
    QStringList recentFiles;
    QVariantList lastOpenTabs;
    QSize windowSize;
    QPoint windowPosition;
    bool isMaximized;
};
