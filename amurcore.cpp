#include "amurcore.h"
#include "ui_AmurCore.h"

AmurCore::AmurCore(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AmurCore)
{
    ui->setupUi(this);

    outputMat = imread("../AmurCore/data/images/no_picture.jpeg");

    this->initialize();
}

 AmurCore::~AmurCore()
{
    capture.release();
    delete ui;
}

void AmurCore::initialize()
{
    controls = new AMUR::AmurControls();
    sensors = new AMUR::AmurSensors();

    camHolder = new CamSettingsHolder();
    joyState = new JoyState();

    joystickDialog = new JoystickDialog(joyState, this);
    speechDialog = new SpeechDialog(this);
    connectDialog = new ConnectDialog(this);

    network = new NetworkController(controls, sensors);
    amurLogic = new Logic(joyState, controls, sensors);

    connMenu();
    startTimer();
    startCap();

    network->runServer(address_mask);
}

void AmurCore::connMenu()
{
    connect(joystickDialog, &JoystickDialog::accepted, this, &AmurCore::fetchJoystickId);

    // File menu
    connect(ui->action_Joystick, SIGNAL(triggered()), this, SLOT(joystickDialogOpen()));
    connect(ui->actionE_xit, SIGNAL(triggered(bool)), this, SLOT(close()));

    // Zanya menu
    connect(ui->action_Connect, SIGNAL(triggered()), this, SLOT(connectDialogOpen()));
    connect(ui->action_Halt, SIGNAL(triggered()), this, SLOT(amurHalt()));
    connect(ui->action_Reboot, SIGNAL(triggered()), this, SLOT(amurReboot()));
    connect(ui->actionCamera, SIGNAL(triggered()), this, SLOT(calibDialogOpen()));

    // Speech dialog
    connect(ui->action_Speech, SIGNAL(triggered()), this, SLOT(speechDialogOpen()));
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

void AmurCore::resizeEvent(QResizeEvent *event)
{    
    Q_UNUSED(event);
    QImage qimgOut((uchar*) outputMat.data, outputMat.cols, outputMat.rows, outputMat.step, QImage::Format_RGB888);
    ui->OutLabel->setPixmap(QPixmap::fromImage(qimgOut).scaled(
                    this->width() - 16,
                    this->height() - 60
                    ));
}

void AmurCore::speechDialogOpen()
{
    speechDialog->exec();
}

void AmurCore::amurHalt()
{
    controls->mutable_system()->set_haltflag(true);
}

void AmurCore::amurReboot()
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
        worker();
    }

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
    cv::flip(sourceMat, sourceMat, 1);
//    cv::resize(sourceMat, sourceMat, Size(320, 240));

    undistortMat(sourceMat, undistortedMat);

    amurLogic->setSrcMat(&undistortedMat);
    outputMat = amurLogic->getOutMat();

    outMat(sourceMat);

    amurLogic->process();
}
