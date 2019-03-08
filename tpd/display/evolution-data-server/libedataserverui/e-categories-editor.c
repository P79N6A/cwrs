/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>

#include <libedataserver/libedataserver.h>
#include <libedataserver/libedataserver-private.h>

#include "e-categories-editor.h"
#include "e-categories-selector.h"
#include "e-category-completion.h"
#include "e-category-editor.h"

#define E_CATEGORIES_EDITOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_CATEGORIES_EDITOR, ECategoriesEditorPrivate))

struct _ECategoriesEditorPrivate {
	ECategoriesSelector *categories_list;
	GtkWidget *categories_entry;
	GtkWidget *categories_entry_label;
	GtkWidget *new_button;
	GtkWidget *edit_button;
	GtkWidget *delete_button;

	guint ignore_category_changes : 1;
};

enum {
	COLUMN_ACTIVE,
	COLUMN_ICON,
	COLUMN_CATEGORY,
	N_COLUMNS
};

enum {
	PROP_0,
	PROP_ENTRY_VISIBLE
};

enum {
	ENTRY_CHANGED,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (ECategoriesEditor, e_categories_editor, GTK_TYPE_TABLE)

static void
entry_changed_cb (GtkEntry *entry,
                  ECategoriesEditor *editor)
{
	g_signal_emit (editor, signals[ENTRY_CHANGED], 0);
}

static void
categories_editor_selection_changed_cb (ECategoriesEditor *editor,
                                        GtkTreeSelection *selection)
{
	GtkWidget *widget;
	gint n_rows;

	n_rows = gtk_tree_selection_count_selected_rows (selection);

	widget = editor->priv->edit_button;
	gtk_widget_set_sensitive (widget, n_rows == 1);

	widget = editor->priv->delete_button;
	gtk_widget_set_sensitive (widget, n_rows >= 1);
}

static void
category_checked_cb (ECategoriesSelector *selector,
                     const gchar *category,
                     const gboolean checked,
                     ECategoriesEditor *editor)
{
	GtkEntry *entry;
	gchar *categories;

	entry = GTK_ENTRY (editor->priv->categories_entry);
	categories = e_categories_selector_get_checked (selector);

	gtk_entry_set_text (entry, categories);

	g_free (categories);
}

static void
new_button_clicked_cb (GtkButton *button,
                       ECategoriesEditor *editor)
{
	ECategoryEditor *cat_editor = e_category_editor_new ();

	e_category_editor_create_category (cat_editor);

	gtk_widget_destroy (GTK_WIDGET (cat_editor));
}

static void
edit_button_clicked_cb (GtkButton *button,
                        ECategoriesEditor *editor)
{
	ECategoryEditor *cat_editor = e_category_editor_new ();
	gchar *category;

	category = e_categories_selector_get_selected (
		editor->priv->categories_list);

	e_category_editor_edit_category (cat_editor, category);

	gtk_widget_destroy (GTK_WIDGET (cat_editor));
	g_free (category);
}

static void
categories_editor_set_property (GObject *object,
                                guint property_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ENTRY_VISIBLE:
			e_categories_editor_set_entry_visible (
				E_CATEGORIES_EDITOR (object),
				g_value_get_boolean (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
categories_editor_get_property (GObject *object,
                                guint property_id,
                                GValue *value,
                                GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ENTRY_VISIBLE:
			g_value_set_boolean (
				value, e_categories_editor_get_entry_visible (
				E_CATEGORIES_EDITOR (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
e_categories_editor_class_init (ECategoriesEditorClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (ECategoriesEditorPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = categories_editor_set_property;
	object_class->get_property = categories_editor_get_property;

	g_object_class_install_property (
		object_class,
		PROP_ENTRY_VISIBLE,
		g_param_spec_boolean (
			"entry-visible",
			NULL,
			NULL,
			TRUE,
			G_PARAM_READWRITE));

	signals[ENTRY_CHANGED] = g_signal_new (
		"entry-changed",
		G_TYPE_FROM_CLASS (class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (ECategoriesEditorClass, entry_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
e_categories_editor_init (ECategoriesEditor *editor)
{
	GtkEntryCompletion *completion;
	GtkWidget *entry_categories;
	GtkWidget *label_header;
	GtkWidget *label2;
	GtkWidget *scrolledwindow1;
	GtkWidget *categories_list;
	GtkWidget *hbuttonbox1;
	GtkWidget *button_new;
	GtkWidget *button_edit;
	GtkWidget *button_delete;

	gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 400);

	gtk_table_resize (GTK_TABLE (editor), 3, 2);
	gtk_table_set_row_spacings (GTK_TABLE (editor), 6);
	gtk_table_set_col_spacings (GTK_TABLE (editor), 6);

	entry_categories = gtk_entry_new ();
	gtk_table_attach (
		GTK_TABLE (editor),
		entry_categories, 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);

	label_header = gtk_label_new_with_mnemonic (
		_("Currently _used categories:"));
	gtk_table_attach (
		GTK_TABLE (editor),
		label_header, 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label_header), GTK_JUSTIFY_CENTER);
	gtk_misc_set_alignment (GTK_MISC (label_header), 0, 0.5);

	label2 = gtk_label_new_with_mnemonic (_("_Available Categories:"));
	gtk_table_attach (
		GTK_TABLE (editor),
		label2, 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL),
		(GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_CENTER);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_table_attach (
		GTK_TABLE (editor),
		scrolledwindow1, 0, 1, 3, 4,
		(GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
		(GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (scrolledwindow1),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

	categories_list = GTK_WIDGET (e_categories_selector_new ());
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), categories_list);
	gtk_widget_set_size_request (categories_list, -1, 350);
	gtk_tree_view_set_headers_visible (
		GTK_TREE_VIEW (categories_list), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (categories_list), TRUE);
	g_signal_connect (
		G_OBJECT (categories_list), "category-checked",
		G_CALLBACK (category_checked_cb), editor);

	hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_table_attach (
		GTK_TABLE (editor),
		hbuttonbox1, 0, 1, 4, 5,
		(GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
		(GtkAttachOptions) (GTK_SHRINK), 0, 0);
	gtk_box_set_spacing (GTK_BOX (hbuttonbox1), 6);

	button_new = gtk_button_new_from_stock (GTK_STOCK_NEW);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_new);
	gtk_widget_set_can_default (button_new, TRUE);

	button_edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_edit);
	gtk_widget_set_can_default (button_edit, TRUE);

	button_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_delete);
	gtk_widget_set_can_default (button_delete, TRUE);

	gtk_label_set_mnemonic_widget (
		GTK_LABEL (label_header), entry_categories);
	gtk_label_set_mnemonic_widget (
		GTK_LABEL (label2), categories_list);

	editor->priv = E_CATEGORIES_EDITOR_GET_PRIVATE (editor);

	editor->priv->categories_list = E_CATEGORIES_SELECTOR (categories_list);
	editor->priv->categories_entry = entry_categories;
	editor->priv->categories_entry_label = label_header;

	g_signal_connect_swapped (
		editor->priv->categories_list, "selection-changed",
		G_CALLBACK (categories_editor_selection_changed_cb), editor);

	completion = e_category_completion_new ();
	gtk_entry_set_completion (
		GTK_ENTRY (editor->priv->categories_entry), completion);
	g_object_unref (completion);

	editor->priv->new_button = button_new;
	g_signal_connect (
		editor->priv->new_button, "clicked",
		G_CALLBACK (new_button_clicked_cb), editor);

	editor->priv->edit_button = button_edit;
	g_signal_connect (
		editor->priv->edit_button, "clicked",
		G_CALLBACK (edit_button_clicked_cb), editor);

	editor->priv->delete_button = button_delete;
	g_signal_connect_swapped (
		editor->priv->delete_button, "clicked",
		G_CALLBACK (e_categories_selector_delete_selection),
		editor->priv->categories_list);

	g_signal_connect (
		editor->priv->categories_entry, "changed",
		G_CALLBACK (entry_changed_cb), editor);

	gtk_widget_show_all (GTK_WIDGET (editor));
}

/**
 * e_categories_editor_new:
 *
 * Creates a new #ECategoriesEditor widget.
 *
 * Returns: a new #ECategoriesEditor
 *
 * Since: 3.2
 **/
GtkWidget *
e_categories_editor_new (void)
{
	return g_object_new (E_TYPE_CATEGORIES_EDITOR, NULL);
}

/**
 * e_categories_editor_get_categories:
 * @editor: an #ECategoriesEditor
 *
 * Gets a comma-separated list of the categories currently selected
 * in the editor.
 *
 * Returns: a comma-separated list of categories. Free returned
 * pointer with g_free().
 *
 * Since: 3.2
 **/
gchar *
e_categories_editor_get_categories (ECategoriesEditor *editor)
{
	ECategoriesSelector *categories_list;

	g_return_val_if_fail (E_IS_CATEGORIES_EDITOR (editor), NULL);

	categories_list = editor->priv->categories_list;

	return e_categories_selector_get_checked (categories_list);
}

/**
 * e_categories_editor_set_categories:
 * @editor: an #ECategoriesEditor
 * @categories: comma-separated list of categories
 *
 * Sets the list of categories selected on the editor.
 *
 * Since: 3.2
 **/
void
e_categories_editor_set_categories (ECategoriesEditor *editor,
                                    const gchar *categories)
{
	ECategoriesSelector *categories_list;

	g_return_if_fail (E_IS_CATEGORIES_EDITOR (editor));

	categories_list = editor->priv->categories_list;

	e_categories_selector_set_checked (categories_list, categories);
	category_checked_cb (categories_list, NULL, FALSE, editor);
}

/**
 * e_categories_editor_get_entry_visible:
 * @editor: an #ECategoriesEditor
 *
 * Return the visibility of the category input entry.
 *
 * Returns: whether the entry is visible
 *
 * Since: 3.2
 **/
gboolean
e_categories_editor_get_entry_visible (ECategoriesEditor *editor)
{
	g_return_val_if_fail (E_IS_CATEGORIES_EDITOR (editor), TRUE);

	return gtk_widget_get_visible (editor->priv->categories_entry);
}

/**
 * e_categories_editor_set_entry_visible:
 * @editor: an #ECategoriesEditor
 * @entry_visible: whether to make the entry visible
 *
 * Sets the visibility of the category input entry.
 *
 * Since: 3.2
 **/
void
e_categories_editor_set_entry_visible (ECategoriesEditor *editor,
                                       gboolean entry_visible)
{
	g_return_if_fail (E_IS_CATEGORIES_EDITOR (editor));

	if ((gtk_widget_get_visible (editor->priv->categories_entry) ? 1 : 0) ==
	    (entry_visible ? 1 : 0))
		return;

	gtk_widget_set_visible (
		editor->priv->categories_entry, entry_visible);
	gtk_widget_set_visible (
		editor->priv->categories_entry_label, entry_visible);
	e_categories_selector_set_items_checkable (
		editor->priv->categories_list, entry_visible);

	g_object_notify (G_OBJECT (editor), "entry-visible");
}
