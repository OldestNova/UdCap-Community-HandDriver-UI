//
// Created by max_3 on 2025/6/7.
//

#include "HandThreeppContext.h"
#include "PoseCoordinates.h"
#include <cmath>

namespace {

// Thumb, index, middle, ring, little. Each finger has its own proportions.
constexpr std::array<std::array<float, 3>, 5> fingerLengths{{
    {0.25f, 0.19f, 0.14f},
    {0.35f, 0.25f, 0.18f},
    {0.40f, 0.28f, 0.20f},
    {0.37f, 0.26f, 0.18f},
    {0.30f, 0.21f, 0.15f},
}};
constexpr std::array<float, 5> fingerWidths{0.14f, 0.12f, 0.13f, 0.12f, 0.10f};
constexpr std::array<float, 5> fingerRootY{-0.30f, -0.02f, 0.0f, -0.01f, -0.06f};

threepp::Quaternion previewRotation(const BoneQuaternion &bone, UdTarget hand,
                                    std::size_t finger, std::size_t joint) {
    const auto q = poseForPreview(bone, hand, finger, joint);
    return threepp::Quaternion(q.x, q.y, q.z, q.w);
}

} // namespace


HandThreeppContext::HandThreeppContext(threepp::PeripheralsEventSource &evt)
        : renderer(evt.size()), camera(threepp::PerspectiveCamera(75, evt.size().aspect())), orbitControls(camera, evt) {

    scene.background = threepp::Color::aliceblue;
    camera.position.z = 5;

    for (int hand = 0; hand < 2; ++hand) {
        // The hands face forward in top view. Both thumbs point toward the
        // center, and index-to-little runs from center to the outside.
        const float side = hand == 0 ? -1.0f : 1.0f;
        auto root = threepp::Group::create();
        root->position.x = hand == 0 ? -1.1f : 1.1f;
        auto material = threepp::MeshBasicMaterial::create();
        material->color.setRGB(hand == 0 ? 0.18f : 0.9f, 0.45f, hand == 0 ? 0.9f : 0.25f);
        auto palm = threepp::Mesh::create(threepp::BoxGeometry::create(0.8f, 0.75f, 0.18f), material);
        palm->position.y = -0.42f;
        root->add(palm);

        FingerNodes &nodes = hand == 0 ? leftFingers : rightFingers;
        for (int finger = 0; finger < 5; ++finger) {
            auto parent = root;
            if (finger == 0) {
                // Keep the thumb's neutral direction separate from live bone
                // rotation so an identity pose still has an open, sideways thumb.
                auto thumbRest = threepp::Group::create();
                thumbRest->position.x = -side * 0.36f;
                thumbRest->position.y = fingerRootY[finger];
                thumbRest->rotation.z = side * 1.15f;
                root->add(thumbRest);
                parent = thumbRest;
            }
            for (int joint = 0; joint < 3; ++joint) {
                auto pivot = threepp::Group::create();
                if (joint == 0 && finger != 0) {
                    pivot->position.x = side * (finger - 2) * 0.18f;
                    pivot->position.y = fingerRootY[finger];
                } else if (joint > 0) {
                    pivot->position.y = fingerLengths[finger][joint - 1];
                }
                const float segmentLength = fingerLengths[finger][joint];
                auto mesh = threepp::Mesh::create(
                    threepp::BoxGeometry::create(fingerWidths[finger], segmentLength, 0.13f), material);
                mesh->position.y = segmentLength / 2.0f;
                pivot->add(mesh);
                parent->add(pivot);
                nodes[finger][joint] = pivot;
                parent = pivot;
            }
        }
        scene.add(root);
    }
}

void HandThreeppContext::loop() {
    orbitControls.update();
    renderer.render(scene, camera);
}

void HandThreeppContext::setHandPose(UdTarget hand, const HandQuaternion &pose) {
    FingerNodes &nodes = hand == UD_TARGET_LEFT_HAND ? leftFingers : rightFingers;
    const std::array<FingerQuaternion, 5> fingers{
        pose.thumbFinger, pose.indexFinger, pose.middleFinger,
        pose.ringFinger, pose.littleFinger
    };
    for (std::size_t finger = 0; finger < fingers.size(); ++finger) {
        const std::array<BoneQuaternion, 3> joints{
            fingers[finger].proximal, fingers[finger].intermediate, fingers[finger].distal
        };
        for (std::size_t joint = 0; joint < joints.size(); ++joint) {
            nodes[finger][joint]->quaternion.copy(previewRotation(joints[joint], hand,
                                                                   finger, joint));
        }
    }
}

void HandThreeppContext::onWindowResize(threepp::WindowSize size) {
    camera.aspect = size.aspect();
    camera.updateProjectionMatrix();
    renderer.setSize(size);
}
