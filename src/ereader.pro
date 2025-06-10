# ---------------------------------------------------------------------------
# Qt Project File for Lightweight E-Reader
# ---------------------------------------------------------------------------

QT       += core gui widgets
CONFIG   += c++17
TARGET   = ereader
TEMPLATE = app
CONFIG   += DWM_API

# Define all source and header files for the project.
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    viewerwidget.cpp \
    document.cpp \
    settings.cpp \
    novelview.cpp \
    selectionlabel.cpp \
    searchpanel.cpp

HEADERS += \
    mainwindow.h \
    viewerwidget.h \
    document.h \
    settings.h \
    novelview.h \
    selectionlabel.h \
    searchpanel.h


RESOURCES += resources.qrc

win32 {
    RC_FILE += win_resources.rc
}

# ---------------------------------------------------------------------------
# External Library Linking
# ---------------------------------------------------------------------------
win32 {
    # Tell the compiler where to find the MuPDF header files.
    INCLUDEPATH += $$PWD/libs/mupdf/include

    # Tell the linker where to find the MuPDF library files (.lib).
    LIBS += -L$$PWD/libs/mupdf/platform/win32/x64/Release/ \
            -llibmupdf \
            -llibthirdparty

    # Suppress warnings from the external MuPDF library
    QMAKE_CXXFLAGS += /wd4100
    QMAKE_CXXFLAGS += /wd4702

    # Add linker optimization flag
    QMAKE_LFLAGS += /OPT:REF
}
