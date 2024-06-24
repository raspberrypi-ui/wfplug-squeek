#ifndef WIDGETS_SQUEEK_HPP
#define WIDGETS_SQUEEK_HPP

#include "widget.hpp"
#include <gtkmm/button.h>
#include <gtkmm/image.h>

class WayfireSqueek : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;
    std::unique_ptr <Gtk::Image> icon;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL,       NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireSqueek ();
    void icon_size_changed_cb (void);
    bool set_icon (void);
    void on_button_press_event (void);
    void do_menu (void);
    static const char *display_name (void) { return N_("Squeekboard"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_SQUEEK_HPP */