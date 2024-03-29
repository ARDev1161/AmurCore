/*
 *  Copyright (c) 2011 Evgeny Proydakov <lord.tiran@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __V_JOYSTICK_ADAPTER_H
#define __V_JOYSTICK_ADAPTER_H

#include <QThread>
#include <QString>
#include <QStringList>
#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_haptic.h>

class VJoystickAdapter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VJoystickAdapter)

public:
    enum HatPosition
    {
        JOYSTICK_HAT_CENTERED = SDL_HAT_DOWN,
        JOYSTICK_HAT_UP = SDL_HAT_UP,
        JOYSTICK_HAT_UP_RIGHT = SDL_HAT_RIGHT,
        JOYSTICK_HAT_RIGHT = SDL_HAT_RIGHT,
        JOYSTICK_HAT_RIGHT_DOWN = SDL_HAT_RIGHTDOWN,
        JOYSTICK_HAT_DOWN = SDL_HAT_DOWN,
        JOYSTICK_HAT_DOWN_LEFT = SDL_HAT_LEFTDOWN,
        JOYSTICK_HAT_LEFT = SDL_HAT_LEFT,
        JOYSTICK_HAT_LEFT_UP = SDL_HAT_LEFTUP
    };

public:
    explicit VJoystickAdapter(QObject *parent = 0);
    ~VJoystickAdapter();

    bool open(int id);
    void close();
    bool isConnected() const
    {
        return SDL_JoystickGetAttached(m_joystick);
    }

    inline int getJoystickId() const
    {
        return SDL_JoystickGetPlayerIndex(m_joystick);
    }
    inline QString getJoystickName() const
    {
        return QString(SDL_JoystickName(m_joystick));
    }
    inline int getJoystickNumAxes() const
    {
        return SDL_JoystickNumAxes(m_joystick);
    }
    inline int getJoystickNumHats() const
    {
        return SDL_JoystickNumHats(m_joystick);
    }
    inline int getJoystickNumBalls() const
    {
        return SDL_JoystickNumBalls(m_joystick);
    }
    inline int getJoystickNumButtons() const
    {
        return SDL_JoystickNumButtons(m_joystick);
    }

    int test_haptic();
    static int getNumAvailableJoystick();
    static QStringList getAvailableJoystickName();

signals:
    void sigButtonChanged(int id, bool state);
    void sigAxisChanged(int id, int state);
    void sigHatChanged(int id, int state);
    void sigBallChanged(int id, int stateX, int stateY);

private:
    class               VJoystickThread;

    SDL_Joystick*       m_joystick;
    VJoystickThread*    m_joystickThread;
};

class VJoystickAdapter::VJoystickThread : public QThread
{
    Q_OBJECT

public:
    inline VJoystickThread(VJoystickAdapter* adapter) : m_adapter(adapter) { }

protected:
    virtual void run();

private:
    VJoystickAdapter *m_adapter;
};

#endif // __V_JOYSTICK_ADAPTER_H
