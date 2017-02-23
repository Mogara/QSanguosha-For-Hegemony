# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = QSanguosha
QT += network widgets
TEMPLATE = app
CONFIG += audio

CONFIG += c++11

CONFIG += lua
#CONFIG += lua53

SOURCES += \
    src/main.cpp \
    src/client/aux-skills.cpp \
    src/client/client.cpp \
    src/client/clientplayer.cpp \
    src/client/clientstruct.cpp \
    src/core/banpair.cpp \
    src/core/card.cpp \
    src/core/engine.cpp \
    src/core/general.cpp \
    src/core/lua-wrapper.cpp \
    src/core/player.cpp \
    src/core/protocol.cpp \
    src/core/record-analysis.cpp \
    src/core/roomstate.cpp \
    src/core/settings.cpp \
    src/core/skill.cpp \
    src/core/structs.cpp \
    src/core/util.cpp \
    src/core/wrappedcard.cpp \
    src/core/version.cpp \
    src/core/json.cpp \
    src/dialog/aboutus.cpp \
    src/dialog/cardeditor.cpp \
    src/dialog/cardoverview.cpp \
    src/dialog/configdialog.cpp \
    src/dialog/connectiondialog.cpp \
    src/dialog/customassigndialog.cpp \
    src/dialog/distanceviewdialog.cpp \
    src/dialog/freechoosedialog.cpp \
    src/dialog/generaloverview.cpp \
    src/dialog/mainwindow.cpp \
    src/dialog/rule-summary.cpp \
    src/dialog/updatechecker.cpp \
    src/dialog/udpdetectordialog.cpp \
    src/dialog/avatarmodel.cpp \
    src/dialog/generalmodel.cpp \
    src/dialog/serverdialog.cpp \
    src/dialog/flatdialog.cpp \
    src/dialog/banipdialog.cpp \
    src/dialog/banlistdialog.cpp \
    src/package/exppattern.cpp \
    src/package/formation.cpp \
    src/package/jiange-defense.cpp \
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
    src/package/strategic-advantage.cpp \
    src/package/package.cpp \
    src/scenario/miniscenarios.cpp \
    src/scenario/scenario.cpp \
    src/scenario/scenerule.cpp \
    src/scenario/jiange-defense-scenario.cpp \
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
    src/ui/genericcardcontainerui.cpp \
    src/ui/indicatoritem.cpp \
    src/ui/magatamasitem.cpp \
    src/ui/photo.cpp \
    src/ui/pixmapanimation.cpp \
    src/ui/qsanbutton.cpp \
    src/ui/qsanselectableitem.cpp \
    src/ui/rolecombobox.cpp \
    src/ui/roomscene.cpp \
    src/ui/skinbank.cpp \
    src/ui/sprite.cpp \
    src/ui/startscene.cpp \
    src/ui/tablepile.cpp \
    src/ui/timedprogressbar.cpp \
    src/ui/uiutils.cpp \
    src/ui/window.cpp \
    src/ui/chooseoptionsbox.cpp \
    src/ui/choosetriggerorderbox.cpp \
    src/ui/graphicsbox.cpp \
    src/ui/guanxingbox.cpp \
    src/ui/title.cpp \
    src/ui/bubblechatbox.cpp \
    src/ui/stylehelper.cpp \
    src/ui/playercardbox.cpp \
    src/ui/graphicspixmaphoveritem.cpp \
    src/ui/heroskincontainer.cpp \
    src/ui/skinitem.cpp \
    src/ui/tile.cpp \
    src/ui/choosesuitbox.cpp \
    src/util/detector.cpp \
    src/util/nativesocket.cpp \
    src/util/recorder.cpp \
    swig/sanguosha_wrap.cxx \
    src/ui/guhuobox.cpp \
	src/ui/cardchoosebox.cpp \   
    src/package/transformation.cpp \
    src/ui/lightboxanimation.cpp \
    src/ui/pindianbox.cpp

