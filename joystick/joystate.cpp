#include "joystate.h"


JoyState::JoyState()
{
}

JoyState::JoyState(const JoyState& other)
    : QObject(other.parent()), joyId(other.joyId),
    buttonVector(other.buttonVector),
    joystickXaxis(other.joystickXaxis),
    joystickYaxis(other.joystickYaxis),
    joystickZLTaxis(other.joystickZLTaxis),
    joystickXrotation(other.joystickXrotation),
    joystickYrotation(other.joystickYrotation),
    joystickZRTaxis(other.joystickZRTaxis),
    joystickPOV0(other.joystickPOV0)
{
    // Set up a new parent object for signals and register with the meta system
    setParent(other.parent());
}

JoyState& JoyState::operator=(const JoyState& other)
{
    if (this == &other) // Checking for self-assignment
        return *this;

    // Copy data from other
    joyId = other.joyId;
    buttonVector = other.buttonVector;
    joystickXaxis = other.joystickXaxis;
    joystickYaxis = other.joystickYaxis;
    joystickZLTaxis = other.joystickZLTaxis;
    joystickXrotation = other.joystickXrotation;
    joystickYrotation = other.joystickYrotation;
    joystickZRTaxis = other.joystickZRTaxis;
    joystickPOV0 = other.joystickPOV0;

    // Set up a new parent object for signals and register with the meta system
    setParent(other.parent());

    return *this;
}

int JoyState::getJoyId() const
{
    return joyId;
}

void JoyState::setJoyId(int newJoyId)
{
    if (joyId == newJoyId)
        return;
    joyId = newJoyId;
    emit joyIdChanged(joyId);
}

QVector<bool> JoyState::getButtonVector() const
{
    return buttonVector;
}

void JoyState::setButtonVector(const QVector<bool> &newButtonVector)
{
    if (buttonVector == newButtonVector)
        return;
    buttonVector = newButtonVector;
    emit buttonVectorChanged(buttonVector);
}

bool JoyState::getButton(int id) const
{
    return buttonVector[id];
}

int JoyState::getJoystickXaxis() const
{
    return joystickXaxis;
}

void JoyState::setJoystickXaxis(int newJoystickXaxis)
{
    if (joystickXaxis == newJoystickXaxis)
        return;
    joystickXaxis = newJoystickXaxis;
    emit joystickXaxisChanged(joystickXaxis);
}

int JoyState::getJoystickYaxis() const
{
    return joystickYaxis;
}

void JoyState::setJoystickYaxis(int newJoystickYaxis)
{
    if (joystickYaxis == newJoystickYaxis)
        return;
    joystickYaxis = newJoystickYaxis;
    emit joystickYaxisChanged(joystickYaxis);
}

int JoyState::getJoystickZLTaxis() const
{
    return joystickZLTaxis;
}

void JoyState::setJoystickZLTaxis(int newJoystickZLTaxis)
{
    if (joystickZLTaxis == newJoystickZLTaxis)
        return;
    joystickZLTaxis = newJoystickZLTaxis;
    emit joystickZLTaxisChanged(joystickZLTaxis);
}

int JoyState::getJoystickXrotation() const
{
    return joystickXrotation;
}

void JoyState::setJoystickXrotation(int newJoystickXrotation)
{
    if (joystickXrotation == newJoystickXrotation)
        return;
    joystickXrotation = newJoystickXrotation;
    emit joystickXrotationChanged(joystickXrotation);
}

int JoyState::getJoystickYrotation() const
{
    return joystickYrotation;
}

void JoyState::setJoystickYrotation(int newJoystickYrotation)
{
    if (joystickYrotation == newJoystickYrotation)
        return;
    joystickYrotation = newJoystickYrotation;
    emit joystickYrotationChanged(joystickYrotation);
}

int JoyState::getJoystickZRTaxis() const
{
    return joystickZRTaxis;
}

void JoyState::setJoystickZRTaxis(int newJoystickZRTaxis)
{
    if (joystickZRTaxis == newJoystickZRTaxis)
        return;
    joystickZRTaxis = newJoystickZRTaxis;
    emit joystickZRTaxisChanged(joystickZRTaxis);
}

int JoyState::getJoystickPOV0() const
{
    return joystickPOV0;
}

void JoyState::setJoystickPOV0(int newJoystickPOV0)
{
    if (joystickPOV0 == newJoystickPOV0)
        return;
    joystickPOV0 = newJoystickPOV0;
    emit joystickPOV0Changed(joystickPOV0);
}

void JoyState::setButton(int id, bool value)
{
    if (buttonVector[id] == value)
        return;
    buttonVector[id] = value;
    emit buttonVectorChanged(buttonVector);

    switch (id) {
    case 0:
        emit button0Changed(value);
        break;
    case 1:
        emit button1Changed(value);
        break;
    case 2:
        emit button2Changed(value);
        break;
    case 3:
        emit button3Changed(value);
        break;
    case 4:
        emit button4Changed(value);
        break;
    case 5:
        emit button5Changed(value);
        break;
    case 6:
        emit button6Changed(value);
        break;
    case 7:
        emit button7Changed(value);
        break;
    case 8:
        emit button8Changed(value);
        break;
    case 9:
        emit button9Changed(value);
        break;
    case 10:
        emit button0Changed(value);
        break;
    case 11:
        emit button1Changed(value);
        break;
    case 12:
        emit button2Changed(value);
        break;
    case 13:
        emit button3Changed(value);
        break;
    case 14:
        emit button4Changed(value);
        break;
    case 15:
        emit button5Changed(value);
        break;
    case 16:
        emit button6Changed(value);
        break;
    case 17:
        emit button7Changed(value);
        break;
    case 18:
        emit button8Changed(value);
        break;
    case 19:
        emit button9Changed(value);
        break;
    case 20:
        emit button0Changed(value);
        break;
    case 21:
        emit button1Changed(value);
        break;
    case 22:
        emit button2Changed(value);
        break;
    case 23:
        emit button3Changed(value);
        break;
    case 24:
        emit button4Changed(value);
        break;
    case 25:
        emit button5Changed(value);
        break;
    case 26:
        emit button6Changed(value);
        break;
    case 27:
        emit button7Changed(value);
        break;
    case 28:
        emit button8Changed(value);
        break;
    case 29:
        emit button9Changed(value);
        break;
    default:
        break;
    }
}
