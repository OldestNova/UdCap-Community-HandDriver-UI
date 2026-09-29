#include "HandAdjustmentEditor.h"
#include <glibmm/i18n.h>

namespace {
constexpr const char *names[]{
    N_("Thumb distal bend"),N_("Thumb intermediate bend"),N_("Thumb proximal bend"),N_("Thumb yaw"),
    N_("Index distal bend"),N_("Index intermediate bend"),N_("Index proximal bend"),N_("Index spread"),
    N_("Middle distal bend"),N_("Middle intermediate bend"),N_("Middle proximal bend"),N_("Middle spread"),
    N_("Ring distal bend"),N_("Ring intermediate bend"),N_("Ring proximal bend"),N_("Ring spread"),
    N_("Little distal bend"),N_("Little intermediate bend"),N_("Little proximal bend"),N_("Little spread"),N_("Thumb roll")};
constexpr const char *jointNames[]{
    N_("Thumb proximal"),N_("Thumb intermediate"),N_("Thumb distal"),
    N_("Index proximal"),N_("Index intermediate"),N_("Index distal"),
    N_("Middle proximal"),N_("Middle intermediate"),N_("Middle distal"),
    N_("Ring proximal"),N_("Ring intermediate"),N_("Ring distal"),
    N_("Little proximal"),N_("Little intermediate"),N_("Little distal")};
std::array<BoneRotation *,15> bones(HandRotation &r) {
    return {&r.thumbFinger.proximal,&r.thumbFinger.intermediate,&r.thumbFinger.distal,
        &r.indexFinger.proximal,&r.indexFinger.intermediate,&r.indexFinger.distal,
        &r.middleFinger.proximal,&r.middleFinger.intermediate,&r.middleFinger.distal,
        &r.ringFinger.proximal,&r.ringFinger.intermediate,&r.ringFinger.distal,
        &r.littleFinger.proximal,&r.littleFinger.intermediate,&r.littleFinger.distal};
}
}

