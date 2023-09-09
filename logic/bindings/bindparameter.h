#ifndef BINDPARAMETER_H
#define BINDPARAMETER_H

#include <QShortcut>
#include "../joystick/joystate.h"

enum BindingType {
    KEYBOARD,
    JOYSTICK
};

// Base class BindParam
class BindParameter : public QWidget
{
    Q_OBJECT

public:
    explicit BindParameter(BindingType type, QWidget *parent = nullptr)
        : QWidget(parent), m_type(type)
    {
    }

    virtual ~BindParameter() {}

    BindingType getType() const
    {
        return m_type;
    }

    virtual QObject* getSignalSource() const = 0;
    virtual const char* getSignal() const = 0;

signals:
    void triggered();

private:
    BindingType m_type;
};


class BindParamKeyboard : public BindParameter
{
    Q_OBJECT

    QShortcut *shortcut;
    QKeySequence m_keySequence;

public:
    explicit BindParamKeyboard(QString keySequence, QWidget *parent) :
        BindParameter(KEYBOARD, parent),
        m_keySequence( QKeySequence::fromString(keySequence))
    {
        shortcut = new QShortcut(m_keySequence, parent);
//        connect(m_shortcut, SIGNAL(activated()), this, SIGNAL(triggered()));
    }
    QObject* getSignalSource() const override
    {
        return shortcut;
    }

    const char* getSignal() const override
    {
        return SIGNAL(activated());
    }
};


class BindParamJoy : public BindParameter
{
    Q_OBJECT
    JoyState &joy;
    QList<QStringList> commands;
    QList<QStringList> splitCommand(const QString& input);
public:
    explicit BindParamJoy(QString joyCommand, JoyState &joyState, QWidget *parent = nullptr)
        : BindParameter(JOYSTICK, parent),
        joy(joyState)
    {
        commands = splitCommand(joyCommand);
//        connect(m_source, m_signal, this, SIGNAL(triggered()));
    }

    QObject* getSignalSource() const override
    {
        return m_source;
    }

    const char* getSignal() const override
    {
        return m_signal;
    }

private:
    QObject *m_source;
    const char *m_signal;

    void getSignalByString(const QString &signalName) const;

//    static QHash<QString, int> signalIndexMap = {
//                      { "JOY_X_AXIS", joystickXaxisChanged(int)},
//                      { "JOY_Y_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickYaxisChanged(int)")},
//                      { "JOY_ZLT_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickZLTaxisChanged(int)")},
//                      { "JOY_ZRT_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickZRTaxisChanged(int)")},
//                      { "JOY_XROT_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickXrotationChanged(int)")},
//                      { "JOY_YROT_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickYrotationChanged(int)")},
//                      { "JOY_POV0_AXIS", JoyState::staticMetaObject.indexOfSignal("joystickPOV0Changed(int)")},

//                      { "0", JoyState::staticMetaObject.indexOfSignal("button0Changed(bool)") },
//                      { "1", JoyState::staticMetaObject.indexOfSignal("button1Changed(bool)") },
//                      { "2", JoyState::staticMetaObject.indexOfSignal("button2Changed(bool)") },
//                      { "3", JoyState::staticMetaObject.indexOfSignal("button3Changed(bool)") },
//                      { "4", JoyState::staticMetaObject.indexOfSignal("button4Changed(bool)") },
//                      { "5", JoyState::staticMetaObject.indexOfSignal("button5Changed(bool)") },
//                      { "6", JoyState::staticMetaObject.indexOfSignal("button6Changed(bool)") },
//                      { "7", JoyState::staticMetaObject.indexOfSignal("button7Changed(bool)") },
//                      { "8", JoyState::staticMetaObject.indexOfSignal("button8Changed(bool)") },
//                      { "9", JoyState::staticMetaObject.indexOfSignal("button9Changed(bool)") },

//                      { "10", JoyState::staticMetaObject.indexOfSignal("button10Changed(bool)") },
//                      { "11", JoyState::staticMetaObject.indexOfSignal("button11Changed(bool)") },
//                      { "12", JoyState::staticMetaObject.indexOfSignal("button12Changed(bool)") },
//                      { "13", JoyState::staticMetaObject.indexOfSignal("button13Changed(bool)") },
//                      { "14", JoyState::staticMetaObject.indexOfSignal("button14Changed(bool)") },
//                      { "15", JoyState::staticMetaObject.indexOfSignal("button15Changed(bool)") },
//                      { "16", JoyState::staticMetaObject.indexOfSignal("button16Changed(bool)") },
//                      { "17", JoyState::staticMetaObject.indexOfSignal("button17Changed(bool)") },
//                      { "18", JoyState::staticMetaObject.indexOfSignal("button18Changed(bool)") },
//                      { "19", JoyState::staticMetaObject.indexOfSignal("button19Changed(bool)") },

//                      { "20", JoyState::staticMetaObject.indexOfSignal("button20Changed(bool)") },
//                      { "21", JoyState::staticMetaObject.indexOfSignal("button21Changed(bool)") },
//                      { "22", JoyState::staticMetaObject.indexOfSignal("button22Changed(bool)") },
//                      { "23", JoyState::staticMetaObject.indexOfSignal("button23Changed(bool)") },
//                      { "24", JoyState::staticMetaObject.indexOfSignal("button24Changed(bool)") },
//                      { "25", JoyState::staticMetaObject.indexOfSignal("button25Changed(bool)") },
//                      { "26", JoyState::staticMetaObject.indexOfSignal("button26Changed(bool)") },
//                      { "27", JoyState::staticMetaObject.indexOfSignal("button27Changed(bool)") },
//                      { "28", JoyState::staticMetaObject.indexOfSignal("button28Changed(bool)") },
//                      { "29", JoyState::staticMetaObject.indexOfSignal("button29Changed(bool)") },
//                      };
};


#endif // BINDPARAMETER_H
