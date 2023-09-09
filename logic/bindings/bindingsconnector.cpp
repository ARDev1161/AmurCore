#include "bindingsconnector.h"


BindingsConnector::BindingsConnector(QString configBindKeyboard, QString configBindJoystick, JoyState &joyState, QWidget *parent)
    : QWidget{parent},
    parentWidget(parent),
    joyState(joyState),
    joy(configBindJoystick),
    keyboard(configBindKeyboard)
{
    loadBindings();
    keyBindCreate();
}

int BindingsConnector::loadBindings()
{
    int res = 0;

    keyboardBindings = keyboard.getBindTable();
    if(keyboardBindings.isEmpty())
        res += -1;

    joyBindings = joy.getBindTable();
    if(joyBindings.isEmpty())
        res += -2;

    loadingState = res;
    return res;
}

int BindingsConnector::getLoadingState() const
{
    return loadingState;
}

int BindingsConnector::keyBindCreate()
{
    QHash<QString, QVariant>::const_iterator i = keyboardBindings.constBegin();
    while (i != keyboardBindings.constEnd()) {
        BindParamKeyboard *bindParam = new BindParamKeyboard(i.value().toString(), parentWidget);
        keyboardBindParams.insert(i.key(), bindParam);
    }
    return 0;
}

int BindingsConnector::joyBindCreate()
{
    QHash<QString, QVariant>::const_iterator i = joyBindings.constBegin();
    while (i != joyBindings.constEnd()) {
        BindParamJoy *bindParam = new BindParamJoy(i.value().toString(), joyState, parentWidget);
        joyBindParam.insert(i.key(), bindParam);
    }
    return 0;

}
