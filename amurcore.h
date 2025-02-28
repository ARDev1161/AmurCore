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
#include <mutex>

#define NO_PICTURE "./data/images/no_picture.jpeg"
#define SOURCE_STREAM 0
//#define SOURCE_STREAM "udpsrc port=1488 ! application/x-rtp, encoding-name=H264 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink"

using namespace Robot;

namespace Ui {
    class AmurCore;
}

class AmurCore : public QMainWindow
{
    Q_OBJECT

    Ui::AmurCore *ui;

    QTimer *tmrTimer;
    QString *hostName;
    QString statusMessage = "No robot connected";

    const char *configName = "AmurCore.cfg";
    ConfigProcessor *config;

    int arpPort = 11111;
    int grpcPort = 7777;
    QString arpHeader = "AMUR";
    std::string address_mask = "0.0.0.0:" + std::to_string(grpcPort);
    // TODO - add vector with robots id
    std::shared_ptr<NetworkController> network;
    std::shared_ptr<RobotRepository> repo;

    ConnectDialog *connectDialog;
    JoystickDialog *joystickDialog;
    NavDialog *navigationDialog;
    RobotInfoDialog *robotInfoDialog;

    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::shared_ptr<map_service::GetMapResponse> map;

    Joystick *joyThread;
    std::shared_ptr<JoyState> joyState;
    Logic *amurLogic;

    CamSettingsHolder *camHolder;
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

    void connectDialogOpen();
    void joystickDialogOpen();
    void calibDialogOpen();
    void mapDialogOpen();
    void robotInfoDialogOpen();

    void fetchJoystickId();

    void robotHalt();
    void robotReboot();

private:
    void fillFieldsByConfig();
    void startCap();
    void startTimer();
    void initialize();
    void connMenu();
    void worker();
    void outMat(Mat &toOut);
    void undistortMat(Mat &inMat, Mat &outMat);

signals:
    void timeout();

};

#endif // AmurCore_H
