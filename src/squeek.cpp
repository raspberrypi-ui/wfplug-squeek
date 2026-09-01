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
#include "gtk-utils.hpp"
#include "squeek.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireSqueek; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_TYPE_NONE, NULL, NULL, NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; }
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

GDBusProxy *proxy;

static struct udev         *udev_ctx;
static struct udev_monitor *udev_mon;
static int                  kbd_count;
static GtkWidget           *panel_button;

bool WayfireSqueek::set_icon (void)
{
    set_taskbar_icon (GTK_WIDGET (icon->gobj ()), "squeekboard");
    return false;
}

void WayfireSqueek::on_button_press_event (void)
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

/* Set squeekboard visibility based on physical keyboard presence */

static void update_keyboard_visibility (void)
{
    GError *err = NULL;
    GVariant *val;

    if (!proxy) return;

    val = g_variant_new ("(b)", kbd_count == 0);
    g_dbus_proxy_call_sync (proxy, "SetVisible", val, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    g_variant_unref (val);
    if (err) printf ("%s\n", err->message);

    if (panel_button)
    {
        if (kbd_count == 0) gtk_widget_show (panel_button);
        else gtk_widget_hide (panel_button);
    }
}

/* Return TRUE if device is a real keyboard (not a mouse, touchscreen, etc.) */

static gboolean is_keyboard (struct udev_device *dev)
{
    const char *val;

    val = udev_device_get_property_value (dev, "ID_INPUT_KEYBOARD");
    if (!val || strcmp (val, "1") != 0) return FALSE;

    /* Exclude devices that are primarily something other than a keyboard */
    const char *excl[] = { "ID_INPUT_MOUSE", "ID_INPUT_TOUCHSCREEN",
                           "ID_INPUT_TABLET", "ID_INPUT_JOYSTICK", NULL };
    for (int i = 0; excl[i]; i++)
    {
        val = udev_device_get_property_value (dev, excl[i]);
        if (val && strcmp (val, "1") == 0) return FALSE;
    }

    return TRUE;
}

/* Count physical keyboards currently present */

static int count_keyboards (void)
{
    int count = 0;
    struct udev_enumerate *en = udev_enumerate_new (udev_ctx);
    udev_enumerate_add_match_property (en, "ID_INPUT_KEYBOARD", "1");
    udev_enumerate_scan_devices (en);
    struct udev_list_entry *entry, *devices = udev_enumerate_get_list_entry (en);
    udev_list_entry_foreach (entry, devices)
    {
        struct udev_device *dev = udev_device_new_from_syspath (udev_ctx,
            udev_list_entry_get_name (entry));
        if (dev)
        {
            if (udev_device_get_devnode (dev) && is_keyboard (dev)) count++;
            udev_device_unref (dev);
        }
    }
    udev_enumerate_unref (en);
    return count;
}

/* udev monitor callback - called by GLib main loop when udev socket is readable */

static gboolean udev_cb (GIOChannel *, GIOCondition, gpointer)
{
    struct udev_device *dev = udev_monitor_receive_device (udev_mon);
    if (!dev) return TRUE;

    const char *action = udev_device_get_action (dev);

    if (action && strcmp (action, "add") == 0 && is_keyboard (dev))
    {
        kbd_count++;
        update_keyboard_visibility ();
    }
    else if (action && strcmp (action, "remove") == 0)
    {
        /* Re-enumerate on removal as properties may be absent in remove events */
        kbd_count = count_keyboards ();
        update_keyboard_visibility ();
    }

    udev_device_unref (dev);
    return TRUE;
}

/* Callback for Squeekboard D-Bus property changes - suppress auto-show when physical keyboard present */

static void proxy_properties_changed (GDBusProxy *, GVariant *changed, const gchar *const *, gpointer)
{
    GVariant *vis = g_variant_lookup_value (changed, "Visible", G_VARIANT_TYPE_BOOLEAN);
    if (!vis) return;
    gboolean val;
    g_variant_get (vis, "b", &val);
    g_variant_unref (vis);
    if (val && kbd_count > 0) update_keyboard_visibility ();
}

/* Callback for Squeekboard appearing on D-Bus */

static void sb_cb_name_owned (GDBusConnection *conn, const gchar *name, const gchar *, gpointer user_data)
{
    GError *err = NULL;
    proxy = g_dbus_proxy_new_sync (conn, G_DBUS_PROXY_FLAGS_NONE, NULL, name, "/sm/puri/OSK0", "sm.puri.OSK0", NULL, &err);
    if (err) printf ("%s\n", err->message);
    g_signal_connect (proxy, "g-properties-changed", G_CALLBACK (proxy_properties_changed), NULL);
    panel_button = GTK_WIDGET (user_data);
    gtk_widget_show_all (GTK_WIDGET (user_data));
    update_keyboard_visibility ();
}

/* Callback for Squeekboard disappearing on D-Bus */

static void sb_cb_name_unowned (GDBusConnection *, const gchar *, gpointer user_data)
{
    gtk_widget_hide (GTK_WIDGET (user_data));
}

void WayfireSqueek::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Create the icon */
    icon = std::make_unique <Gtk::Image> ();
    plugin->add (*icon.get());
    plugin->signal_clicked().connect (sigc::mem_fun (*this, &WayfireSqueek::on_button_press_event));
    plugin->set_tooltip_text (_("Click to show or hide the virtual keyboard"));

    /* Setup structure */
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireSqueek::set_icon));

    /* Add long press for right click */
    gesture = add_longpress_default (*plugin);

    /* Set up callbacks to see if squeekboard is on D-Bus */
    g_bus_watch_name (G_BUS_TYPE_SESSION, "sm.puri.OSK0", G_BUS_NAME_WATCHER_FLAGS_NONE, sb_cb_name_owned, sb_cb_name_unowned, (*plugin).gobj(), NULL);

    /* Set up udev monitor to detect physical keyboard plug/unplug */
    udev_ctx = udev_new ();
    if (udev_ctx)
    {
        udev_mon = udev_monitor_new_from_netlink (udev_ctx, "udev");
        udev_monitor_filter_add_match_subsystem_devtype (udev_mon, "input", NULL);
        udev_monitor_enable_receiving (udev_mon);

        GIOChannel *chan = g_io_channel_unix_new (udev_monitor_get_fd (udev_mon));
        udev_watch = g_io_add_watch (chan, G_IO_IN, udev_cb, NULL);
        g_io_channel_unref (chan);

        /* Count keyboards already present at startup */
        kbd_count = count_keyboards ();
    }
}

WayfireSqueek::~WayfireSqueek()
{
    icon_timer.disconnect ();
    if (udev_watch) g_source_remove (udev_watch);
    if (udev_mon) udev_monitor_unref (udev_mon);
    if (udev_ctx) udev_unref (udev_ctx);
}

/* End of file */
/*----------------------------------------------------------------------------*/
