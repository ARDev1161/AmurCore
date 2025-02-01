#ifndef JOYSTICKDIALOG_H
#define JOYSTICKDIALOG_H

#include <QDialog>
#include <QVector>

#include "v_joystick_adapter.h"
#include "getstatebyjoy.h"

namespace Ui {
class JoystickDialog;
}

class JoystickDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JoystickDialog(const std::shared_ptr<JoyState> state, QWidget *parent = 0);
    ~JoystickDialog();

private:
    Ui::JoystickDialog *ui;
    QVector<bool> buttonVector;
    enum { MAX_JOYSTICK_BUTTONS = 30 };

    std::shared_ptr<JoyState> joyState;
    VJoystickAdapter* joyAdapter;

    int joyId;

    void setDefaultText();
    void initFields();
    void connMenu();
    void setDialog();
    int loadJoy(int joyId);

private slots:
    int connectToJoystick();
    void disconnectFromJoystick();
    void scanJoystickDevice();

    void axisSetup(int id, int state);
    void hatSetup(int id, int state);
    void buttonSetup(int id, bool state);
    void ballSetup(int id, int stateX, int stateY);

    void on_JoystickDialog_accepted();
    void on_JoystickDialog_rejected();
};

#endif // JOYSTICKDIALOG_H
