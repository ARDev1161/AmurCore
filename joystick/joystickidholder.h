#ifndef JOYSTICKIDHOLDER_H
#define JOYSTICKIDHOLDER_H


class JoystickIdHolder
{
public:
    JoystickIdHolder();
    int getJoyId() const;
    void setJoyId(int value);

private:
    int joyId;
};

#endif // JOYSTICKIDHOLDER_H
