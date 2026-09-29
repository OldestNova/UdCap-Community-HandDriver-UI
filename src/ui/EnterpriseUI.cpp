//
// Created by max_3 on 2025/6/6.
//

#include "EnterpriseUI.h"

EnterpriseUI::EnterpriseUI(std::shared_ptr<Gtk::Application> app): ConsumerUI(app, true) {
    set_title(_("UdCap Community Driver - Enterprise Edition"));
    set_default_size(1280, 900);
}
