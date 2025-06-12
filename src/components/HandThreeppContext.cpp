//
// Created by max_3 on 2025/6/7.
//

#include "HandThreeppContext.h"


HandThreeppContext::HandThreeppContext(threepp::PeripheralsEventSource &evt)
        : renderer(evt.size()), camera(threepp::PerspectiveCamera(75, evt.size().aspect())), orbitControls(camera, evt) {

    scene.background = threepp::Color::aliceblue;
    camera.position.z = 5;

    const auto boxGeometry = threepp::BoxGeometry::create();
    const auto boxMaterial = threepp::MeshBasicMaterial::create();
    boxMaterial->color.setRGB(1, 0, 0);
    boxMaterial->transparent = true;
    boxMaterial->opacity = 0.1f;
    auto box = threepp::Mesh::create(boxGeometry, boxMaterial);

    auto wiredBox = threepp::LineSegments::create(threepp::WireframeGeometry::create(*boxGeometry));
    wiredBox->material()->as<threepp::LineBasicMaterial>()->depthTest = false;
    wiredBox->material()->as<threepp::LineBasicMaterial>()->color = threepp::Color::gray;
    box->add(wiredBox);
    scene.add(box);
}

void HandThreeppContext::loop() {
    static threepp::Clock clock;
    float dt = clock.getDelta();

    scene.children[0]->rotation.y += 1.f * dt;

    renderer.render(scene, camera);
}

void HandThreeppContext::onWindowResize(threepp::WindowSize size) {
    camera.aspect = size.aspect();
    camera.updateProjectionMatrix();
    renderer.setSize(size);
}