HEADERS += \
    src/client/aux-skills.h \
    src/client/client.h \
    src/client/clientplayer.h \
    src/client/clientstruct.h \
    src/core/audio.h \
    src/core/banpair.h \
    src/core/card.h \
    src/core/compiler-specific.h \
    src/core/engine.h \
    src/core/general.h \
    src/core/lua-wrapper.h \
    src/core/namespace.h \
    src/core/player.h \
    src/core/protocol.h \
    src/core/record-analysis.h \
    src/core/roomstate.h \
    src/core/settings.h \
    src/core/skill.h \
    src/core/structs.h \
    src/core/util.h \
    src/core/wrappedcard.h \
    src/core/version.h \
    src/core/json.h \
    src/dialog/aboutus.h \
    src/dialog/cardeditor.h \
    src/dialog/cardoverview.h \
    src/dialog/configdialog.h \
    src/dialog/connectiondialog.h \
    src/dialog/customassigndialog.h \
    src/dialog/distanceviewdialog.h \
    src/dialog/freechoosedialog.h \
    src/dialog/generaloverview.h \
    src/dialog/mainwindow.h \
    src/dialog/rule-summary.h \
    src/dialog/updatechecker.h \
    src/dialog/udpdetectordialog.h \
    src/dialog/avatarmodel.h \
    src/dialog/generalmodel.h \
    src/dialog/serverdialog.h \
    src/dialog/flatdialog.h \
    src/dialog/banipdialog.h \
    src/dialog/banlistdialog.h \
    src/package/exppattern.h \
    src/package/formation.h \
    src/package/jiange-defense.h \
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
    src/package/strategic-advantage.h \
    src/package/standard-package.h \
    src/scenario/miniscenarios.h \
    src/scenario/scenario.h \
    src/scenario/scenerule.h \
    src/scenario/jiange-defense-scenario.h \
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
    src/ui/genericcardcontainerui.h \
    src/ui/indicatoritem.h \
    src/ui/magatamasitem.h \
    src/ui/photo.h \
    src/ui/pixmapanimation.h \
    src/ui/qsanbutton.h \
    src/ui/qsanselectableitem.h \
    src/ui/rolecombobox.h \
    src/ui/roomscene.h \
    src/ui/skinbank.h \
    src/ui/sprite.h \
    src/ui/startscene.h \
    src/ui/tablepile.h \
    src/ui/timedprogressbar.h \
    src/ui/uiutils.h \
    src/ui/window.h \
    src/ui/chooseoptionsbox.h \
    src/ui/choosetriggerorderbox.h \
    src/ui/graphicsbox.h \
    src/ui/guanxingbox.h \
    src/ui/title.h \
    src/ui/bubblechatbox.h \
    src/ui/stylehelper.h \
    src/ui/playercardbox.h \
    src/ui/graphicspixmaphoveritem.h \
    src/ui/heroskincontainer.h \
    src/ui/skinitem.h \
    src/ui/tile.h \
    src/ui/choosesuitbox.h \
    src/util/detector.h \
    src/util/nativesocket.h \
    src/util/recorder.h \
    src/util/socket.h \
    src/ui/guhuobox.h \
	src/ui/cardchoosebox.h \
    src/package/transformation.h \
    src/ui/lightboxanimation.h \
    src/ui/pindianbox.h

FORMS += \
    src/dialog/cardoverview.ui \
    src/dialog/configdialog.ui \
    src/dialog/connectiondialog.ui \
    src/dialog/generaloverview.ui
    


CONFIG(buildbot) {
    DEFINES += USE_BUILDBOT
    SOURCES += src/bot_version.cpp
}

win32 {
    FORMS += src/dialog/mainwindow.ui
}
else: linux {
    FORMS += src/dialog/mainwindow.ui
}
else: ios {
    FORMS += \
    src/dialog/mainwindow_ios.ui\
    src/dialog/cardoverview_ios.ui \
    src/dialog/configdialog_ios.ui \
    src/dialog/connectiondialog_ios.ui \
    src/dialog/generaloverview_ios.ui
}
else {
    FORMS += src/dialog/mainwindow_nonwin.ui
}

INCLUDEPATH += include
INCLUDEPATH += src/client
INCLUDEPATH += src/core
INCLUDEPATH += src/dialog
INCLUDEPATH += src/package
INCLUDEPATH += src/scenario
INCLUDEPATH += src/server
INCLUDEPATH += src/ui
INCLUDEPATH += src/util
INCLUDEPATH += ../../../../mingw64/include/freetype2

win32{
    RC_FILE += resource/icon.rc
}

macx{
    ICON = resource/icon/sgs.icns
}

LIBS += -L.
win32-msvc*{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    !contains(QMAKE_HOST.arch, x86_64) {
        DEFINES += WIN32
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x86"
    } else {
        DEFINES += WIN64
        LIBS += -L"$$_PRO_FILE_PWD_/lib/win/x64"
    }
    CONFIG(debug, debug|release) {
        !winrt:INCLUDEPATH += include/vld
    } else {
        QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
        DEFINES += USE_BREAKPAD

        SOURCES += src/breakpad/client/windows/crash_generation/client_info.cc \
            src/breakpad/client/windows/crash_generation/crash_generation_client.cc \
            src/breakpad/client/windows/crash_generation/crash_generation_server.cc \
            src/breakpad/client/windows/crash_generation/minidump_generator.cc \
            src/breakpad/client/windows/handler/exception_handler.cc \
            src/breakpad/common/windows/guid_string.cc

        HEADERS += src/breakpad/client/windows/crash_generation/client_info.h \
            src/breakpad/client/windows/crash_generation/crash_generation_client.h \
            src/breakpad/client/windows/crash_generation/crash_generation_server.h \
            src/breakpad/client/windows/crash_generation/minidump_generator.h \
            src/breakpad/client/windows/handler/exception_handler.h \
            src/breakpad/common/windows/guid_string.h

        INCLUDEPATH += src/breakpad
        INCLUDEPATH += src/breakpad/client/windows
    }
}
win32-g++{
    DEFINES += WIN32
    LIBS += -L"$$_PRO_FILE_PWD_/lib/win/MinGW"
    DEFINES += GPP
    CONFIG(!static) {
        CONFIG += link_pkgconfig
        PKGCONFIG += freetype2
    }
}
winrt{
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += WINRT
    !winphone {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/winrt/x64"
    } else {
        DEFINES += WINPHONE
        contains($$QMAKESPEC, arm): LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/arm"
        else : LIBS += -L"$$_PRO_FILE_PWD_/lib/winphone/x86"
    }
}
macx{
    DEFINES += MAC
    LIBS += -L"$$_PRO_FILE_PWD_/lib/mac/lib"
}
ios{
    DEFINES += IOS
    CONFIG(iphonesimulator){
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/simulator/lib"
    }
    else {
        LIBS += -L"$$_PRO_FILE_PWD_/lib/ios/device/lib"
    }
}
linux{
    android{
        DEFINES += ANDROID
        ANDROID_LIBPATH = $$_PRO_FILE_PWD_/lib/android/$$ANDROID_ARCHITECTURE/lib
        LIBS += -L"$$ANDROID_LIBPATH"
    }
    else {
        DEFINES += LINUX
        !contains(QMAKE_HOST.arch, x86_64) {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x86"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x86
        }
        else {
            LIBS += -L"$$_PRO_FILE_PWD_/lib/linux/x64"
            QMAKE_LFLAGS += -Wl,--rpath=lib/linux/x64
        }
    }
}

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    QT += multimedia
#    INCLUDEPATH += include/fmod
#    CONFIG(debug, debug|release): LIBS += -lfmodL
#    else:LIBS += -lfmod64
    SOURCES += src/core/audio.cpp

