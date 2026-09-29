//
// Created by max_3 on 2025/6/7.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H
#define UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H


#include <threepp/threepp.hpp>
#include <UdCapV1Core.h>
#include <array>

class HandThreeppContext {
public:
    explicit HandThreeppContext(threepp::PeripheralsEventSource &evt);
    void loop();
    void onWindowResize(threepp::WindowSize size);
    void setHandPose(UdTarget hand, const HandQuaternion &pose);
private:
    threepp::GLRenderer renderer;
    threepp::Scene scene;
    threepp::PerspectiveCamera camera;
    threepp::OrbitControls orbitControls;
    using FingerNodes = std::array<std::array<std::shared_ptr<threepp::Group>, 3>, 5>;
    FingerNodes leftFingers{};
    FingerNodes rightFingers{};
};

#endif //UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H
