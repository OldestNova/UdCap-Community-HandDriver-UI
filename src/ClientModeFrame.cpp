//
// Created by max_3 on 25-5-24.
//

#include "BaseFrame.h"
#include "UdCapCommunityUI.h"
//
// void ClientModeFrame::OnMouseCaptureLost(wxMouseCaptureLostEvent &) {
//     if (mDragging) {
//         Unbind(wxEVT_MOUSE_CAPTURE_LOST, &ClientModeFrame::OnMouseCaptureLost, this);
//         // Unbind(wxEVT_LEFT_UP, &ClientModeFrame::mTitleBarOnLeftUp, this);
//         // Unbind(wxEVT_MOTION, &ClientModeFrame::mTitleBarOnMotion, this);
//         mDragging = false;
//     }
//
//     if (HasCapture()) {
//         ReleaseMouse();
//     }
// }
//
// void ClientModeFrame::mTitleBarOnLeftDown(wxMouseEvent &event) {
//     if (!mDragging) {
//         Bind(wxEVT_MOUSE_CAPTURE_LOST, &DraggableBaseFrame::OnMouseCaptureLost, this);
//         // Bind(wxEVT_LEFT_UP, &ClientModeFrame::mTitleBarOnLeftUp, this);
//         // Bind(wxEVT_MOTION, &ClientModeFrame::mTitleBarOnMotion, this);
//         mDragging = true;
//
//         wxPoint clientStart = event.GetPosition();
//         mDragStartMouse = ClientToScreen(clientStart);
//         auto parent = GetParent();
//         if (parent != nullptr)
//         while (parent->GetParent() != nullptr) {
//             parent = parent->GetParent();
//         }
//         if (parent != nullptr) {
//             mDragStartWindow = parent->GetPosition();
//         } else {
//             mDragStartWindow = GetPosition();
//         }
//         CaptureMouse();
//     }
// }
//
// void ClientModeFrame::mTitleBarOnLeftUp(wxMouseEvent &) {
//     if (mDragging) {
//         Unbind(wxEVT_MOUSE_CAPTURE_LOST, &ClientModeFrame::OnMouseCaptureLost, this);
//
//         // Unbind(wxEVT_LEFT_UP, &ClientModeFrame::mTitleBarOnLeftUp, this);
//         // Unbind(wxEVT_MOTION, &ClientModeFrame::mTitleBarOnMotion, this);
//         mDragging = false;
//     }
//
//     if (HasCapture()) {
//         ReleaseMouse();
//     }
// }
//
// void ClientModeFrame::mTitleBarOnMotion(wxMouseEvent &event) {
//     if (mDragging) {
//         wxPoint curClientPsn = event.GetPosition();
//         wxPoint curScreenPsn = ClientToScreen(curClientPsn);
//         wxPoint movementVector = curScreenPsn - mDragStartMouse;
//         auto parent = GetParent();
//         if (parent != nullptr)
//         while (parent->GetParent() != nullptr) {
//             parent = parent->GetParent();
//         }
//         parent->SetPosition(mDragStartWindow + movementVector);
//     }
// }
//
// void ClientModeFrame::mBtnExitOnButtonClick(wxCommandEvent& event) {
//     Close(true);
// }
//
// void ClientModeFrame::mBtnMenuOnButtonClick(wxCommandEvent& event) {
//
// }
