# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = QSanguosha
QT += network sql declarative
TEMPLATE = app
CONFIG += audio

# choose luajit if you like it, the default is to use lua.
CONFIG += lua


SOURCES += \
    src/main.cpp \
    src/client/aux-skills.cpp \
    src/client/client.cpp \
    src/client/clientplayer.cpp \
    src/client/clientstruct.cpp \
    src/core/card.cpp \
    src/core/engine.cpp \
    src/core/general.cpp \
    src/core/jsonutils.cpp \
    src/core/lua-wrapper.cpp \
    src/core/player.cpp \
    src/core/protocol.cpp \
    src/core/record-analysis.cpp \
    src/core/RoomState.cpp \
    src/core/settings.cpp \
    src/core/skill.cpp \
    src/core/structs.cpp \
    src/core/util.cpp \
    src/core/WrappedCard.cpp \
    src/dialog/AboutUs.cpp \
    src/dialog/cardeditor.cpp \
    src/dialog/cardoverview.cpp \
    src/dialog/choosegeneraldialog.cpp \
    src/dialog/configdialog.cpp \
    src/dialog/connectiondialog.cpp \
    src/dialog/customassigndialog.cpp \
    src/dialog/distanceviewdialog.cpp \
    src/dialog/generaloverview.cpp \
    src/dialog/mainwindow.cpp \
    src/dialog/packagingeditor.cpp \
    src/dialog/playercarddialog.cpp \
    src/dialog/rule-summary.cpp \
    src/package/exppattern.cpp \
    src/package/formation.cpp \
    src/package/momentum.cpp \
    src/package/standard.cpp \
    src/package/standard-basics.cpp \
    src/package/standard-equips.cpp \
    src/package/standard-tricks.cpp \
    src/package/standard-qun-generals.cpp \
    src/package/standard-shu-generals.cpp \
    src/package/standard-wu-generals.cpp \
    src/package/standard-wei-generals.cpp \
    src/package/standard-package.cpp \
    src/package/package.cpp \
    src/scenario/miniscenarios.cpp \
    src/scenario/scenario.cpp \
    src/scenario/scenerule.cpp \
    src/server/ai.cpp \
    src/server/gamerule.cpp \
    src/server/generalselector.cpp \
    src/server/room.cpp \
    src/server/roomthread.cpp \
    src/server/server.cpp \
    src/server/serverplayer.cpp \
    src/ui/button.cpp \
    src/ui/cardcontainer.cpp \
    src/ui/carditem.cpp \
    src/ui/chatwidget.cpp \
    src/ui/choosegeneralbox.cpp \
    src/ui/clientlogbox.cpp \
    src/ui/dashboard.cpp \
    src/ui/GenericCardContainerUI.cpp \
    src/ui/indicatoritem.cpp \
    src/ui/magatamasItem.cpp \
    src/ui/photo.cpp \
    src/ui/pixmapanimation.cpp \
    src/ui/qsanbutton.cpp \
    src/ui/QSanSelectableItem.cpp \
    src/ui/rolecombobox.cpp \
    src/ui/roomscene.cpp \
    src/ui/SkinBank.cpp \
    src/ui/sprite.cpp \
    src/ui/startscene.cpp \
    src/ui/TablePile.cpp \
    src/ui/TimedProgressBar.cpp \
    src/ui/uiUtils.cpp \
    src/ui/window.cpp \
    src/util/detector.cpp \
    src/util/nativesocket.cpp \
    src/util/recorder.cpp \
    src/jsoncpp/src/json_writer.cpp \
    src/jsoncpp/src/json_valueiterator.inl \
    src/jsoncpp/src/json_value.cpp \
    src/jsoncpp/src/json_reader.cpp \
    src/jsoncpp/src/json_internalmap.inl \
    src/jsoncpp/src/json_internalarray.inl \
    swig/sanguosha_wrap.cxx
