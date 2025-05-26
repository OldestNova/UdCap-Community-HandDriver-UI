///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/panel.h>
#include <HandGL3D.hpp>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class ClientModeFrame
///////////////////////////////////////////////////////////////////////////////
class ClientModeFrame : public wxFrame
{
	private:

	protected:
		wxPanel* mMainPanel;
		wxStaticText* mHandState;
		wxStaticText* mHandDescState;
		wxButton* mCalibration;
		wxStaticBitmap* mIconLeftHand;
		wxStaticBitmap* mIconRightHand;
		wxStaticBitmap* mIconOSC;
		wxStaticBitmap* mIconVMC;
		wxStaticBitmap* mIconVR;
		wxPanel* m3DPanel;
		HandGL3D* mGL3D;

	public:

		ClientModeFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("UdCap Community Driver"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 320,373 ), long style = wxCAPTION|wxCLOSE_BOX|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxTAB_TRAVERSAL );

		~ClientModeFrame();

};

