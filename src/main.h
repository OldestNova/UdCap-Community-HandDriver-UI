//
// Created by max_3 on 25-5-23.
//

#ifndef MAIN_H
#define MAIN_H
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

class UdCapCommunityDriverUI: public wxApp {
public:
    virtual bool OnInit() override;
};
//
// class UdCapCommunityDriverMainFrame: public wxFrame {
// public:
//     UdCapCommunityDriverMainFrame();
//     virtual ~UdCapCommunityDriverMainFrame() override;
// private:
//     void OnMouseLeftDown(wxMouseEvent& event);
//     void OnMouseLeftUp(wxMouseEvent&);
//     void OnMouseMotion(wxMouseEvent& event);
//     void OnMouseCaptureLost(wxMouseCaptureLostEvent&);
//     void OnResize(wxSizeEvent& event);
//     void OnCloseButton(wxCommandEvent& event);
//     void FinishDrag();
//     void UpdateCloseButton();
//     bool mDragging;
//     wxPoint mDragStartMouse;
//     wxPoint mDragStartWindow;
//
//     wxButton* mCloseButton;
// };

wxIMPLEMENT_APP(UdCapCommunityDriverUI);

#endif //MAIN_H
