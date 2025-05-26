//
// Created by max_3 on 25-5-24.
//

#ifndef BASEFRAME_H
#define BASEFRAME_H
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

class DraggableBaseFrame: public wxFrame {
public:

    DraggableBaseFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style);
    bool mDragging = false;
    wxPoint mDragStartMouse;
    wxPoint mDragStartWindow;
	wxStaticText* mTitle;
	wxBitmapButton* mBtnExit;
	wxBitmapButton* mBtnMenu;
	wxBoxSizer* bSizer1;
	wxPanel* mTitleBar;

    void mTitleBarOnMouseCaptureLost(wxMouseCaptureLostEvent&);
    void mTitleBarOnLeftDown( wxMouseEvent& event );
    void mTitleBarOnLeftUp( wxMouseEvent& event );
    void mTitleBarOnMotion( wxMouseEvent& event );
	void mBtnExitOnButtonClick( wxCommandEvent& event );
	virtual bool showBtnMenu() { return false; }
	virtual void mBtnMenuOnButtonClick( wxCommandEvent& event ) {};
	void SetSizer(wxSizer *sizer);
};

#endif //BASEFRAME_H
