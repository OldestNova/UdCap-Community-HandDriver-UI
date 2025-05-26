#include <iostream>
#include "main.h"

#include "UdCapCommunityUI.h"

// App
ClientModeFrame *frame;
bool UdCapCommunityDriverUI::OnInit() {
    wxImage::AddHandler(new wxPNGHandler);
    frame = new ClientModeFrame(nullptr);
    frame->Show(true);
    return true;
}