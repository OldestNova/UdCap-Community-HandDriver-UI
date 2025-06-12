//
// Created by max_3 on 2025/6/7.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H
#define UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H


#include <threepp/threepp.hpp>

class HandThreeppContext {
public:
    explicit HandThreeppContext(threepp::PeripheralsEventSource &evt);
    void loop();
    void onWindowResize(threepp::WindowSize size);
private:
    threepp::GLRenderer renderer;
    threepp::Scene scene;
    threepp::PerspectiveCamera camera;
    threepp::OrbitControls orbitControls;
};

#endif //UDCAPCOMMUNITYDRIVERUI_HANDTHREEPPCONTEXT_H
