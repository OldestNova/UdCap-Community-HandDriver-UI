#pragma once

#include <openvr_driver.h>
#include <array>

// End frame of the right-hand thumb in the installed UdcapDriver v0.1.8.6
// glove_anim.glb. The open frame matches HandReferencePose.h. Only the three
// thumb joint orientations and the moving thumb root position are needed.
inline constexpr std::array<vr::HmdQuaternionf_t, 3> handThumbClosedRight{{
    {0.474332601f, -0.276037365f, 0.821764708f, -0.153344914f},
    {0.855004609f, -0.040090300f, -0.074431054f, 0.511683405f},
    {0.796900272f, 0.001514574f, 0.000356051f, 0.604108870f},
}};
inline constexpr std::array<float, 3> handThumbClosedRootRight{
    0.016305087f, 0.027528726f, 0.017799662f};
