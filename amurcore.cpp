#include "amurcore.h"
#include <chrono>
#include "ui_AmurCore.h"

AmurCore::AmurCore(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AmurCore)
{
    ui->setupUi(this);

    config = new ConfigProcessor(configName); // Load config file
    fillFieldsByConfig(); // Fill fields by config file

    outputMat = imread("data/images/no_picture.jpeg");

    this->initialize();
}

AmurCore::~AmurCore()
{
    if (captureState) {
        captureState->stop.store(true);
    }
    capture.release();
    delete ui;
}

void AmurCore::fillFieldsByConfig()
{
    std::string ip ;
    // int result = config->searchString("Amur.Network.address", ip);
    // if(ip != ""){
    //     this->address = ip;
    // }
}

void AmurCore::initialize()
{
    controls = std::make_shared<Controls>();
    sensors = std::make_shared<Sensors>();
    map = std::make_shared<map_service::GetMapResponse>();

    camHolder = new CamSettingsHolder();
    joyState = std::make_shared<JoyState>();
    joystickDialog = new JoystickDialog(joyState, this);

    repo = std::make_shared<RobotRepository>("myRobots.db");
    if (!repo->openDatabase()) {
        qDebug() << "Cannot open database!";
    }

    network = std::make_shared<NetworkController>(controls, sensors, map); // TODO - add robot id & &<vector> of robots id
    network->runArpingService(arpPort, grpcPort, arpHeader); // Start listening for initial arp message from robots
    network->runServer(address_mask); // Start AmurCore gRPC server
    connect(network.get(), &NetworkController::sensorsUpdated, this, &AmurCore::onSensorsUpdated);
    connectDialog = new ConnectDialog(this, network, repo);
    statusLabel = new QLabel(statusMessage, this);
    ui->statusbar->addPermanentWidget(statusLabel);

    std::mutex& mapMutex = network->getServerInstance()->getMapMutex(); // MapStream mutex
    std::mutex& grpcMutex = network->getServerInstance()->getMutex(); // DataStreamExchange mutex

    amurLogic = new Logic(joyState, controls, sensors, grpcMutex);
    navigationDialog = new NavDialog(controls, sensors, map, mapMutex, grpcMutex, this);
    robotInfoDialog = new RobotInfoDialog();

    connMenu();
    startTimer();
    startCap();
}

void AmurCore::connMenu()
{
    connect(joystickDialog, &JoystickDialog::accepted, this, &AmurCore::fetchJoystickId);

    // File menu
    connect(ui->action_Joystick, SIGNAL(triggered()), this, SLOT(joystickDialogOpen()));
    connect(ui->actionE_xit, SIGNAL(triggered(bool)), this, SLOT(close()));

    // Robot menu
    connect(ui->action_Connect, SIGNAL(triggered()), this, SLOT(connectDialogOpen()));
    connect(ui->action_Reboot, SIGNAL(triggered()), this, SLOT(robotReboot()));
    connect(ui->action_Halt, SIGNAL(triggered()), this, SLOT(robotHalt()));
    connect(ui->actionCamera, SIGNAL(triggered()), this, SLOT(calibDialogOpen()));
    connect(ui->action_Navigation, SIGNAL(triggered()), this, SLOT(mapDialogOpen()));
    connect(ui->actionRobot_Info, SIGNAL(triggered()), this, SLOT(robotInfoDialogOpen()));
}

void AmurCore::joystickDialogOpen()
{
    joystickDialog->exec();
}

void AmurCore::connectDialogOpen()
{
//    if(tcpThread == nullptr){
//       tcpThread = new TCP(controls, sensors, hostName);
//       tcpThread->addThread();
//    }

    connectDialog->exec();
}

void AmurCore::calibDialogOpen()
{
    CamCalibrate *calibDialog;
    calibDialog = new CamCalibrate(&sourceMat, camHolder, this);

    connect(this, &AmurCore::timeout, calibDialog, &CamCalibrate::frameUpdate);
    connect(calibDialog, &CamCalibrate::finished, calibDialog, &CamCalibrate::deleteLater);

    if(camHolder->getReady())
        camHolder->setReady(false);

    calibDialog->exec();
}

void AmurCore::mapDialogOpen()
{
    navigationDialog->show(); // Немодальное открытие
}

void AmurCore::robotInfoDialogOpen()
{
    robotInfoDialog->show(); // Немодальное открытие
}