#    android{
#        CONFIG(debug, debug|release):ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodexL.so
#        else:ANDROID_EXTRA_LIBS += $$ANDROID_LIBPATH/libfmodex.so
#    }

#    ios: QMAKE_LFLAGS += -framework AudioToolBox
}


CONFIG(lua){

android:DEFINES += "\"getlocaledecpoint()='.'\""

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

CONFIG(lua53){

android:DEFINES += "\"l_getlocaledecpoint()='.'\""

    SOURCES += \
        src/lua53/lzio.c \
        src/lua53/lvm.c \
        src/lua53/lundump.c \
        src/lua53/ltm.c \
        src/lua53/ltablib.c \
        src/lua53/ltable.c \
        src/lua53/lstrlib.c \
        src/lua53/lstring.c \
        src/lua53/lstate.c \
        src/lua53/lparser.c \
        src/lua53/loslib.c \
        src/lua53/lopcodes.c \
        src/lua53/lobject.c \
        src/lua53/loadlib.c \
        src/lua53/lmem.c \
        src/lua53/lmathlib.c \
        src/lua53/llex.c \
        src/lua53/liolib.c \
        src/lua53/linit.c \
        src/lua53/lgc.c \
        src/lua53/lfunc.c \
        src/lua53/ldump.c \
        src/lua53/ldo.c \
        src/lua53/ldebug.c \
        src/lua53/ldblib.c \
        src/lua53/lctype.c \
        src/lua53/lcorolib.c \
        src/lua53/lcode.c \
        src/lua53/lbitlib.c \
        src/lua53/lbaselib.c \
        src/lua53/lauxlib.c \
        src/lua53/lapi.c \
        src/lua53/lutf8lib.c
    HEADERS += \
        src/lua53/lprefix.h \
        src/lua53/lzio.h \
        src/lua53/lvm.h \
        src/lua53/lundump.h \
        src/lua53/lualib.h \
        src/lua53/luaconf.h \
        src/lua53/lua.hpp \
        src/lua53/lua.h \
        src/lua53/ltm.h \
        src/lua53/ltable.h \
        src/lua53/lstring.h \
        src/lua53/lstate.h \
        src/lua53/lparser.h \
        src/lua53/lopcodes.h \
        src/lua53/lobject.h \
        src/lua53/lmem.h \
        src/lua53/llimits.h \
        src/lua53/llex.h \
        src/lua53/lgc.h \
        src/lua53/lfunc.h \
        src/lua53/ldo.h \
        src/lua53/ldebug.h \
        src/lua53/lctype.h \
        src/lua53/lcode.h \
        src/lua53/lauxlib.h \
        src/lua53/lapi.h
    INCLUDEPATH += src/lua53
}

CONFIG(opengl){
    QT += opengl
    DEFINES += USING_OPENGL
}

TRANSLATIONS += builds/sanguosha.ts

!build_pass{
    system("lrelease $$_PRO_FILE_PWD_/builds/sanguosha.ts -qm $$_PRO_FILE_PWD_/sanguosha.qm")

    SWIG_bin = "swig"
    contains(QMAKE_HOST.os, "Windows"): SWIG_bin = "$$_PRO_FILE_PWD_/tools/swig/swig.exe"

    system("$$SWIG_bin -c++ -lua $$_PRO_FILE_PWD_/swig/sanguosha.i")
}

OTHER_FILES += \
    sanguosha.qss \
    ui-script/animation.qml \
    resource/android/AndroidManifest.xml \
    builds/sanguosha.ts


ANDROID_PACKAGE_SOURCE_DIR = $$_PRO_FILE_PWD_/resource/android

RESOURCES +=
