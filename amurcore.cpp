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
    grpcMutex = &network->getServerInstance()->getMutex(); // DataStreamExchange mutex

    amurLogic = new Logic(joyState, controls, sensors, *grpcMutex);
    navigationDialog = new NavDialog(controls, sensors, map, mapMutex, *grpcMutex, this);
    robotInfoDialog = new RobotInfoDialog();

    connMenu();
    setupVideoControls();
    if (!sensorsUpdateTimer) {
        sensorsUpdateTimer = new QTimer(this);
        sensorsUpdateTimer->setInterval(200);
        connect(sensorsUpdateTimer, &QTimer::timeout, this, [this]() {
            if (!sensorsUpdatePending.exchange(false)) {
                return;
            }
            if (!sensors) {
                return;
            }
            std::string pipeline;
            if (grpcMutex) {
                std::unique_lock<std::mutex> lock(*grpcMutex);
                pipeline = sensors->video_stream().pipeline();
            } else {
                pipeline = sensors->video_stream().pipeline();
            }
            if (!pipeline.empty() && pipeline != videoStreamPipeline) {
                videoStreamPipeline = pipeline;
                startCap();
            }
            updateVideoSources();
        });
        sensorsUpdateTimer->start();
    }
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
    if (videoSourceCombo && !updatingVideoSources) {
        if (!videoConfigTimer) {
            videoConfigTimer = new QTimer(this);
            videoConfigTimer->setSingleShot(true);
            connect(videoConfigTimer, &QTimer::timeout, this, &AmurCore::applyVideoSelection);
        }
        videoConfigTimer->start(250);
    }
}

void AmurCore::robotHalt()
{
    std::unique_lock<std::mutex> lock;
    if (grpcMutex) {
        lock = std::unique_lock<std::mutex>(*grpcMutex);
    }
    controls->mutable_system()->set_haltflag(true);
}

void AmurCore::robotReboot()
{
    std::unique_lock<std::mutex> lock;
    if (grpcMutex) {
        lock = std::unique_lock<std::mutex>(*grpcMutex);
    }
    controls->mutable_system()->set_restartflag(true);
}

void AmurCore::onSensorsUpdated()
{
    sensorsUpdatePending.store(true);
}

void AmurCore::fetchJoystickId()
{
    if(joyThread)
    {
        joyThread->stopThreads();
        joyThread->deleteLater();
        joyThread = nullptr;
    }

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

void AmurCore::setupVideoControls()
{
    videoSourceCombo = new QComboBox(ui->menubar);
    videoSourceCombo->setMinimumWidth(220);
    videoSourceCombo->addItem("Video source");
    ui->menubar->setCornerWidget(videoSourceCombo, Qt::TopRightCorner);

    connect(videoSourceCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &AmurCore::applyVideoSelection);
}

void AmurCore::updateVideoSources()
{
    if (!videoSourceCombo || !sensors) {
        return;
    }

    video::VideoStatus status;
    {
        std::unique_lock<std::mutex> lock;
        if (grpcMutex) {
            lock = std::unique_lock<std::mutex>(*grpcMutex);
        }
        status = sensors->video_stream().status();
    }

    if (status.sources_size() == 0) {
        return;
    }

    QStringList signature_parts;
    signature_parts.reserve(status.sources_size());
    for (int i = 0; i < status.sources_size(); ++i) {
        const auto &src = status.sources(i);
        signature_parts << QString("%1:%2").arg(src.type()).arg(QString::fromStdString(src.name()));
    }
    const QString signature = signature_parts.join("|");

    const int active_type = status.active_source().type();
    const QString active_name = QString::fromStdString(status.active_source().name());
    if (signature == lastVideoSourcesSignature &&
        active_type == lastVideoActiveType &&
        active_name == lastVideoActiveName) {
        return;
    }

    QString current_name = videoSourceCombo->currentData(Qt::UserRole + 1).toString();
    int current_type = videoSourceCombo->currentData(Qt::UserRole).toInt();

    updatingVideoSources = true;
    videoSourceCombo->blockSignals(true);
    videoSourceCombo->clear();
    videoSourceCombo->addItem("Video source");

    int active_index = -1;
    for (int i = 0; i < status.sources_size(); ++i) {
        const auto &src = status.sources(i);
        QString label;
        if (src.type() == video::V4L2_DEVICE) {
            label = QString("Device: %1").arg(QString::fromStdString(src.name()));
        } else {
            label = QString::fromStdString(src.name());
        }
        videoSourceCombo->addItem(label);
        int index = videoSourceCombo->count() - 1;
        videoSourceCombo->setItemData(index, static_cast<int>(src.type()), Qt::UserRole);
        videoSourceCombo->setItemData(index, QString::fromStdString(src.name()), Qt::UserRole + 1);

        if (!status.active_source().name().empty() &&
            status.active_source().name() == src.name() &&
            status.active_source().type() == src.type()) {
            active_index = index;
        }
    }

    if (active_index >= 0) {
        videoSourceCombo->setCurrentIndex(active_index);
    } else {
        for (int i = 1; i < videoSourceCombo->count(); ++i) {
            if (videoSourceCombo->itemData(i, Qt::UserRole).toInt() == current_type &&
                videoSourceCombo->itemData(i, Qt::UserRole + 1).toString() == current_name) {
                videoSourceCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    videoSourceCombo->blockSignals(false);
    updatingVideoSources = false;
    lastVideoSourcesSignature = signature;
    lastVideoActiveType = active_type;
    lastVideoActiveName = active_name;
}

void AmurCore::applyVideoSelection()
{
    if (updatingVideoSources || !controls || !videoSourceCombo) {
        return;
    }

    int index = videoSourceCombo->currentIndex();
    if (index <= 0) {
        return;
    }

    int type = videoSourceCombo->itemData(index, Qt::UserRole).toInt();
    QString name = videoSourceCombo->itemData(index, Qt::UserRole + 1).toString();
    if (name.isEmpty()) {
        return;
    }

    int width = ui->OutLabel->width();
    int height = ui->OutLabel->height();
    if (type == lastVideoSourceType &&
        name == lastVideoSourceName &&
        width == lastVideoWidth &&
        height == lastVideoHeight) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock;
        if (grpcMutex) {
            lock = std::unique_lock<std::mutex>(*grpcMutex);
        }

        auto config = controls->mutable_video_config();
        auto source = config->mutable_requested_source();
        source->set_type(static_cast<video::VideoSourceType>(type));
        source->set_name(name.toStdString());

        auto format = config->mutable_format();
        format->set_width(width);
        format->set_height(height);
        format->set_fps(static_cast<float>(1000.0 / loopTime));
        format->set_encoding("rgb8");
        format->set_bitrate_kbps(videoBitrateKbps);

        config->set_use_widget_size(true);
    }

    lastVideoSourceType = type;
    lastVideoSourceName = name;
    lastVideoWidth = width;
    lastVideoHeight = height;
}

void AmurCore::worker()
{
    updateStatusMessage();
    amurLogic->process();
}