Gtk::Widget &createHandAdjustmentEditor(const std::shared_ptr<UdCapV1Core> &core) {
    auto tabs = Gtk::make_managed<Gtk::Notebook>();
    auto curves = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL,8);
    curves->set_margin(8);
    auto channel = Gtk::make_managed<Gtk::ComboBoxText>();
    for (int i=0;i<21;++i) channel->append(std::to_string(i),_(names[i]));
    channel->set_active(2); curves->append(*channel);
    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_column_spacing(10); grid->set_row_spacing(6);
    std::array<Gtk::SpinButton *,4> values{};
    const char *labels[]{_("Output minimum (degrees)"),_("Output maximum (degrees)"),
                         _("Response balance"),_("Bone offset (degrees)")};
    for (int i=0;i<4;++i) {
        auto label=Gtk::make_managed<Gtk::Label>(labels[i]); label->set_halign(Gtk::Align::START);
        auto spin=Gtk::make_managed<Gtk::SpinButton>();
        spin->set_range(i==2?0:i==3?-45:-180,i==2?1:i==3?45:180);
        spin->set_increments(i==2?.01:1,i==2?.1:5); spin->set_digits(i==2?2:1); spin->set_width_chars(7);
        grid->attach(*label,0,i); grid->attach(*spin,1,i); values[i]=spin;
    }
    curves->append(*grid);
    auto plot=Gtk::make_managed<Gtk::DrawingArea>();
    plot->set_content_height(120); plot->set_hexpand(true);
    plot->set_draw_func([core,channel](const Cairo::RefPtr<Cairo::Context> &cr,int w,int h) {
        const int i=channel->get_active_row_number(); if(i<0) return;
        const auto c=core->getHandResponse()[i];
        cr->set_source_rgb(.55,.55,.55); cr->set_line_width(1);
        cr->move_to(12,8); cr->line_to(12,h-12); cr->line_to(w-10,h-12); cr->stroke();
        if(i==11) return;
        const double lo=std::min(handResponseMin[i],c.outputMin+c.offset);
        const double hi=std::max(handResponseMax[i],c.outputMax+c.offset);
        const double range=std::max(1.0,hi-lo);
        cr->set_source_rgb(.15,.45,.85); cr->set_line_width(2);
        for(int j=0;j<=100;++j) {
            const double x=handResponseMin[i]+(handResponseMax[i]-handResponseMin[i])*j/100;
            const double y=evaluateHandResponse(x,handResponseMin[i],handResponseMax[i],c);
            const double px=12+(w-24)*j/100.0,py=h-12-(h-24)*(y-lo)/range;
            if(j==0) cr->move_to(px,py); else cr->line_to(px,py);
        }
        cr->stroke();
    });
    curves->append(*plot);
    auto status=Gtk::make_managed<Gtk::Label>(); status->set_wrap(true); curves->append(*status);
    auto updating=std::make_shared<bool>(false);
    const auto refresh=[=]() {
        *updating=true; const int i=channel->get_active_row_number(); const auto c=core->getHandResponse()[i];
        values[0]->set_value(c.outputMin); values[1]->set_value(c.outputMax);
        values[2]->set_value(c.balance); values[3]->set_value(c.offset);
        for(int j=0;j<3;++j) values[j]->set_sensitive(i!=11);
        status->set_text(i==11?_("Middle spread uses a constant offset."):
            _("0.50 is linear. Settings apply to every output; values beyond the range are extrapolated."));
        *updating=false; plot->queue_draw();
    };
    channel->signal_changed().connect(refresh);
    for(auto spin:values) spin->signal_value_changed().connect([=]() {
        if(*updating) return;
        auto settings=core->getHandResponse();
        settings[channel->get_active_row_number()]={values[0]->get_value(),values[1]->get_value(),values[2]->get_value(),values[3]->get_value()};
        try { core->setHandResponse(settings); status->set_text(_("Applied")); plot->queue_draw(); }
        catch(const std::exception &) { status->set_text(_("Output minimum must not exceed maximum.")); }
    });
    auto reset=Gtk::make_managed<Gtk::Button>(_("Reset this curve"));
    reset->signal_clicked().connect([=]() {
        auto settings=core->getHandResponse(); const int i=channel->get_active_row_number();
        settings[i]=defaultHandResponse()[i]; core->setHandResponse(settings); refresh();
    });
    curves->append(*reset); refresh(); tabs->append_page(*curves,_("Response curves"));

    auto local=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL,8); local->set_margin(8);
    auto joint=Gtk::make_managed<Gtk::ComboBoxText>();
    for(int i=0;i<15;++i) joint->append(std::to_string(i),_(jointNames[i]));
    joint->set_active(0); local->append(*joint);
    auto axes=Gtk::make_managed<Gtk::Grid>(); axes->set_row_spacing(6); axes->set_column_spacing(10);
    std::array<Gtk::SpinButton *,3> xyz{};
    auto updatingOffset=std::make_shared<bool>(false);
    for(int i=0;i<3;++i) {
        auto label=Gtk::make_managed<Gtk::Label>(std::string(1,"XYZ"[i]));
        auto spin=Gtk::make_managed<Gtk::SpinButton>();
        spin->set_range(-45,45); spin->set_increments(1,5); spin->set_digits(1); spin->set_width_chars(7);
        axes->attach(*label,0,i); axes->attach(*spin,1,i); xyz[i]=spin;
    }
    const auto refreshOffset=[=]() {
        *updatingOffset=true; auto r=core->getHandOffset(); const auto b=*bones(r)[joint->get_active_row_number()];
        xyz[0]->set_value(b.x); xyz[1]->set_value(b.y); xyz[2]->set_value(b.z); *updatingOffset=false;
    };
    joint->signal_changed().connect(refreshOffset);
    for(auto spin:xyz) spin->signal_value_changed().connect([=]() {
        if(*updatingOffset) return;
        auto r=core->getHandOffset();
        *bones(r)[joint->get_active_row_number()]={float(xyz[0]->get_value()),float(xyz[1]->get_value()),float(xyz[2]->get_value())};
        core->setHandOffset(r);
    });
    refreshOffset(); local->append(*axes);
    auto note=Gtk::make_managed<Gtk::Label>(_("Local Unity bone axes. VRChat OSC supports only bend and spread; other twist axes require a skeleton output."));
    note->set_wrap(true); local->append(*note);
    auto resetAll=Gtk::make_managed<Gtk::Button>(_("Reset all hand adjustments"));
    resetAll->signal_clicked().connect([=]() {core->setHandOffset(HandRotation{});core->setHandResponse(defaultHandResponse());refresh();refreshOffset();});
    local->append(*resetAll); tabs->append_page(*local,_("Local bone offsets"));
    return *tabs;
}
