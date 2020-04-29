/*
 *  Copyright (c) 2020 ARDev1161 <radtarasov@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AmurCore_H
#define AmurCore_H

#undef main
#include "pch.h"

#define NO_PICTURE "./data/images/no_picture.jpeg"
#define SOURCE_STREAM 0
//#define SOURCE_STREAM "udpsrc port=1488 ! application/x-rtp, encoding-name=H264 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink"

namespace Ui {
    class AmurCore;
}

class AmurCore : public QMainWindow
{
    Q_OBJECT

    Ui::AmurCore *ui;

    QTimer *tmrTimer;
    QString *hostName;

    SpeechDialog *speechDialog;
    ConnectDialog *connectDialog;
    JoystickDialog *joystickDialog;

    AmurControls *amurControl;
    AmurSensors *zanyaSensors;

    TCP *tcpThread;
    Joystick *joyThread;
    JoyState *joyState;
    Logic *amurLogic;

    CamSettingsHolder *camHolder;
    JoystickIdHolder *idHolder;

    VideoCapture capture;

    Mat sourceMat;
    Mat undistortedMat;
    Mat outputMat;

    int loopTime = 50;

public:
    explicit AmurCore(QWidget *parent = nullptr);
    ~AmurCore();

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void frameUpdate();
    void calibDialogOpen();
    void speechDialogOpen();
    void connectDialogOpen();
    void joystickDialogOpen();
    void fetchJoystickId();

    void amurHalt();
    void amurReboot();

private:
    void startCap();
    void startTimer();
    void initFields();
    void connMenu();
    void worker();
    void outMat(Mat &toOut);
    void undistortMat(Mat &inMat, Mat &outMat);

signals:
    void timeout();

};

#endif // AmurCore_H
