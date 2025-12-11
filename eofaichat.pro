QT  += core
QT  += gui
QT  += widgets
QT  += concurrent
QT  += core5compat
QT  += network
QT  += dbus
QT  += opengl
QT  += svg
QT  += quick
QT  += quick3d
QT  += quick3dutils
QT  += quick3druntimerender
#QT  += quickshapes
QT  += qml
QT  += qmlmeta
QT  += qmlmodels
QT  += qmlworkerscript

greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TRANSLATIONS += \
    eofaichat_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(core/core.pri)
include(models/models.pri)
include(tokenizers/tokenizers.pri)
include(ui/ui.pri)

# eofaichat.pri
INCLUDEPATH += $$PWD/

SUBDIRS += \
    core \
    models \
    tokenizers \
    ui

# Main application source files
HEADERS += \
    $$PWD/main.cpp

SOURCES += \
    $$PWD/main.cpp

DISTFILES += \
    eofaichat.css \
    syntaxcolors.json

RESOURCES += \
    eofaichat.qrc
