#-------------------------------------------------
#
# Project created by QtCreator 2018-10-18T06:43:04
#
#-------------------------------------------------

QT       += core gui multimedia network texttospeech

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimediawidgets

TARGET = AmurCore
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += /usr/include/opencv4/

INCLUDEPATH += /usr/include/SDL/

# INCLUDEPATH += /usr/include/pocketsphinx/
# INCLUDEPATH += /usr/include/sphinxbase/

LIBS += -lSDL

# LIBS += -lpocketsphinx
# LIBS += -lsphinxbase

LIBS += -L$$LIBS_PATH /
LIBS += -lprotobuf \
        -lopencv_core \
        -lopencv_calib3d \
        -lopencv_ml \
        -lopencv_dnn \
        -lopencv_features2d \
        -lopencv_flann \
        -lopencv_imgproc \
        -lopencv_imgcodecs \
        -lopencv_highgui \
        -lopencv_objdetect \
        -lopencv_photo \
        -lopencv_shape \
        -lopencv_stitching \
        -lopencv_superres \
        -lopencv_video \
        -lopencv_videoio \
        -lopencv_videostab

SOURCES += \
        amurcore.cpp \
        main.cpp \
#Cameras sources
        camera/camcalibrate.cpp \
        camera/camsettingsholder.cpp \
        camera/calibrator.cpp \
        camera/calibratorworker.cpp \
#Joystick sources
        joystick/joystick.cpp \
        joystick/v_joystick_adapter.cpp \
        joystick/joystickdialog.cpp \
        joystick/getstatebyjoy.cpp \
        joystick/joystickidholder.cpp \
        joystick/joystickstateworker.cpp \
#Network sources
        network/connectdialog.cpp \
    network/protobuf/controls.pb.cc \
        network/tcp/tcp.cpp \
        network/tcp/tcpcontrol.cpp \
        network/tcp/tcpsensors.cpp \
        network/tcp/tcpworker.cpp \
        network/protobuf/sensors.pb.cc \
#Threads sources
        threads/session.cpp \
        threads/worker.cpp \
#Logic sources
        logic/logic.cpp \
        logic/speech/sphinxrecognizer.cpp \
        logic/speech/speechdialog.cpp

CONFIG += precompile_header
PRECOMPILED_HEADER = pch.h

HEADERS += \
        amurcore.h \
    joystick/joystickstate.h \
    network/protobuf/controls.pb.h \
        pch.h \
#Cameras headers
        camera/camcalibrate.h \
        camera/camsettingsholder.h \
        camera/calibrator.h \
        camera/calibratorworker.h \
#Joystick headers
        joystick/v_joystick_adapter.h \
        joystick/joystickdialog.h \
        joystick/getstatebyjoy.h \
        joystick/joystickidholder.h \
        joystick/joystickstateworker.h \
        joystick/joystick.h \
#Network headers
        network/connectdialog.h \
        network/tcp/tcpcontrol.h \
        network/tcp/tcpsensors.h \
        network/tcp/tcpworker.h \
        network/tcp/tcp.h \
        network/protobuf/sensors.pb.h \
#Threads headers
        threads/session.h \
        threads/worker.h \
#Logic headers
        logic/logic.h \
        logic/speech/prim_type.h \
        logic/speech/speechdialog.h \
        logic/speech/sphinxrecognizer.h

FORMS += \
        AmurCore.ui \
        camera/camcalibrate.ui \
        joystick/joystickdialog.ui \
        network/connectdialog.ui \
        logic/speech/speechdialog.ui

DISTFILES += \
        README.md \
        network/protobuf/controls.proto \
        network/protobuf/sensors.proto \
        network/protobuf/rebuild.sh \
        data/images/no_picture.jpeg

SUBDIRS += \
        AmurCore.pro
