QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Get git commit count and hash for versioning
GIT_COMMIT_COUNT = $$system(git rev-list --count HEAD)
GIT_COMMIT_HASH = $$system(git rev-parse --short HEAD)
VERSION = 0.$${GIT_COMMIT_COUNT}-$${GIT_COMMIT_HASH}
DEFINES += APP_VERSION_STRING=\\\"$$VERSION\\\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    MainWindow.cpp \
    JsonHighlighter.cpp \
    CodeEditor.cpp \
    settings.cpp \
    SaveCommandDialog.cpp

HEADERS += MainWindow.h \
    JsonHighlighter.h \
    CodeEditor.h \
    settings.h \
    SaveCommandDialog.h

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
   Quish.qrc

# Installation
desktop.path = /usr/share/applications
desktop.files = Quish.desktop
INSTALLS += desktop
