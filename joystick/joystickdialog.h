#ifndef JOYSTICKDIALOG_H
#define JOYSTICKDIALOG_H

#include <QDialog>
#include <QVector>

#include "v_joystick_adapter.h"
#include "joystickidholder.h"

namespace Ui {
class JoystickDialog;
}

class JoystickDialog : public QDialog
{
    Q_OBJECT

public:
    explicit JoystickDialog(JoystickIdHolder *idHolder, QWidget *parent = 0);
    ~JoystickDialog();

private:
    Ui::JoystickDialog *ui;

    VJoystickAdapter* joyAdapter;
    QVector<bool> buttonVector;

    JoystickIdHolder *idHolder;

    int joyId;

    enum { MAX_JOYSTICK_BUTTONS = 30 };

    void setDefaultText();
    void setAvaliableJoystick();
    void initFields();
    void connMenu();

private slots:
    void connectToJoystick();
    void disconnectFromJoystick();
    void rescanJoystickDevice();

    void axisSetup(int id, int state);
    void hatSetup(int id, int state);
    void buttonSetup(int id, bool state);
    void ballSetup(int id, int stateX, int stateY);

    void on_JoystickDialog_accepted();
    void on_JoystickDialog_rejected();
};

#endif // JOYSTICKDIALOG_H
