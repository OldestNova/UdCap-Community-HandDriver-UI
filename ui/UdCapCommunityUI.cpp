///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "UdCapCommunityUI.h"

///////////////////////////////////////////////////////////////////////////

ClientModeFrame::ClientModeFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	bSizer1->SetMinSize( wxSize( 300,300 ) );
	mMainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	mHandState = new wxStaticText( mMainPanel, wxID_ANY, _("CAP_STATUS"), wxDefaultPosition, wxDefaultSize, 0 );
	mHandState->Wrap( -1 );
	mHandState->SetFont( wxFont( 30, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer9->Add( mHandState, 0, wxALL, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	mHandDescState = new wxStaticText( mMainPanel, wxID_ANY, _("CAP_DESC_STATUS"), wxDefaultPosition, wxDefaultSize, 0 );
	mHandDescState->Wrap( -1 );
	bSizer11->Add( mHandDescState, 0, wxALL, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	mCalibration = new wxButton( mMainPanel, wxID_ANY, _("BTN_CALI"), wxDefaultPosition, wxDefaultSize, 0 );
	mCalibration->SetLabelMarkup( _("BTN_CALI") );
	mCalibration->Enable( false );

	bSizer12->Add( mCalibration, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizer11->Add( bSizer12, 1, wxEXPAND, 5 );


	bSizer9->Add( bSizer11, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	mIconLeftHand = new wxStaticBitmap( mMainPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( mIconLeftHand, 0, wxALIGN_CENTER|wxALL, 5 );

	mIconRightHand = new wxStaticBitmap( mMainPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( mIconRightHand, 0, wxALIGN_CENTER|wxALL, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );

	mIconOSC = new wxStaticBitmap( mMainPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( mIconOSC, 0, wxALL, 5 );

	mIconVMC = new wxStaticBitmap( mMainPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( mIconVMC, 0, wxALL, 5 );

	mIconVR = new wxStaticBitmap( mMainPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( mIconVR, 0, wxALL, 5 );


	bSizer13->Add( bSizer17, 1, wxALIGN_RIGHT|wxSHAPED, 5 );


	bSizer8->Add( bSizer13, 1, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer9->Add( bSizer8, 1, wxEXPAND, 5 );


	mMainPanel->SetSizer( bSizer9 );
	mMainPanel->Layout();
	bSizer9->Fit( mMainPanel );
	bSizer1->Add( mMainPanel, 1, wxEXPAND | wxALL, 5 );

	m3DPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 1, 1, 0, 0 );

	mGL3D = new HandGL3D( m3DPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( mGL3D, 0, wxALL|wxEXPAND, 5 );


	m3DPanel->SetSizer( gSizer1 );
	m3DPanel->Layout();
	gSizer1->Fit( m3DPanel );
	bSizer1->Add( m3DPanel, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	this->Centre( wxBOTH );
}

ClientModeFrame::~ClientModeFrame()
{
}
