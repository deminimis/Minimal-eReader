# ----------------------------------------------------
# Project Configuration
# ----------------------------------------------------
QT       += core gui widgets concurrent
TARGET   = Minimal-eReader
TEMPLATE = app
CONFIG   += c++17
CONFIG   += DWM_API

# ----------------------------------------------------
# Source Files
# ----------------------------------------------------
SOURCES += \
    main.cpp \
    settings.cpp \
    document.cpp \
    selectionlabel.cpp \
    viewerwidget.cpp \
    favoritesdialog.cpp \
    mainwindow.cpp \
    mainwindow_ui.cpp \
    mainwindow_events.cpp \
    mainwindow_actions.cpp \
    mainwindow_file.cpp \
    mainwindow_text.cpp \
    mainwindow_toc.cpp \
    mainwindow_notes.cpp

# ----------------------------------------------------
# Header Files
# ----------------------------------------------------
HEADERS += \
    settings.h \
    document.h \
    selectionlabel.h \
    viewerwidget.h \
    favoritesdialog.h \
    mainwindow.h

# ----------------------------------------------------
# Resource Files
# ----------------------------------------------------
RESOURCES += \
    resources.qrc

# ----------------------------------------------------
# Windows-Specific Settings
# ----------------------------------------------------
win32 {
    # Link the windows resource file for the application icon
    RC_FILE = win_resources.rc

    # Link the MuPDF library
    # Tell the compiler where to find MuPDF's .h header files
    INCLUDEPATH += $$PWD/libs/mupdf/include

    # Tell the linker where to find MuPDF's .lib library files and which ones to use
    LIBS += -L$$PWD/libs/mupdf/platform/win32/x64/Release/ \
            -llibmupdf \
            -llibthirdparty

    # Suppress specific warnings coming from the MuPDF library
    QMAKE_CXXFLAGS += /wd4100
    QMAKE_CXXFLAGS += /wd4702

    # Add a linker optimization flag
    QMAKE_LFLAGS += /OPT:REF
}
