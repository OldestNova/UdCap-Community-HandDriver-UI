//
// Created by max_3 on 25-5-24.
//

#ifndef HANDGL3D_HPP
#define HANDGL3D_HPP

#include "wx/wx.h"
#include "wx/glcanvas.h"
#include "threepp/threepp.hpp"
class ThreeppContext {
public:
    explicit ThreeppContext(threepp::PeripheralsEventSource &evt);
    void loop();
    void onWindowResize(threepp::WindowSize size);
private:
    threepp::GLRenderer renderer;
    threepp::Scene scene;
    threepp::PerspectiveCamera camera;
    threepp::OrbitControls orbitControls;
};
class HandGL3D : public wxGLCanvas, threepp::PeripheralsEventSource {
private:
    std::unique_ptr<wxGLContext> openGLContext;
    std::unique_ptr<ThreeppContext> threeppContext;
public:
    HandGL3D(wxWindow *parent,
               wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0);
    virtual ~HandGL3D();

    void init();

     void OnPaint(wxPaintEvent &event);
     void OnSize(wxSizeEvent &event);
     void OnMouseMove(wxMouseEvent &event);
     void OnMousePress(wxMouseEvent &event);
     void OnMouseRelease(wxMouseEvent &event);
     void OnMouseWheel(wxMouseEvent &event);
     void OnInternalIdle() override;
    threepp::WindowSize size() const override;
};



#endif //HANDGL3D_HPP
