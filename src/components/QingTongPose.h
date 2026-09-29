#pragma once
#include <UdCapV1Core.h>
#include <cmath>

// QingTongClient's protocol uses Unity Quaternion.Euler (Y*X*Z), then
// negates the Z component. This is a protocol-specific convention, not
// the newer WPF VMC Z*Y*X convention. Both hands use the same local values;
// DeviceName identifies the hand to the QingTong receiver.
inline BoneQuaternion qingTongEuler(double x, double y, double z) {
    constexpr double halfRadians = 3.14159265358979323846 / 360.0;
    const double sx = std::sin(x * halfRadians), cx = std::cos(x * halfRadians);
    const double sy = std::sin(y * halfRadians), cy = std::cos(y * halfRadians);
    const double sz = std::sin(z * halfRadians), cz = std::cos(z * halfRadians);
    return {float(cy*sx*cz + sy*cx*sz), float(sy*cx*cz - cy*sx*sz),
            float(sy*sx*cz - cy*cx*sz), float(cy*cx*cz + sy*sx*sz)};
}

inline HandQuaternion poseForQingTong(const std::array<double, 28> &a) {
    HandQuaternion q{};
    q.indexFinger = {qingTongEuler(a[21], -a[7], -a[6]),
                     qingTongEuler(0, 0, -a[5]), qingTongEuler(0, 0, -a[4])};
    q.middleFinger = {qingTongEuler(0, a[11], -a[10]),
                      qingTongEuler(0, 0, -a[9]), qingTongEuler(0, 0, -a[8])};
    q.ringFinger = {qingTongEuler(0, -a[15], -a[14]),
                    qingTongEuler(0, 0, -a[13]), qingTongEuler(0, 0, -a[12])};
    q.littleFinger = {qingTongEuler(a[22], -a[19], -a[18]),
                      qingTongEuler(0, 0, -a[17]), qingTongEuler(0, 0, -a[16])};
    q.thumbFinger = {qingTongEuler(-a[3], a[2], a[20]),
                     qingTongEuler(0, a[1], 0), qingTongEuler(0, a[0], 0)};
    return q;
}

inline HandQuaternion poseForQingTong(const std::array<double, 28> &a,
                                     UdTarget hand, const HandRotation &o) {
    // Convert local Euler offsets along with the measured axes, before the
    // protocol's Y*X*Z composition. Both hands still use QingTong's own basis.
    const double s = hand == UD_TARGET_LEFT_HAND ? -1 : 1;
    const auto make = [](double x, double y, double z, const BoneRotation &d,
                         double sx, double sy, double sz) {
        return qingTongEuler(x + sx*d.x, y + sy*d.y, z + sz*d.z);
    };
    HandQuaternion q{};
    const FingerRotation offsets[]{o.indexFinger,o.middleFinger,o.ringFinger,o.littleFinger};
    FingerQuaternion *fingers[]{&q.indexFinger,&q.middleFinger,&q.ringFinger,&q.littleFinger};
    for (int f = 0; f < 4; ++f) {
        const double yawSign = f == 0 ? s : f == 1 ? 1 : -s;
        const double x = f == 0 ? a[21] : f == 3 ? a[22] : 0;
        const double y = f == 1 ? a[11] : -a[7+4*f];
        *fingers[f] = {make(x,y,-a[6+4*f],offsets[f].proximal,1,yawSign,-s),
            make(0,0,-a[5+4*f],offsets[f].intermediate,1,yawSign,-s),
            make(0,0,-a[4+4*f],offsets[f].distal,1,yawSign,-s)};
    }
    q.thumbFinger = {make(-a[3],a[2],a[20],o.thumbFinger.proximal,-1/1.2,-s/0.3,s/0.3),
        make(0,a[1],0,o.thumbFinger.intermediate,-1,-s,s),
        make(0,a[0],0,o.thumbFinger.distal,-1,-s,s)};
    return q;
}