HEADERS += \
    src/client/aux-skills.h \
    src/client/client.h \
    src/client/clientplayer.h \
    src/client/clientstruct.h \
    src/core/audio.h \
    src/core/card.h \
    src/core/compiler-specific.h \
    src/core/engine.h \
    src/core/general.h \
    src/core/jsonutils.h \
    src/core/lua-wrapper.h \
    src/core/namespace.h \
    src/core/player.h \
    src/core/protocol.h \
    src/core/record-analysis.h \
    src/core/RoomState.h \
    src/core/settings.h \
    src/core/skill.h \
    src/core/structs.h \
    src/core/util.h \
    src/core/WrappedCard.h \
    src/dialog/AboutUs.h \
    src/dialog/cardeditor.h \
    src/dialog/cardoverview.h \
    src/dialog/choosegeneraldialog.h \
    src/dialog/configdialog.h \
    src/dialog/connectiondialog.h \
    src/dialog/customassigndialog.h \
    src/dialog/distanceviewdialog.h \
    src/dialog/generaloverview.h \
    src/dialog/mainwindow.h \
    src/dialog/packagingeditor.h \
    src/dialog/playercarddialog.h \
    src/dialog/rule-summary.h \
    src/package/exppattern.h \
    src/package/formation.h \
    src/package/momentum.h \
    src/package/package.h \
    src/package/standard.h \
    src/package/standard-basics.h \
    src/package/standard-equips.h \
    src/package/standard-tricks.h \
    src/package/standard-qun-generals.h \
    src/package/standard-shu-generals.h \
    src/package/standard-wu-generals.h \
    src/package/standard-wei-generals.h \
    src/package/standard-package.h \
    src/scenario/miniscenarios.h \
    src/scenario/scenario.h \
    src/scenario/scenerule.h \
    src/server/ai.h \
    src/server/gamerule.h \
    src/server/generalselector.h \
    src/server/room.h \
    src/server/roomthread.h \
    src/server/server.h \
    src/server/serverplayer.h \
    src/ui/button.h \
    src/ui/cardcontainer.h \
    src/ui/carditem.h \
    src/ui/chatwidget.h \
    src/ui/choosegeneralbox.h \
    src/ui/clientlogbox.h \
    src/ui/dashboard.h \
    src/ui/GenericCardContainerUI.h \
    src/ui/indicatoritem.h \
    src/ui/magatamasItem.h \
    src/ui/photo.h \
    src/ui/pixmapanimation.h \
    src/ui/qsanbutton.h \
    src/ui/QSanSelectableItem.h \
    src/ui/rolecombobox.h \
    src/ui/roomscene.h \
    src/ui/SkinBank.h \
    src/ui/sprite.h \
    src/ui/startscene.h \
    src/ui/TablePile.h \
    src/ui/TimedProgressBar.h \
    src/ui/uiUtils.h \
    src/ui/window.h \
    src/util/detector.h \
    src/util/nativesocket.h \
    src/util/recorder.h \
    src/util/socket.h \
    src/jsoncpp/src/json_tool.h \
    src/jsoncpp/src/json_batchallocator.h \
    src/jsoncpp/include/json/writer.h \
    src/jsoncpp/include/json/value.h \
    src/jsoncpp/include/json/reader.h \
    src/jsoncpp/include/json/json.h \
    src/jsoncpp/include/json/forwards.h \
    src/jsoncpp/include/json/features.h \
    src/jsoncpp/include/json/config.h \
    src/jsoncpp/include/json/autolink.h \
    src/jsoncpp/include/json/assertions.h

FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/connectiondialog.ui \
    src/dialog/generaloverview.ui \
    src/dialog/mainwindow.ui

INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/ui
INCLUDEPATH += src/util
INCLUDEPATH += src/jsoncpp/include

win32{
    RC_FILE += resource/icon.rc
}

macx{
    ICON = resource/icon/sgs.icns
}


LIBS += -L.
LIBS += -L"$$_PRO_FILE_PWD_/lib"
win32{
    !contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win32"
    } else {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win64"
    }
}

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/fmod
    LIBS += -lfmodex
    SOURCES += src/core/audio.cpp
}

CONFIG(lua){
    SOURCES += \
        src/lua/lzio.c \
        src/lua/lvm.c \
        src/lua/lundump.c \
        src/lua/ltm.c \
        src/lua/ltablib.c \
        src/lua/ltable.c \
        src/lua/lstrlib.c \
        src/lua/lstring.c \
        src/lua/lstate.c \
        src/lua/lparser.c \
        src/lua/loslib.c \
        src/lua/lopcodes.c \
        src/lua/lobject.c \
        src/lua/loadlib.c \
        src/lua/lmem.c \
        src/lua/lmathlib.c \
        src/lua/llex.c \
        src/lua/liolib.c \
        src/lua/linit.c \
        src/lua/lgc.c \
        src/lua/lfunc.c \
        src/lua/ldump.c \
        src/lua/ldo.c \
        src/lua/ldebug.c \
        src/lua/ldblib.c \
        src/lua/lctype.c \
        src/lua/lcorolib.c \
        src/lua/lcode.c \
        src/lua/lbitlib.c \
        src/lua/lbaselib.c \
        src/lua/lauxlib.c \
        src/lua/lapi.c
    HEADERS += \
        src/lua/lzio.h \
        src/lua/lvm.h \
        src/lua/lundump.h \
        src/lua/lualib.h \
        src/lua/luaconf.h \
        src/lua/lua.hpp \
        src/lua/lua.h \
        src/lua/ltm.h \
        src/lua/ltable.h \
        src/lua/lstring.h \
        src/lua/lstate.h \
        src/lua/lparser.h \
        src/lua/lopcodes.h \
        src/lua/lobject.h \
        src/lua/lmem.h \
        src/lua/llimits.h \
        src/lua/llex.h \
        src/lua/lgc.h \
        src/lua/lfunc.h \
        src/lua/ldo.h \
        src/lua/ldebug.h \
        src/lua/lctype.h \
        src/lua/lcode.h \
        src/lua/lauxlib.h \
        src/lua/lapi.h
    INCLUDEPATH += src/lua
}

#CONFIG(luajit){
#    HEADERS += \
#        src/luajit/lauxlib.h \
#        src/luajit/luaconf.h \
#        src/luajit/lua.h \
#        src/luajit/lua.hpp \
#        src/luajit/luajit.h \
#        src/luajit/lualib.h \
#        src/luajit/luatools.h
#    INCLUDEPATH += src/luajit
#    unix: LIBS += -L/usr/local/lib -lluajit-5.1
#}

TRANSLATIONS += builds/vs2010/sanguosha.ts

#system("lrelease builds/vs2010/sanguosha.ts")

OTHER_FILES += \
    sanguosha.qss \
    acknowledgement/main.qml \
    acknowledgement/list.png \
    acknowledgement/back.png

symbian: LIBS += -lfreetype
else:unix|win32: LIBS += -L$$PWD/lib/ -lfreetype253

INCLUDEPATH += $$PWD/include/freetype
DEPENDPATH += $$PWD/include/freetype

DEFINES += _CRT_SECURE_NO_WARNINGS
