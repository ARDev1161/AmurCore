# Amur Core
This is application for communicate with robot on RPi base with [AmurClient](https://github.com/ARDev1161/AmurClient).

## Requirements:
  1) Qt 5.12.2 + qmake
  2) OpenCV 4.0.1
  3) Protobuf 3.7.0
  4) SDL 2.0
  5) PocketSphinx+SphinxBase 5prealpha-2
  6) qt5-speech 5.12.2-1

## Sensors of robot:
  1) Camera - OV5647
  2) Accelerometer & Gyroscope & Magnitometer - MPU-9250
  3) Barometer - BMP-280
  4) Encoders: left wheel, right wheel & hands 
  5) Gas sensor - MQ-7
  6) Battery voltage

## Controls of Robot:
  1) Camera servoX, servoY - MG90S
  2) Motors: left wheel, right wheel & hand


[AmurCore]:https://github.com/ARDev1161/AmurCore


## Experimental RC API v2 capability discovery (feature branch)

This branch **preserves the legacy gRPC services and all existing map/Navigation/GStreamer messages**.
`Sensors.robot_info = 9` is an optional additive field. When a v2-aware
`rc_agent` sends it, open **Robot → Robot Info** to inspect the advertised
robot ID, API version, capabilities and supported velocity axes. This is
currently single-session inspection; multi-robot session isolation is P3 in
[PLAN.md](PLAN.md). No dynamic UI factory or new motion command transport
has been enabled.

See [rc_agent/robot.cfg.in](https://github.com/ARDev1161/rc_agent/blob/feat/rc-api-v2/robot.cfg.in)
for capability declaration. Empty `Amur.API.robot_id` leaves v2 advertisement
disabled to preserve existing deployments. For a robot with a configured ID,
legacy controls, Nav2 map and video continue to use the old wire fields.
