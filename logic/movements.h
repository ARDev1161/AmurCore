#ifndef MOVEMENTS_H
#define MOVEMENTS_H


class Movements
{
    JoyState *joyState;
    AMUR::AmurControls *controls;
public:
    Movements(JoyState *joyState, AMUR::AmurControls *controls);
    int joyTranslate();
};

#endif // MOVEMENTS_H
