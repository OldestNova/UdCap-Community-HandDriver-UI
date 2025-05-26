//
// Created by max_3 on 25-5-24.
//
#include "BaseFrame.h"
#include "res/close.png.h"
#include "res/menu.png.h"

DraggableBaseFrame::DraggableBaseFrame(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &pos, const wxSize &size, long style): wxFrame(
        parent, id, title, pos, size, style) {


    bSizer1 = new wxBoxSizer( wxVERTICAL );
	bSizer1->SetMinSize( wxSize( 300,300 ) );
    mTitleBar = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,35 ), wxTAB_TRAVERSAL );
    mTitleBar->SetMinSize( wxSize( -1,35 ) );
    mTitleBar->SetMaxSize( wxSize( -1,35 ) );

    wxBoxSizer* bSizer3;
    bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	mBtnMenu = new wxBitmapButton( mTitleBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|wxBORDER_NONE );

	mBtnMenu->SetBitmap( menu_png_to_wx_bitmap() );
	mBtnMenu->SetBackgroundColour( wxColour( 29, 29, 29 ) );

	bSizer3->Add( mBtnMenu, 0, wxALL, 5 );

    mTitle = new wxStaticText( mTitleBar, wxID_ANY, _("UdCap Community Driver"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    mTitle->SetForegroundColour( wxColour( 255, 255, 255 ) );

    bSizer3->Add( mTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );

    wxBoxSizer* bSizer5;
    bSizer5 = new wxBoxSizer( wxVERTICAL );

    mBtnExit = new wxBitmapButton( mTitleBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|wxBORDER_NONE );

    mBtnExit->SetBitmap( close_png_to_wx_bitmap() );
    mBtnExit->SetBackgroundColour( wxColour( 29, 29, 29 ) );

    bSizer5->Add( mBtnExit, 0, wxALIGN_RIGHT|wxALL|wxRIGHT, 5 );


    bSizer3->Add( bSizer5, 1, 0, 5 );


    mTitleBar->SetSizer( bSizer3 );
    mTitleBar->Layout();
    bSizer1->Add( mTitleBar, 1, wxEXPAND, 5 );

    mDragging = false;
	this->SetSizer(bSizer1);

	this->Layout();

	this->Centre( wxBOTH );
	mTitleBar->Bind(wxEVT_LEFT_DOWN, &DraggableBaseFrame::mTitleBarOnLeftDown, this);
	mTitleBar->Bind(wxEVT_LEFT_UP, &DraggableBaseFrame::mTitleBarOnLeftUp, this);
	mTitleBar->Bind(wxEVT_MOTION, &DraggableBaseFrame::mTitleBarOnMotion, this);
	mTitleBar->Bind(wxEVT_MOUSE_CAPTURE_LOST, &DraggableBaseFrame::mTitleBarOnMouseCaptureLost, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &DraggableBaseFrame::mTitleBarOnMouseCaptureLost, this);

	mBtnExit->Bind(wxEVT_BUTTON, &DraggableBaseFrame::mBtnExitOnButtonClick, this);
}

void DraggableBaseFrame::mTitleBarOnLeftDown(wxMouseEvent& event)
{
	if (!mDragging) {
		mDragging = true;
		// 排除按钮区域
		wxPoint pos = event.GetPosition();
		wxRect menuRect = mBtnMenu->GetRect();
		wxRect exitRect = mBtnExit->GetRect();

		if (menuRect.Contains(pos) || exitRect.Contains(pos)) {
			event.Skip();
			return;
		}

		// 记录初始位置
		mDragStartMouse = wxGetMousePosition(); // 鼠标屏幕坐标
		mDragStartWindow = this->GetPosition(); // 窗口当前位置

		// 捕获鼠标并标记拖动状态
		CaptureMouse();
	}
}

void DraggableBaseFrame::mTitleBarOnLeftUp(wxMouseEvent& event) {
	if (mDragging) {
		mDragging = false;
	}
	if ( HasCapture() )
	{
		ReleaseMouse();
	}
}

void DraggableBaseFrame::mTitleBarOnMotion(wxMouseEvent& event) {
	if (mDragging) {
		wxPoint currentPos = wxGetMousePosition();
		wxPoint delta = currentPos - mDragStartMouse;
		this->Move(mDragStartWindow + delta); // 直接移动窗口
	}
}

void DraggableBaseFrame::mTitleBarOnMouseCaptureLost(wxMouseCaptureLostEvent &) {
	if (mDragging) {
		mDragging = false;
	}
	if ( HasCapture() )
	{
		ReleaseMouse();
	}
}

void DraggableBaseFrame::mBtnExitOnButtonClick(wxCommandEvent& event)
{
	Close(true);
}
void DraggableBaseFrame::SetSizer(wxSizer *sizer) {
	bSizer1->Add(sizer);
}