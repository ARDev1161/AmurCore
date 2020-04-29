#include "joystickdialog.h"
#include "ui_joystickdialog.h"

JoystickDialog::JoystickDialog(JoystickIdHolder *idHolder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JoystickDialog)
{
    ui->setupUi(this);

    this->idHolder = idHolder;

    initFields();
    connMenu();

    setAvaliableJoystick();
    setDefaultText();
    setFixedSize(590, 365);
}

JoystickDialog::~JoystickDialog()
{
    disconnectFromJoystick();
    delete ui;
    delete joyAdapter;
}

void JoystickDialog::initFields()
{
    joyAdapter = new VJoystickAdapter(nullptr);
    buttonVector = QVector<bool>(MAX_JOYSTICK_BUTTONS);

    buttonVector.fill(false);

    ui->joystickStateBox->setDisabled(true);
    ui->joystickInformationBox->setDisabled(true);
    ui->disconnectPushButton->setDisabled(true);
}

void JoystickDialog::connMenu()
{
    connect(ui->connectPushButton, SIGNAL(clicked()), this, SLOT(connectToJoystick()));
    connect(ui->disconnectPushButton, SIGNAL(clicked()), this, SLOT(disconnectFromJoystick()));
    connect(ui->rescanPushButton, SIGNAL(clicked()), this, SLOT(rescanJoystickDevice()));
}

void JoystickDialog::setDefaultText()
{
    ui->joystickNameLabel->setText("Not connected");
    ui->joystickIdLabel->setText(tr("-1"));
    ui->joystickAxisLabel->setText(tr("-1"));
    ui->joystickHatsLabel->setText(tr("-1"));
    ui->joystickBallsLabel->setText(tr("-1"));
    ui->joystickButtonsLabel->setText(tr("-1"));

    ui->joystickXaxisLabel->setText(tr("0"));
    ui->joystickYaxisLabel->setText(tr("0"));
    ui->joystickXrotationLabel->setText(tr("0"));
    ui->joystickYrotationLabel->setText(tr("0"));
    ui->joystickZLTaxisLabel->setText(tr("0"));
    ui->joystickZRTaxisLabel->setText(tr("0"));
    ui->joystickButtonsTestLabel->setText(tr(""));
    ui->joystickPOV0Label->setText(tr("0"));
}

void JoystickDialog::setAvaliableJoystick()
{
    ui->joystickComboBox->clear();
    ui->joystickComboBox->addItems(VJoystickAdapter::getAvailableJoystickName());
}

void JoystickDialog::rescanJoystickDevice()
{
    setAvaliableJoystick();
}

void JoystickDialog::connectToJoystick()
{
    int joyComboBox = ui->joystickComboBox->currentIndex();

    if(joyComboBox != -1)
    {
        joyId = joyComboBox;

        if(joyAdapter->open(joyId))
        {
            connect(joyAdapter, SIGNAL(sigButtonChanged(int, bool)), this, SLOT(buttonSetup(int,bool)));
            connect(joyAdapter, SIGNAL(sigAxisChanged(int,int)), this, SLOT(axisSetup(int,int)));
            connect(joyAdapter, SIGNAL(sigHatChanged(int,int)), this, SLOT(hatSetup(int,int)));
            connect(joyAdapter, SIGNAL(sigBallChanged(int,int,int)), this, SLOT(ballSetup(int,int,int)));
        }

        QString joyName = joyAdapter->getJoystickName();
        if(joyName == "")
            joyName = "Not connected";
        ui->joystickNameLabel->setText(joyName);
        ui->joystickIdLabel->setText(tr("%1").arg(joyAdapter->getJoystickId()));
        ui->joystickAxisLabel->setText(tr("%1").arg(joyAdapter->getJoystickNumAxes()));
        ui->joystickHatsLabel->setText(tr("%1").arg(joyAdapter->getJoystickNumHats()));
        ui->joystickBallsLabel->setText(tr("%1").arg(joyAdapter->getJoystickNumBalls()));
        ui->joystickButtonsLabel->setText(tr("%1").arg(joyAdapter->getJoystickNumButtons()));

        ui->joystickStateBox->setEnabled(true);
        ui->joystickInformationBox->setEnabled(true);
        ui->connectPushButton->setDisabled(true);
        ui->disconnectPushButton->setEnabled(true);
        ui->joystickComboBox->setDisabled(true);
        ui->rescanPushButton->setDisabled(true);
    }
}

void JoystickDialog::disconnectFromJoystick()
{
    if(joyAdapter->isConnected())
    {
        joyAdapter->close();
        disconnect(joyAdapter, SIGNAL(sigButtonChanged(int, bool)), this, SLOT(buttonSetup(int,bool)));
        disconnect(joyAdapter, SIGNAL(sigAxisChanged(int,int)), this, SLOT(axisSetup(int,int)));
        disconnect(joyAdapter, SIGNAL(sigHatChanged(int,int)), this, SLOT(hatSetup(int,int)));
        disconnect(joyAdapter, SIGNAL(sigBallChanged(int,int,int)), this, SLOT(ballSetup(int,int,int)));
    }

    ui->joystickStateBox->setDisabled(true);
    ui->joystickInformationBox->setDisabled(true);

    ui->connectPushButton->setEnabled(true);
    ui->disconnectPushButton->setDisabled(true);
    ui->joystickComboBox->setEnabled(true);
    ui->rescanPushButton->setEnabled(true);

    setDefaultText();
    setAvaliableJoystick();
}

void JoystickDialog::axisSetup(int id, int state)
{
    switch(id)
    {
    case 0:
        ui->joystickXaxisLabel->setText(tr("%1").arg(state));
        break;
    case 1:
        ui->joystickYaxisLabel->setText(tr("%1").arg(-1*state));
        break;
    case 2:
        ui->joystickZLTaxisLabel->setText(tr("%1").arg(state));
        break;
    case 3:
        ui->joystickXrotationLabel->setText(tr("%1").arg(state));
        break;
    case 4:
        ui->joystickYrotationLabel->setText(tr("%1").arg(-1*state));
        break;
    case 5:
        ui->joystickZRTaxisLabel->setText(tr("%1").arg(state));
        break;
    }
}

void JoystickDialog::hatSetup(int id, int state)
{
    Q_UNUSED(id)

    ui->joystickPOV0Label->setText(tr("%1").arg(state));
}

void JoystickDialog::buttonSetup(int id, bool state)
{
    buttonVector[id] = state;

    QString buttonTest = "";
    for(int i = 0; i < MAX_JOYSTICK_BUTTONS; ++i)
    {
        if(buttonVector[i] == true)
        {
            if(i < 10)
                buttonTest += tr("0%1  ").arg(i);
            else
                buttonTest += tr("%1").arg(i);
        }
    }
    ui->joystickButtonsTestLabel->setText(buttonTest);
}

void JoystickDialog::ballSetup(int id, int stateX, int stateY)
{
    Q_UNUSED(id)
    Q_UNUSED(stateX)
    Q_UNUSED(stateY)
}

void JoystickDialog::on_JoystickDialog_accepted()
{
    disconnectFromJoystick();
    idHolder->setJoyId(joyId);
}

void JoystickDialog::on_JoystickDialog_rejected()
{
    disconnectFromJoystick();
}
