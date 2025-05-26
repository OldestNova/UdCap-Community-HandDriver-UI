//
// Created by max_3 on 25-5-24.
//

#include "HandGL3D.hpp"


ThreeppContext::ThreeppContext(threepp::PeripheralsEventSource &evt)
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

    void ThreeppContext::loop() {
        static threepp::Clock clock;
        float dt = clock.getDelta();

        scene.children[0]->rotation.y += 1.f * dt;

        renderer.render(scene, camera);
    }

    void ThreeppContext::onWindowResize(threepp::WindowSize size) {
        camera.aspect = size.aspect();
        camera.updateProjectionMatrix();
        renderer.setSize(size);
    }



HandGL3D::HandGL3D(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               long style) :
   wxGLCanvas(parent, id, nullptr, pos, size, style)
{
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
    openGLContext = std::make_unique<wxGLContext>(this, nullptr, &ctxAttrs);

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    init();
}
 
HandGL3D::~HandGL3D()
{

}

void HandGL3D::init() {
    Bind(wxEVT_PAINT, &HandGL3D::OnPaint, this);
    Bind(wxEVT_SIZE, &HandGL3D::OnSize, this);

    Bind(wxEVT_MOTION, &HandGL3D::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &HandGL3D::OnMousePress, this);
    Bind(wxEVT_RIGHT_DOWN, &HandGL3D::OnMousePress, this);
    Bind(wxEVT_LEFT_UP, &HandGL3D::OnMouseRelease, this);
    Bind(wxEVT_RIGHT_UP, &HandGL3D::OnMouseRelease, this);
    Bind(wxEVT_MOUSEWHEEL, &HandGL3D::OnMouseWheel, this);

    SetCurrent(*openGLContext);
    threepp::PeripheralsEventSource* p = static_cast<threepp::PeripheralsEventSource*>(this);
    threeppContext = std::make_unique<ThreeppContext>(*p);

    //yes, raw pointer
    // auto button = new wxButton(this, wxID_ANY, "Click Me", wxPoint(10, 10), wxSize(150, 30));
    // button->Bind(wxEVT_BUTTON, &ThreeppContext::OnButtonClick, threeppContext.get());
}


void HandGL3D::OnPaint(wxPaintEvent &WXUNUSED(event)) {
    wxPaintDC dc(this);
    threeppContext->loop();
    SwapBuffers();
}

void HandGL3D::OnInternalIdle() {
    wxWindow::OnInternalIdle();
    Refresh(false);
}

threepp::WindowSize HandGL3D::size() const {
    auto viewPortSize = GetSize() * GetContentScaleFactor();
    return {viewPortSize.x, viewPortSize.y};
}

void HandGL3D::OnSize(wxSizeEvent &event) {
    auto viewPortSize = event.GetSize() * GetContentScaleFactor();
    threepp::WindowSize size{viewPortSize.x, viewPortSize.y};
    threeppContext->onWindowResize(size);

    event.Skip();
}

void HandGL3D::OnMouseMove(wxMouseEvent &event) {
    wxPoint pos = event.GetPosition();
    threepp::Vector2 mousePos(static_cast<float>(pos.x), static_cast<float>(pos.y));
    onMouseMoveEvent(mousePos);

    event.Skip();
}

void HandGL3D::OnMousePress(wxMouseEvent &event) {
    int buttonFlag = event.GetButton();
    wxPoint pos = event.GetPosition();
    int button = 0;
    if (wxMOUSE_BTN_LEFT == buttonFlag) {
        button = 0;
    } else if (wxMOUSE_BTN_RIGHT == buttonFlag) {
        button = 1;
    }
    threepp::Vector2 p{pos.x, pos.y};
    onMousePressedEvent(button, p, PeripheralsEventSource::MouseAction::PRESS);

    event.Skip();
}

void HandGL3D::OnMouseRelease(wxMouseEvent &event) {
    int buttonFlag = event.GetButton();
    wxPoint pos = event.GetPosition();
    int button = 0;
    if (wxMOUSE_BTN_LEFT == buttonFlag) {
        button = 0;
    } else if (wxMOUSE_BTN_RIGHT == buttonFlag) {
        button = 1;
    }
    threepp::Vector2 p{pos.x, pos.y};
    onMousePressedEvent(button, p, PeripheralsEventSource::MouseAction::RELEASE);

    event.Skip();
}

void HandGL3D::OnMouseWheel(wxMouseEvent &event) {
    int direction = event.GetWheelRotation() / 120;// 1 or -1
    int xoffset = 0;
    int yoffset = direction;

    onMouseWheelEvent({static_cast<float>(xoffset), static_cast<float>(yoffset)});

    event.Skip();
}
