//
// Created by max_3 on 2025/6/6.
//

#ifndef UDCAPCOMMUNITYDRIVERUI_ENTERPRISEUI_H
#define UDCAPCOMMUNITYDRIVERUI_ENTERPRISEUI_H
#include <cstdint>
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include "ConsumerUI.h"

class EnterpriseUI: public ConsumerUI {
public:
    EnterpriseUI(std::shared_ptr<Gtk::Application> app);
};


#endif //UDCAPCOMMUNITYDRIVERUI_ENTERPRISEUI_H
