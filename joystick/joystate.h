#ifndef JOYSTATE_H
#define JOYSTATE_H

#include <QObject>
#include <QHash>

class JoyState : public QObject
{
    Q_OBJECT

    int joyId = -1;

    QVector<bool> buttonVector; // Buttons state

    int joystickXaxis = 0;
    int joystickYaxis = 0;
    int joystickZLTaxis = 0;
    int joystickXrotation = 0;
    int joystickYrotation = 0;
    int joystickZRTaxis = 0;

    int joystickPOV0 = 0;
public:
    JoyState();
    JoyState(const JoyState& other);
    JoyState& operator=(const JoyState& other);

    int getJoyId() const;
    void setJoyId(int newJoyId);

    QVector<bool> getButtonVector() const;
    void setButtonVector(const QVector<bool> &newButtonVector);

    bool getButton(int id) const;
    void setButton(int id, bool value);

    int getJoystickXaxis() const;
    void setJoystickXaxis(int newJoystickXaxis);

    int getJoystickYaxis() const;
    void setJoystickYaxis(int newJoystickYaxis);

    int getJoystickZLTaxis() const;
    void setJoystickZLTaxis(int newJoystickZLTaxis);

    int getJoystickXrotation() const;
    void setJoystickXrotation(int newJoystickXrotation);

    int getJoystickYrotation() const;
    void setJoystickYrotation(int newJoystickYrotation);

    int getJoystickZRTaxis() const;
    void setJoystickZRTaxis(int newJoystickZRTaxis);

    int getJoystickPOV0() const;
    void setJoystickPOV0(int newJoystickPOV0);

signals:
    void joyIdChanged(int newJoyId);
    void joystickXaxisChanged(int newXaxis);
    void joystickYaxisChanged(int newYaxis);
    void joystickZLTaxisChanged(int newZLTaxis);
    void joystickXrotationChanged(int newXrotation);
    void joystickYrotationChanged(int newYrotation);
    void joystickZRTaxisChanged(int newZRTaxis);
    void joystickPOV0Changed(int newPOV0);

    void buttonVectorChanged(const QVector<bool>& newButtonVector);
    void button0Changed(bool pressed);
    void button1Changed(bool pressed);
    void button2Changed(bool pressed);
    void button3Changed(bool pressed);
    void button4Changed(bool pressed);
    void button5Changed(bool pressed);
    void button6Changed(bool pressed);
    void button7Changed(bool pressed);
    void button8Changed(bool pressed);
    void button9Changed(bool pressed);
    void button10Changed(bool pressed);
    void button11Changed(bool pressed);
    void button12Changed(bool pressed);
    void button13Changed(bool pressed);
    void button14Changed(bool pressed);
    void button15Changed(bool pressed);
    void button16Changed(bool pressed);
    void button17Changed(bool pressed);
    void button18Changed(bool pressed);
    void button19Changed(bool pressed);
    void button20Changed(bool pressed);
    void button21Changed(bool pressed);
    void button22Changed(bool pressed);
    void button23Changed(bool pressed);
    void button24Changed(bool pressed);
    void button25Changed(bool pressed);
    void button26Changed(bool pressed);
    void button27Changed(bool pressed);
    void button28Changed(bool pressed);
    void button29Changed(bool pressed);
};

#endif // JOYSTATE_H
