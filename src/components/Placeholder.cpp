//
// Created by max_3 on 25-6-20.
//

#include "Placeholder.h"
Glib::RefPtr<Gdk::Pixbuf> create_placeholder_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0xAAAAAAFF); // 灰色占位
    return pb;
}
Glib::RefPtr<Gdk::Pixbuf> create_placeholder_green_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0x00FF00FF); // 绿色占位
    return pb;
}
Glib::RefPtr<Gdk::Pixbuf> create_placeholder_blue_image() {
    auto pb = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, 48, 48);
    pb->fill(0x0000FFFF); // 绿色占位
    return pb;
}