#include "amurcore.h"
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
    connectDialog = new ConnectDialog(this, network, repo);

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
    QImage qimgOut((uchar*) outputMat.data, outputMat.cols, outputMat.rows, outputMat.step, QImage::Format_RGB888);
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
    capture.open(SOURCE_STREAM);

    if(!capture.isOpened())
        return;

    frameUpdate();
}

void AmurCore::frameUpdate()
{
    if(capture.read(sourceMat)){
        cv::flip(sourceMat, sourceMat, 1);
    //    cv::resize(sourceMat, sourceMat, Size(320, 240));
        undistortMat(sourceMat, undistortedMat);
        amurLogic->setSrcMat(&undistortedMat);
        outputMat = amurLogic->getOutMat();
        outMat(sourceMat);
    }

    worker();
    ui->statusbar->showMessage(statusMessage);
}

void AmurCore::outMat(Mat &toOut)
{
    QImage qimgOut((uchar*) toOut.data, toOut.cols, toOut.rows, toOut.step, QImage::Format_RGB888);

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

void AmurCore::worker()
{
    amurLogic->process();
}
