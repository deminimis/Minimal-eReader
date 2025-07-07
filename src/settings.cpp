#include "settings.h"
#include <QSettings>
#include <QApplication>
#include <QScreen>
#include <QColor>
#include <QCoreApplication>

QRect defaultGeometry() {
    return QGuiApplication::primaryScreen()->availableGeometry();
}

void AppSettings::load()
{
    QString settingsPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    isStatusBarVisible = settings.value("UI/isStatusBarVisible", true).toBool();
    zoomFactor = settings.value("View/zoomFactor", 1.0).toReal();
    invertPageColors = settings.value("View/invertPageColors", false).toBool();
    isMaximized = settings.value("Window/isMaximized", false).toBool();
    windowSize = settings.value("Window/size", defaultGeometry().size() * 0.8).toSize();
    windowPosition = settings.value("Window/position", defaultGeometry().center() - QPoint(windowSize.width()/2, windowSize.height()/2)).toPoint();
    recentFiles = settings.value("Session/recentFiles").toStringList();
    lastOpenTabs = settings.value("Session/lastOpenTabs").toList();
    favoriteFiles = settings.value("Session/favoriteFiles").toStringList();
    notesDirectory = settings.value("General/notesDirectory", "").toString();
}

void AppSettings::save()
{
    QString settingsPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    settings.setValue("UI/isStatusBarVisible", isStatusBarVisible);
    settings.setValue("View/zoomFactor", zoomFactor);
    settings.setValue("View/invertPageColors", invertPageColors);
    settings.setValue("Session/recentFiles", recentFiles);
    settings.setValue("Session/lastOpenTabs", lastOpenTabs);
    settings.setValue("Session/favoriteFiles", favoriteFiles);
    settings.setValue("General/notesDirectory", notesDirectory);
    settings.setValue("Window/isMaximized", isMaximized);
    if (!isMaximized) {
        settings.setValue("Window/size", windowSize);
        settings.setValue("Window/position", windowPosition);
    }
}
