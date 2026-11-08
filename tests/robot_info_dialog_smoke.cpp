// Offscreen Qt5 acceptance test for robot capability display and reconnect reset.
#include "logic/robotinfodialog.h"

#include <QApplication>
#include <QPlainTextEdit>
#include <QString>

#include <stdexcept>
#include <string>
#include <iostream>

namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
}  // namespace

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    try {
        RobotInfoDialog dialog;
        auto *view = dialog.findChild<QPlainTextEdit *>();
        require(view && view->isReadOnly(), "Capability view missing or editable");
        require(view->toPlainText().contains("No v2 robot descriptor"),
                "Legacy fallback missing");

        rc::api::v2::RobotInfo first;
        first.set_robot_id("diff-001");
        first.set_display_name("Hall robot");
        first.set_api_major(2);
        first.set_api_minor(0);
        auto *speed = first.add_capabilities();
        speed->set_id("manual.velocity");
        speed->add_velocity_axes("linear.x");
        speed->add_velocity_axes("angular.z");
        first.add_capabilities()->set_id("map.2d");
        dialog.setRobotInfo(first);

        QString text = view->toPlainText();
        require(text.contains("Hall robot") && text.contains("diff-001"),
                "Robot name or stable ID not visible");
        require(text.contains("RC API: 2.0") && text.contains("manual.velocity"),
                "Protocol or capability missing");
        require(text.contains("linear.x, angular.z") && text.contains("map.2d"),
                "Velocity axes or map capability missing");

        rc::api::v2::RobotInfo second;
        second.set_robot_id("dog-002");
        second.set_display_name("Dog");
        second.set_api_major(2);
        second.add_capabilities()->set_id("navigation.nav2");
        dialog.setRobotInfo(second);
        text = view->toPlainText();
        require(text.contains("dog-002") && text.contains("navigation.nav2"),
                "Robot selection replacement missing");
        require(!text.contains("diff-001") && !text.contains("manual.velocity"),
                "Stale previous robot descriptor shown");

        // Blank identity is legacy mode even when capabilities are populated.
        rc::api::v2::RobotInfo anonymous;
        anonymous.set_display_name("Anonymous");
        anonymous.set_api_major(2);
        anonymous.add_capabilities()->set_id("manual.velocity");
        dialog.setRobotInfo(anonymous);
        text = view->toPlainText();
        require(text.contains("No v2 robot descriptor") &&
                    !text.contains("dog-002") &&
                    !text.contains("manual.velocity") &&
                    !text.contains("Anonymous"),
                "Anonymous descriptor leaked stale or untrusted metadata");

        dialog.setRobotInfo(rc::api::v2::RobotInfo());
        text = view->toPlainText();
        require(text.contains("No v2 robot descriptor") &&
                    !text.contains("dog-002"),
                "Legacy/disconnected robot did not clear old metadata");
    } catch (const std::exception &e) {
        std::cerr << "RobotInfo Qt smoke failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Qt RobotInfo dynamic descriptor smoke: PASS" << std::endl;
    return 0;
}
