//      other.c
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.


#include "lxappearance.h"
#include "other.h"
#include <glib/gi18n.h>

static void on_tb_style_changed(GtkComboBox* combo, gpointer user_data)
{
    app.toolbar_style = gtk_combo_box_get_active(combo) + GTK_TOOLBAR_ICONS;
    lxappearance_changed();
}

static void on_tb_icon_size_changed(GtkComboBox* combo, gpointer user_data)
{
    app.toolbar_icon_size = gtk_combo_box_get_active(combo) + GTK_ICON_SIZE_MENU;
    lxappearance_changed();
}

void other_init(GtkBuilder* b)
{
    int idx;
    app.tb_style_combo = GTK_WIDGET(gtk_builder_get_object(b, "tb_style"));
    idx = app.toolbar_style - GTK_TOOLBAR_ICONS;
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.tb_style_combo), idx);
    g_signal_connect(app.tb_style_combo, "changed", G_CALLBACK(on_tb_style_changed), NULL);

    app.tb_icon_size_combo = GTK_WIDGET(gtk_builder_get_object(b, "tb_icon_size"));
    idx = app.toolbar_icon_size - GTK_ICON_SIZE_MENU;
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.tb_icon_size_combo), idx);
    g_signal_connect(app.tb_icon_size_combo, "changed", G_CALLBACK(on_tb_icon_size_changed), NULL);

    app.button_images_check = GTK_WIDGET(gtk_builder_get_object(b, "button_images"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.button_images_check), app.button_images);
    g_signal_connect(app.button_images_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.button_images);

    app.menu_images_check = GTK_WIDGET(gtk_builder_get_object(b, "menu_images"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.menu_images_check), app.menu_images);
    g_signal_connect(app.menu_images_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.menu_images);

#if GTK_CHECK_VERSION(2, 14, 0)
    app.event_sound_check = GTK_WIDGET(gtk_builder_get_object(b, "event_sound"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.event_sound_check), app.enable_event_sound);
    g_signal_connect(app.event_sound_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.enable_event_sound);

    app.input_feedback_check = GTK_WIDGET(gtk_builder_get_object(b, "input_feedback_sound"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.input_feedback_check), app.enable_input_feedback);
    g_signal_connect(app.input_feedback_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.enable_input_feedback);

    /* event sound support */
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(b, "sound_effect")));
#endif

}

