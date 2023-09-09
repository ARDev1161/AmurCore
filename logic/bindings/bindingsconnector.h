#ifndef BINDINGSCONNECTOR_H
#define BINDINGSCONNECTOR_H

#include <QObject>
#include "../joystick/joystate.h"
#include "bindingsloader.h"
#include "bindparameter.h"

class BindingsConnector : public QWidget
{
    Q_OBJECT
    QWidget* parentWidget;
    int loadingState = 0;

    JoyState &joyState;

    BindingsLoader joy; // Loader object for bindings from joystick config
    QHash<QString, QVariant> joyBindings; // Hash table from loaded config
    QHash<QString, BindParamJoy*> joyBindParam; // Hash table for proccessing parameters

    BindingsLoader keyboard; // Loader object for bindings from keyboard config
    QHash<QString, QVariant> keyboardBindings; // Hash table from loaded config
    QHash<QString, BindParamKeyboard*> keyboardBindParams; // Hash table for proccessing parameters

    // TODO table for connection command to functions
    int keyBindCreate();
    int joyBindCreate();
public:
    explicit BindingsConnector(QString configBindKeyboard, QString configBindJoystick, JoyState &joyState, QWidget *parent = nullptr);
    int loadBindings();

    int getLoadingState() const;

signals:

};

#endif // BINDINGSCONNECTOR_H
