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

#include <glibmm.h>
#include <giomm/dbuswatchname.h>
#include <giomm/dbusconnection.h>
#include "squeek.hpp"

extern "C" {
    PanelWidget *create () { return new WidgetSqueek; }
    void destroy (PanelWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_TYPE_NONE, NULL, NULL, NULL, NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; }
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WidgetSqueek::on_button_press_event (void)
{
    GError *err = NULL;
    GVariant *val;
    gboolean res;

    CHECK_LONGPRESS
    val = g_dbus_proxy_get_cached_property (proxy, "Visible");
    g_variant_get (val, "b", &res);
    g_variant_unref (val);

    val = g_variant_new ("(b)", !res);
    g_dbus_proxy_call_sync (proxy, "SetVisible", val, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    g_variant_unref (val);
    if (err) printf ("%s\n", err->message);
}

/* Callback for Squeekboard appearing on D-Bus */

void WidgetSqueek::sb_cb_name_owned (const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& name, const Glib::ustring&)
{
    GError *err = NULL;
    proxy = g_dbus_proxy_new_sync (connection->gobj (), G_DBUS_PROXY_FLAGS_NONE, NULL, name.c_str (), "/sm/puri/OSK0", "sm.puri.OSK0", NULL, &err);
    if (err) printf ("%s\n", err->message);
    plugin->show_all ();
}

/* Callback for Squeekboard disappearing on D-Bus */

void WidgetSqueek::sb_cb_name_unowned (const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&)
{
    plugin->hide ();
}

void WidgetSqueek::widget_set_icon (void)
{
    set_taskbar_icon (GTK_WIDGET (icon->gobj ()), "squeekboard");
}

void WidgetSqueek::widget_init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Create the icon */
    icon = std::make_unique <Gtk::Image> ();
    plugin->add (*icon.get());
    plugin->signal_clicked().connect (sigc::mem_fun (*this, &WidgetSqueek::on_button_press_event));
    plugin->set_tooltip_text (_("Click to show or hide the virtual keyboard"));

    /* Add long press for right click */
    gesture = add_long_press (GTK_WIDGET (plugin->gobj ()), NULL, NULL);

    /* Set up callbacks to see if squeekboard is on D-Bus */
    owner_id = Gio::DBus::watch_name (Gio::DBus::BusType::BUS_TYPE_SESSION, "sm.puri.OSK0",
        sigc::mem_fun (this, &WidgetSqueek::sb_cb_name_owned), sigc::mem_fun (this, &WidgetSqueek::sb_cb_name_unowned));
}

WidgetSqueek::~WidgetSqueek()
{
    if (owner_id) Gio::DBus::unwatch_name (owner_id);
    g_object_unref (gesture);
}

/* End of file */
/*----------------------------------------------------------------------------*/
