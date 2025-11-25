#ifndef NAVIGATION_ADAPTER_H
#define NAVIGATION_ADAPTER_H

#include <QPointF>
#include "network/protobuf/robot.pb.h"

// Simple DTO helpers to decouple UI from proto schema
namespace NavigationAdapter {

inline Navigation::Pose* fromPoint(const QPointF& point, Navigation::Pose* pose)
{
    if (!pose)
        return nullptr;

    pose->set_x(point.x());
    pose->set_y(point.y());
    pose->set_z(0.0);

    pose->set_orientation_w(1.0);
    pose->set_orientation_x(0.0);
    pose->set_orientation_y(0.0);
    pose->set_orientation_z(0.0);

    return pose;
}

} // namespace NavigationAdapter

#endif // NAVIGATION_ADAPTER_H