void AmurCore::resizeEvent(QResizeEvent *event)
{    
    Q_UNUSED(event);
    Mat rgb;
    if (outputMat.channels() == 3) {
        cv::cvtColor(outputMat, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = outputMat;
    }
    QImage qimgOut((uchar*) rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    ui->OutLabel->setPixmap(QPixmap::fromImage(qimgOut).scaled(
                    this->width() - 16,
                    this->height() - 60
                    ));
}

void AmurCore::robotHalt()
{
    controls->mutable_system()->set_haltflag(true);
}

void AmurCore::robotReboot()
{
    controls->mutable_system()->set_restartflag(true);
}

void AmurCore::onSensorsUpdated()
{
    if (!sensors) {
        return;
    }

    const std::string pipeline = sensors->video_stream().pipeline();
    static std::string last_reported_pipeline;
    if (pipeline != last_reported_pipeline) {
        last_reported_pipeline = pipeline;
        std::cerr << "Video pipeline received: "
                  << (pipeline.empty() ? "<empty>" : pipeline) << std::endl;
    }
    if (pipeline.empty() || pipeline == videoStreamPipeline) {
        return;
    }

    videoStreamPipeline = pipeline;
    startCap();
}

void AmurCore::fetchJoystickId()
{
    joyThread = new Joystick(joyState);
    joyThread->addThread();
}

void AmurCore::startTimer()
{
    tmrTimer = new QTimer(this);
    connect(tmrTimer,SIGNAL(timeout()),this,SLOT(frameUpdate()));
    connect(tmrTimer, &QTimer::timeout, this, &AmurCore::timeout);
    tmrTimer->start(loopTime); //msec
}

void AmurCore::startCap()
{
    capture.release();

    if (videoStreamPipeline.empty()) {
        return;
    }

    const std::string pipeline = videoStreamPipeline;
    auto state = std::make_shared<CaptureState>();
    if (captureState) {
        captureState->stop.store(true);
    }
    captureState = state;

    std::thread([state, pipeline]() {
        cv::VideoCapture localCapture;
        localCapture.set(cv::CAP_PROP_OPEN_TIMEOUT_MSEC, 2000);
        localCapture.set(cv::CAP_PROP_READ_TIMEOUT_MSEC, 200);

        if (!localCapture.open(pipeline, cv::CAP_GSTREAMER)) {
            return;
        }

        cv::Mat frame;
        while (!state->stop.load()) {
            if (localCapture.read(frame)) {
                std::lock_guard<std::mutex> lock(state->mutex);
                frame.copyTo(state->latest);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        localCapture.release();
    }).detach();
}

void AmurCore::frameUpdate()
{
    Mat frame;
    auto state = captureState;
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->latest.empty()) {
            state->latest.copyTo(frame);
        }
    }

    if(!frame.empty()){
        sourceMat = frame;
    //    cv::resize(sourceMat, sourceMat, Size(320, 240));
        undistortMat(sourceMat, undistortedMat);
        amurLogic->setSrcMat(&undistortedMat);
        outputMat = amurLogic->getOutMat();
        outMat(sourceMat);
    }

    worker();
}

void AmurCore::outMat(Mat &toOut)
{
    Mat rgb;
    if (toOut.channels() == 3) {
        cv::cvtColor(toOut, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = toOut;
    }
    QImage qimgOut((uchar*) rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);

    ui->OutLabel->setPixmap(QPixmap::fromImage(qimgOut).scaled(
                                this->width() - 16,
                                this->height() - 60
                                ));
}

void AmurCore::undistortMat(Mat &inMat, Mat &outMat)
{
    if(camHolder->getReady())
        outMat = camHolder->remap(inMat);
    else
        outMat = inMat;
}

void AmurCore::updateStatusMessage()
{
    const RobotList robots = network->getRobots();
    if (robots.isEmpty()) {
        currentRobotId.clear();
        if (statusMessage != "No robot connected") {
            statusMessage = "No robot connected";
            statusLabel->setText(statusMessage);
        }
        return;
    }

    RobotEntryPtr activeRobot;
    for (const auto &robot : robots) {
        if (!robot) {
            continue;
        }

        // Prefer robots with a human-friendly name, otherwise use the first available.
        if (!robot->name().isEmpty()) {
            activeRobot = robot;
            break;
        }

        if (!activeRobot) {
            activeRobot = robot;
        }
    }

    if (!activeRobot) {
        currentRobotId.clear();
        if (statusMessage != "No robot connected") {
            statusMessage = "No robot connected";
            statusLabel->setText(statusMessage);
        }
        return;
    }

    const QString machineId = activeRobot->machineID();

    // Try to pull a stored name from the repo once per machine ID.
    if (activeRobot->name().isEmpty() && machineId != currentRobotId && !machineId.isEmpty()) {
        RobotList known = repo->searchRobots(nullptr, machineId);
        if (!known.isEmpty() && known.first()) {
            activeRobot->setName(known.first()->name());
        }
    }

    currentRobotId = machineId;

    QString displayName = activeRobot->name();
    if (displayName.isEmpty()) {
        displayName = machineId.isEmpty() ? QString("Unknown robot") : machineId;
    }

    const QString newMessage = QString("Connected to %1 (%2:%3)")
                                   .arg(displayName, activeRobot->address().toString())
                                   .arg(activeRobot->port());

    if (newMessage != statusMessage) {
        statusMessage = newMessage;
        statusLabel->setText(statusMessage);
    }
}

void AmurCore::worker()
{
    updateStatusMessage();
    amurLogic->process();
}
