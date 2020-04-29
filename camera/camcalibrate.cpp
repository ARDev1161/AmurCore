#include "camcalibrate.h"
#include "ui_camcalibrate.h"

CamCalibrate::CamCalibrate(Mat *source, CamSettingsHolder *holder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CamCalibrate)
{
    ui->setupUi(this);

    camHolder = holder;
    camHolder->setFlag(0);
    sourceMat = source;
}

CamCalibrate::~CamCalibrate()
{
    if(this->ui)
        delete this->ui;
}

void CamCalibrate::outMat(Mat &ToOut)
{
    QImage qimgOut((uchar*) ToOut.data, ToOut.cols, ToOut.rows, ToOut.step,QImage::Format_RGB888);
    ui->OutLabel->setPixmap(QPixmap::fromImage(qimgOut));
}

void CamCalibrate::initFields()
{
    Size imageSize;
    imageSize.width = ui->OutLabel->width();
    imageSize.height = ui->OutLabel->height();
    camHolder->setImageSize(imageSize);

    boardSize.width = ui->spinHorizontal->value();
    boardSize.height = ui->spinVertical->value();

    Mat cameraMatrix ;
    cameraMatrix = Mat(3, 3, CV_32FC1);
    cameraMatrix.ptr<float>(0)[0] = 1;
    cameraMatrix.ptr<float>(1)[1] = 1;
    camHolder->setCameraMatrix(cameraMatrix);

    // Initialize the chessboard corners in the chessboard reference frame.
    // The corners are at 3D location (X,Y,Z)= (i,j,0)
    for (int i=0; i<boardSize.height; i++)
        for (int j=0; j<boardSize.width; j++)
            objectCorners.push_back(Point3f(i, j, 0.0f));

    started = true;
}

void CamCalibrate::getCalibInfo()
{
        cvtColor(*sourceMat, gray, COLOR_BGR2GRAY);

        bool found = findChessboardCorners(gray, boardSize, imageCorners, CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE | CALIB_CB_FILTER_QUADS | CALIB_CB_FAST_CHECK);

        if(found & (pauseForChangePosition > 7))
        {
            cornerSubPix(gray, imageCorners, Size(7, 7), Size(-1, -1), TermCriteria(TermCriteria::EPS | TermCriteria::MAX_ITER, 30, 0.1));

            drawChessboardCorners(*sourceMat, camHolder->getImageSize(), imageCorners, found);

            objectPoints.push_back(objectCorners);
            imagePoints.push_back(imageCorners);

            successes++;
            if(ui)
                ui->labelSuccessed->setText(QString::number(successes));

            pauseForChangePosition = 0;
         }

        pauseForChangePosition++;
        if(successes > 20)
            on_buttonBox_accepted();
}

void CamCalibrate::frameUpdate()
{
    if(started)
        getCalibInfo();

    outMat(*sourceMat);
}

void CamCalibrate::on_pushButton_clicked()
{
    initFields();

    ui->groupBox->setEnabled(false);
}

void CamCalibrate::on_buttonBox_accepted()
{
    if(!imagePoints.empty()){
        camHolder->setObjectPoints(objectPoints);
        camHolder->setImagePoints(imagePoints);

        camCalib = new Calibrator(camHolder);
        camCalib->addThread();
    }
    this->close();
}

void CamCalibrate::on_buttonBox_rejected()
{
    this->close();
}
