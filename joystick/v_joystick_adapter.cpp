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

#include "v_joystick_adapter.h"

VJoystickAdapter::VJoystickAdapter(QObject *parent) :
    QObject(parent),
    m_joystick(0)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
    m_joystickThread = new VJoystickThread(this);
}

VJoystickAdapter::~VJoystickAdapter()
{
    if(isConnected())
        close();
    delete m_joystickThread;
    SDL_JoystickEventState(SDL_DISABLE);
    SDL_Quit();
}

bool VJoystickAdapter::open(int id)
{
    Q_ASSERT(!m_joystick);

    if(SDL_JoystickGetAttached(m_joystick))
        return false;
    m_joystick = SDL_JoystickOpen(id);
    if(SDL_JoystickGetAttached(m_joystick))
    {
        test_haptic();
        m_joystickThread->start();
    }
    else
        m_joystick = 0;

    return m_joystick;
}

void VJoystickAdapter::close()
{
    if(m_joystick)
    {
        SDL_Event closeEvent;

        closeEvent.type = SDL_QUIT;
        SDL_PushEvent(&closeEvent);

        m_joystickThread->wait();
        SDL_JoystickClose(m_joystick);
        m_joystick = 0;
    }
}

void VJoystickAdapter::VJoystickThread::run()
{
    SDL_Event joyEvent;
    bool running = true;

    while(running)
    {
        SDL_WaitEvent(&joyEvent);

        if(joyEvent.type == SDL_QUIT)
            running = false;
        else if(joyEvent.jbutton.which == m_adapter->getJoystickId())
        {
            switch(joyEvent.type)
            {
            case SDL_JOYAXISMOTION:
                emit m_adapter->sigAxisChanged(joyEvent.jaxis.axis,joyEvent.jaxis.value);
                break;

            case SDL_JOYHATMOTION:
                emit m_adapter->sigHatChanged(joyEvent.jhat.hat, joyEvent.jhat.value);
                break;

            case SDL_JOYBALLMOTION:
                emit m_adapter->sigBallChanged(joyEvent.jball.ball, joyEvent.jball.xrel, joyEvent.jball.yrel);
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                emit m_adapter->sigButtonChanged(joyEvent.jbutton.button, joyEvent.jbutton.state);
                break;
            }
        }
    }
}

int VJoystickAdapter::getNumAvailableJoystick()
{
    SDL_JoystickUpdate();
    return SDL_NumJoysticks();
}

QStringList VJoystickAdapter::getAvailableJoystickName()
{
    QStringList joyNames;

    int joyNum = getNumAvailableJoystick();

    for(int i = 0; i < joyNum; ++i)
        joyNames.push_front(QString(SDL_JoystickNameForIndex(i)));

    return joyNames;
}

int VJoystickAdapter::test_haptic() {
 SDL_Haptic *haptic;
 SDL_HapticEffect effect;
 int effect_id;

 // Open the device
 haptic = SDL_HapticOpenFromJoystick( m_joystick );
 if (haptic == NULL) return -1; // Most likely joystick isn't haptic

 // See if it can do sine waves
 if ((SDL_HapticQuery(haptic) & SDL_HAPTIC_SINE)==0) {
  SDL_HapticClose(haptic); // No sine effect
  return -1;
 }

 // Create the effect
 SDL_memset( &effect, 0, sizeof(SDL_HapticEffect) ); // 0 is safe default
 effect.type = SDL_HAPTIC_SINE;
 effect.periodic.direction.type = SDL_HAPTIC_POLAR; // Polar coordinates
 effect.periodic.direction.dir[0] = 18000; // Force comes from south
 effect.periodic.period = 1000; // 1000 ms
 effect.periodic.magnitude = 20000; // 20000/32767 strength
 effect.periodic.length = 5000; // 5 seconds long
 effect.periodic.attack_length = 1000; // Takes 1 second to get max strength
 effect.periodic.fade_length = 1000; // Takes 1 second to fade away

 // Upload the effect
 effect_id = SDL_HapticNewEffect( haptic, &effect );

 // Test the effect
 SDL_HapticRunEffect( haptic, effect_id, 1 );
 SDL_Delay( 5000); // Wait for the effect to finish

 // We destroy the effect, although closing the device also does this
 SDL_HapticDestroyEffect( haptic, effect_id );

 // Close the device
 SDL_HapticClose(haptic);

 return 0; // Success
}
