#pragma once
#include <gtkmm.h>
#include <UdCapV1Core.h>

Gtk::Widget &createHandAdjustmentEditor(const std::shared_ptr<UdCapV1Core> &core);
