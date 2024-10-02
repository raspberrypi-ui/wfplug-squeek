#ifndef WIDGETS_SQUEEK_HPP
#define WIDGETS_SQUEEK_HPP

#include "widget.hpp"
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/gesturelongpress.h>

class WayfireSqueek : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;
    std::unique_ptr <Gtk::Image> icon;
    Glib::RefPtr<Gtk::GestureLongPress> gesture;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireSqueek ();
    void icon_size_changed_cb (void);
    bool set_icon (void);
    void on_button_press_event (void);
    void on_gesture_pressed (double x, double y);
    void on_gesture_end (GdkEventSequence *eq);
};

#endif /* end of include guard: WIDGETS_SQUEEK_HPP */
