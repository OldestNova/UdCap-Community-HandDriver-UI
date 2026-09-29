#ifndef UDCAPCOMMUNITYDRIVERUI_STATUSINDICATOR_H
#define UDCAPCOMMUNITYDRIVERUI_STATUSINDICATOR_H

#include <gtkmm/drawingarea.h>
#include <utility>

// A small, theme-independent status icon. The shape identifies the function;
// the tint and corner dot show whether it is off, waiting, or active.
class StatusIndicator : public Gtk::DrawingArea {
public:
    enum class Kind { LeftGlove, RightGlove, SteamVR, OSC, VMC, QingTong };
    enum class State { Off, Waiting, Active };

    explicit StatusIndicator(Kind kind): kind_(kind) {
        set_content_width(36);
        set_content_height(36);
        set_halign(Gtk::Align::CENTER);
        set_valign(Gtk::Align::CENTER);
        set_draw_func([this](const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
            draw(cr, width, height);
        });
    }

    void setState(State state) {
        if (state_ == state) return;
        state_ = state;
        queue_draw();
    }

private:
    static void roundedRect(const Cairo::RefPtr<Cairo::Context> &cr,
                            double x, double y, double w, double h, double r) {
        cr->begin_new_sub_path();
        cr->arc(x + w - r, y + r, r, -1.57079632679, 0);
        cr->arc(x + w - r, y + h - r, r, 0, 1.57079632679);
        cr->arc(x + r, y + h - r, r, 1.57079632679, 3.14159265359);
        cr->arc(x + r, y + r, r, 3.14159265359, 4.71238898038);
        cr->close_path();
    }

    void draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) const {
        const double size = static_cast<double>(width < height ? width : height);
        cr->save();
        cr->translate((width - size) / 2.0, (height - size) / 2.0);
        cr->scale(size / 36.0, size / 36.0);

        double red = 0.49, green = 0.52, blue = 0.56;
        if (state_ == State::Waiting) {
            red = 0.12; green = 0.68; blue = 0.29;
        } else if (state_ == State::Active) {
            red = 0.16; green = 0.43; blue = 0.88;
        }
        roundedRect(cr, 1.5, 1.5, 33, 33, 7);
        cr->set_source_rgba(red, green, blue, 0.13);
        cr->fill_preserve();
        cr->set_source_rgba(red, green, blue, 0.70);
        cr->set_line_width(1.4);
        cr->stroke();

        cr->set_source_rgb(red, green, blue);
        cr->set_line_width(2.1);
        cr->set_line_cap(Cairo::Context::LineCap::ROUND);
        switch (kind_) {
            case Kind::LeftGlove:
            case Kind::RightGlove:
                if (kind_ == Kind::RightGlove) {
                    cr->translate(36, 0);
                    cr->scale(-1, 1);
                }
                // Palm, four fingers, and the outward thumb.
                roundedRect(cr, 13, 18, 14, 12, 3);
                cr->stroke();
                for (int x = 14; x <= 26; x += 4) {
                    cr->move_to(x, 18);
                    cr->line_to(x, x == 18 ? 7 : 9);
                }
                cr->stroke();
                cr->move_to(13, 24);
                cr->line_to(8, 20);
                cr->line_to(6, 21);
                cr->line_to(12, 28);
                cr->stroke();
                break;
            case Kind::SteamVR:
                roundedRect(cr, 6, 13, 24, 13, 5);
                cr->stroke();
                cr->arc(13, 20, 2.4, 0, 6.28318530718);
                cr->arc(23, 20, 2.4, 0, 6.28318530718);
                cr->stroke();
                cr->move_to(6, 18);
                cr->line_to(3.5, 17);
                cr->move_to(30, 18);
                cr->line_to(32.5, 17);
                cr->stroke();
                break;
            case Kind::OSC:
                // Connected endpoints represent OSC's network transport.
                cr->move_to(11, 11);
                cr->line_to(25, 18);
                cr->line_to(11, 25);
                cr->stroke();
                for (const auto [x, y] : {std::pair{11, 11}, {25, 18}, {11, 25}}) {
                    cr->arc(x, y, 3, 0, 6.28318530718);
                    cr->fill();
                }
                break;
            case Kind::VMC:
                // A tracked skeleton distinguishes motion capture from OSC.
                cr->arc(18, 9, 2.5, 0, 6.28318530718);
                cr->stroke();
                cr->move_to(18, 12);
                cr->line_to(18, 22);
                cr->move_to(10, 15);
                cr->line_to(18, 17);
                cr->line_to(26, 15);
                cr->move_to(18, 22);
                cr->line_to(13, 29);
                cr->move_to(18, 22);
                cr->line_to(23, 29);
                cr->stroke();
                for (const auto [x, y] : {std::pair{10, 15}, {26, 15}, {13, 29}, {23, 29}}) {
                    cr->arc(x, y, 1.7, 0, 6.28318530718);
                    cr->fill();
                }
                break;
            case Kind::QingTong:
                // A sender with radiating waves represents QingTong UDP output.
                cr->arc(10, 18, 2.6, 0, 6.28318530718);
                cr->fill();
                cr->arc(10, 18, 8, -0.85, 0.85);
                cr->stroke();
                cr->arc(10, 18, 15, -0.70, 0.70);
                cr->stroke();
                break;
        }

        // The corner dot keeps the status color obvious even at small sizes.
        cr->set_source_rgb(red, green, blue);
        cr->arc(30.5, 30.5, 3.1, 0, 6.28318530718);
        cr->fill();
        cr->restore();
    }

    Kind kind_;
    State state_ = State::Off;
};

#endif
