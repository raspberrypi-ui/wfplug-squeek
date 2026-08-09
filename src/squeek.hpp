/*============================================================================
Copyright (c) 2024 Raspberry Pi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================*/

#ifndef WIDGETS_SQUEEK_HPP
#define WIDGETS_SQUEEK_HPP

#include <widget.hpp>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

extern "C" {
#include "lxutils.h"
}

#define PLUGIN_TITLE N_("Squeekboard")

class WidgetSqueek : public PanelWidget
{
    std::unique_ptr <Gtk::Button> plugin;
    Gtk::Image *icon;   // non-owning - becomes a child of plugin in widget_init(),
                        // which then owns its lifetime; must not be independently
                        // destroyed by a member unique_ptr while still parented
                        // (member destruction order runs icon before plugin,
                        // which was corrupting the heap - see widget_init()).

    GtkGesture *gesture;

    GDBusProxy *proxy;
    guint owner_id;

  public:

    void widget_init (Gtk::HBox *container) override;
    virtual ~WidgetSqueek ();
    void widget_set_icon (void);
    void on_button_press_event (void);

    void sb_cb_name_owned (const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&, const Glib::ustring&);
    void sb_cb_name_unowned (const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&);
};

#endif /* end of include guard: WIDGETS_SQUEEK_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
