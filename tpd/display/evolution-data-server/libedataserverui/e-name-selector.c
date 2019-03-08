/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* e-name-selector.c - Unified context for contact/destination selection UI.
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: Hans Petter Jansson <hpj@novell.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <libebook/libebook.h>

#include <libedataserverui/e-client-utils.h>
#include <libedataserverui/e-contact-store.h>
#include <libedataserverui/e-destination-store.h>
#include <libedataserverui/e-book-auth-util.h>
#include "e-name-selector.h"

#define E_NAME_SELECTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_NAME_SELECTOR, ENameSelectorPrivate))

typedef struct {
	gchar *name;
	ENameSelectorEntry *entry;
} Section;

typedef struct {
	EBookClient *client;
	guint is_completion_book : 1;
} SourceBook;

struct _ENameSelectorPrivate {
	ESourceRegistry *registry;
	ENameSelectorModel *model;
	ENameSelectorDialog *dialog;

	GArray *sections;

	gboolean books_loaded;
	GCancellable *cancellable;
	GArray *source_books;
};

enum {
	PROP_0,
	PROP_REGISTRY
};

G_DEFINE_TYPE (ENameSelector, e_name_selector, G_TYPE_OBJECT)

static gboolean
is_source_enabled_with_parents (ESourceRegistry *registry,
                                ESource *source)
{
	ESource *parent;
	const gchar *parent_uid;

	g_return_val_if_fail (registry != NULL, FALSE);
	g_return_val_if_fail (source != NULL, FALSE);

	if (!e_source_get_enabled (source))
		return FALSE;

	parent = g_object_ref (source);
	while (parent_uid = e_source_get_parent (parent), parent_uid) {
		ESource *next = e_source_registry_ref_source (registry, parent_uid);

		if (!next)
			break;

		g_object_unref (parent);

		if (!e_source_get_enabled (next)) {
			g_object_unref (next);
			return FALSE;
		}

		parent = next;
	}

	g_object_unref (parent);

	return TRUE;
}

static void
reset_pointer_cb (gpointer data,
                  GObject *where_was)
{
	ENameSelector *name_selector = data;
	ENameSelectorPrivate *priv;
	guint ii;

	g_return_if_fail (E_IS_NAME_SELECTOR (name_selector));

	priv = E_NAME_SELECTOR_GET_PRIVATE (name_selector);

	for (ii = 0; ii < priv->sections->len; ii++) {
		Section *section;

		section = &g_array_index (priv->sections, Section, ii);
		if (section->entry == (ENameSelectorEntry *) where_was)
			section->entry = NULL;
	}
}

static void
name_selector_book_loaded_cb (GObject *source_object,
                              GAsyncResult *result,
                              gpointer user_data)
{
	ENameSelector *name_selector = user_data;
	ESource *source = E_SOURCE (source_object);
	EBookClient *book_client;
	EClient *client = NULL;
	GArray *sections;
	SourceBook source_book;
	guint ii;
	GError *error = NULL;

	e_client_utils_open_new_finish (source, result, &client, &error);

	if (error != NULL) {
		if (!g_error_matches (error, E_CLIENT_ERROR, E_CLIENT_ERROR_REPOSITORY_OFFLINE)
		    && !g_error_matches (error, E_CLIENT_ERROR, E_CLIENT_ERROR_OFFLINE_UNAVAILABLE)
		    && !g_error_matches (error, E_CLIENT_ERROR, E_CLIENT_ERROR_CANCELLED)
		    && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning (
				"ENameSelector: Could not load \"%s\": %s",
				e_source_get_display_name (source), error->message);
		g_error_free (error);
		goto exit;
	}

	book_client = E_BOOK_CLIENT (client);
	g_return_if_fail (E_IS_BOOK_CLIENT (book_client));

	source_book.client = book_client;
	source_book.is_completion_book = TRUE;

	g_array_append_val (name_selector->priv->source_books, source_book);

	sections = name_selector->priv->sections;

	for (ii = 0; ii < sections->len; ii++) {
		EContactStore *store;
		Section *section;

		section = &g_array_index (sections, Section, ii);
		if (section->entry == NULL)
			continue;

		store = e_name_selector_entry_peek_contact_store (
			section->entry);
		if (store != NULL)
			e_contact_store_add_client (store, book_client);
	}

 exit:
	g_object_unref (name_selector);
}

/**
 * e_name_selector_load_books:
 * @name_selector: an #ENameSelector
 *
 * Loads address books available for the @name_selector.
 * This can be called only once and it can be cancelled
 * by e_name_selector_cancel_loading().
 *
 * Since: 3.2
 **/
void
e_name_selector_load_books (ENameSelector *name_selector)
{
	ESourceRegistry *registry;
	GList *list, *iter;
	const gchar *extension_name;

	g_return_if_fail (E_IS_NAME_SELECTOR (name_selector));

	extension_name = E_SOURCE_EXTENSION_ADDRESS_BOOK;
	registry = e_name_selector_get_registry (name_selector);
	list = e_source_registry_list_sources (registry, extension_name);

	for (iter = list; iter != NULL; iter = g_list_next (iter)) {
		ESource *source = E_SOURCE (iter->data);
		ESourceAutocomplete *extension;
		const gchar *extension_name;

		extension_name = E_SOURCE_EXTENSION_AUTOCOMPLETE;
		extension = e_source_get_extension (source, extension_name);

		/* Skip disabled address books. */
		if (!is_source_enabled_with_parents (registry, source))
			continue;

		/* Only load address books with autocomplete enabled,
		 * so as to avoid unnecessary authentication prompts. */
		if (!e_source_autocomplete_get_include_me (extension))
			continue;

		e_client_utils_open_new (
			source, E_CLIENT_SOURCE_TYPE_CONTACTS,
			TRUE, name_selector->priv->cancellable,
			name_selector_book_loaded_cb,
			g_object_ref (name_selector));
	}

	g_list_free_full (list, (GDestroyNotify) g_object_unref);
}

/**
 * e_name_selector_cancel_loading:
 * @name_selector: an #ENameSelector
 *
 * Cancels any pending address book load operations. This might be called
 * before an owner unrefs this @name_selector.
 *
 * Since: 3.2
 **/
void
e_name_selector_cancel_loading (ENameSelector *name_selector)
{
	g_return_if_fail (E_IS_NAME_SELECTOR (name_selector));
	g_return_if_fail (name_selector->priv->cancellable != NULL);

	g_cancellable_cancel (name_selector->priv->cancellable);
}

static void
name_selector_set_registry (ENameSelector *name_selector,
                            ESourceRegistry *registry)
{
	g_return_if_fail (E_IS_SOURCE_REGISTRY (registry));
	g_return_if_fail (name_selector->priv->registry == NULL);

	name_selector->priv->registry = g_object_ref (registry);
}

static void
name_selector_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_REGISTRY:
			name_selector_set_registry (
				E_NAME_SELECTOR (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
name_selector_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_REGISTRY:
			g_value_set_object (
				value,
				e_name_selector_get_registry (
				E_NAME_SELECTOR (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
name_selector_dispose (GObject *object)
{
	ENameSelectorPrivate *priv;
	guint ii;

	priv = E_NAME_SELECTOR_GET_PRIVATE (object);

	if (priv->cancellable) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
		priv->cancellable = NULL;
	}

	for (ii = 0; ii < priv->source_books->len; ii++) {
		SourceBook *source_book;

		source_book = &g_array_index (
			priv->source_books, SourceBook, ii);
		if (source_book->client != NULL)
			g_object_unref (source_book->client);
	}

	for (ii = 0; ii < priv->sections->len; ii++) {
		Section *section;

		section = &g_array_index (priv->sections, Section, ii);
		if (section->entry)
			g_object_weak_unref (
				G_OBJECT (section->entry),
				reset_pointer_cb, object);
		g_free (section->name);
	}

	g_array_set_size (priv->source_books, 0);
	g_array_set_size (priv->sections, 0);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (e_name_selector_parent_class)->dispose (object);
}

static void
name_selector_finalize (GObject *object)
{
	ENameSelectorPrivate *priv;

	priv = E_NAME_SELECTOR_GET_PRIVATE (object);

	g_array_free (priv->source_books, TRUE);
	g_array_free (priv->sections, TRUE);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (e_name_selector_parent_class)->finalize (object);
}

static void
e_name_selector_class_init (ENameSelectorClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (ENameSelectorPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = name_selector_set_property;
	object_class->get_property = name_selector_get_property;
	object_class->dispose = name_selector_dispose;
	object_class->finalize = name_selector_finalize;

	g_object_class_install_property (
		object_class,
		PROP_REGISTRY,
		g_param_spec_object (
			"registry",
			"Registry",
			"Data source registry",
			E_TYPE_SOURCE_REGISTRY,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
e_name_selector_init (ENameSelector *name_selector)
{
	GArray *sections;
	GArray *source_books;

	sections = g_array_new (FALSE, FALSE, sizeof (Section));
	source_books = g_array_new (FALSE, FALSE, sizeof (SourceBook));

	name_selector->priv = E_NAME_SELECTOR_GET_PRIVATE (name_selector);
	name_selector->priv->sections = sections;
	name_selector->priv->model = e_name_selector_model_new ();
	name_selector->priv->source_books = source_books;
	name_selector->priv->cancellable = g_cancellable_new ();
	name_selector->priv->books_loaded = FALSE;
}

/**
 * e_name_selector_new:
 * @registry: an #ESourceRegistry
 *
 * Creates a new #ENameSelector.
 *
 * Returns: A new #ENameSelector.
 **/
ENameSelector *
e_name_selector_new (ESourceRegistry *registry)
{
	g_return_val_if_fail (E_IS_SOURCE_REGISTRY (registry), NULL);

	return g_object_new (
		E_TYPE_NAME_SELECTOR,
		"registry", registry, NULL);
}

/**
 * e_name_selector_get_registry:
 * @name_selector: an #ENameSelector
 *
 * Returns the #ESourceRegistry passed to e_name_selector_new().
 *
 * Returns: the #ESourceRegistry
 *
 * Since: 3.6
 **/
ESourceRegistry *
e_name_selector_get_registry (ENameSelector *name_selector)
{
	g_return_val_if_fail (E_IS_NAME_SELECTOR (name_selector), NULL);

	return name_selector->priv->registry;
}

/* ------- *
 * Helpers *
 * ------- */

static gint
add_section (ENameSelector *name_selector,
             const gchar *name)
{
	GArray *array;
	Section section;

	g_assert (name != NULL);

	memset (&section, 0, sizeof (Section));
	section.name = g_strdup (name);

	array = name_selector->priv->sections;
	g_array_append_val (array, section);
	return array->len - 1;
}

static gint
find_section_by_name (ENameSelector *name_selector,
                      const gchar *name)
{
	GArray *array;
	gint i;

	g_assert (name != NULL);

	array = name_selector->priv->sections;

	for (i = 0; i < array->len; i++) {
		Section *section = &g_array_index (array, Section, i);

		if (!strcmp (name, section->name))
			return i;
	}

	return -1;
}

/* ----------------- *
 * ENameSelector API *
 * ----------------- */

/**
 * e_name_selector_peek_model:
 * @name_selector: an #ENameSelector
 *
 * Gets the #ENameSelectorModel used by @name_selector.
 *
 * Returns: The #ENameSelectorModel used by @name_selector.
 **/
ENameSelectorModel *
e_name_selector_peek_model (ENameSelector *name_selector)
{
	g_return_val_if_fail (E_IS_NAME_SELECTOR (name_selector), NULL);

	return name_selector->priv->model;
}

/**
 * e_name_selector_peek_dialog:
 * @name_selector: an #ENameSelector
 *
 * Gets the #ENameSelectorDialog used by @name_selector.
 *
 * Returns: The #ENameSelectorDialog used by @name_selector.
 **/
ENameSelectorDialog *
e_name_selector_peek_dialog (ENameSelector *name_selector)
{
	g_return_val_if_fail (E_IS_NAME_SELECTOR (name_selector), NULL);

	if (name_selector->priv->dialog == NULL) {
		ESourceRegistry *registry;
		ENameSelectorDialog *dialog;
		ENameSelectorModel *model;

		registry = e_name_selector_get_registry (name_selector);
		dialog = e_name_selector_dialog_new (registry);
		name_selector->priv->dialog = dialog;

		model = e_name_selector_peek_model (name_selector);
		e_name_selector_dialog_set_model (dialog, model);

		g_signal_connect (
			dialog, "delete-event",
			G_CALLBACK (gtk_widget_hide_on_delete), name_selector);
	}

	return name_selector->priv->dialog;
}

/**
 * e_name_selector_show_dialog:
 * @name_selector: an #ENameSelector
 * @for_transient_widget: a widget parent or %NULL
 *
 * Shows the associated dialog, and sets the transient parent to the
 * GtkWindow top-level of "for_transient_widget if set (it should be)
 *
 * Since: 2.32
 **/
void
e_name_selector_show_dialog (ENameSelector *name_selector,
                             GtkWidget *for_transient_widget)
{
	GtkWindow *top = NULL;
	ENameSelectorDialog *dialog;

	g_return_if_fail (E_IS_NAME_SELECTOR (name_selector));

	dialog = e_name_selector_peek_dialog (name_selector);
	if (for_transient_widget)
		top = GTK_WINDOW (gtk_widget_get_toplevel (for_transient_widget));
	if (top)
		gtk_window_set_transient_for (GTK_WINDOW (dialog), top);

	gtk_widget_show (GTK_WIDGET (dialog));
}

/**
 * e_name_selector_peek_section_entry:
 * @name_selector: an #ENameSelector
 * @name: the name of the section to peek
 *
 * Gets the #ENameSelectorEntry for the section specified by @name.
 *
 * Returns: The #ENameSelectorEntry for the named section, or %NULL if it
 * doesn't exist in the #ENameSelectorModel.
 **/
ENameSelectorEntry *
e_name_selector_peek_section_entry (ENameSelector *name_selector,
                                    const gchar *name)
{
	ENameSelectorPrivate *priv;
	ENameSelectorModel *model;
	EDestinationStore *destination_store;
	Section *section;
	gint     n;

	g_return_val_if_fail (E_IS_NAME_SELECTOR (name_selector), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	priv = E_NAME_SELECTOR_GET_PRIVATE (name_selector);
	model = e_name_selector_peek_model (name_selector);

	if (!e_name_selector_model_peek_section (
		model, name, NULL, &destination_store))
		return NULL;

	n = find_section_by_name (name_selector, name);
	if (n < 0)
		n = add_section (name_selector, name);

	section = &g_array_index (name_selector->priv->sections, Section, n);

	if (!section->entry) {
		ESourceRegistry *registry;
		EContactStore *contact_store;
		gchar         *text;
		gint           i;

		registry = e_name_selector_get_registry (name_selector);
		section->entry = e_name_selector_entry_new (registry);
		g_object_weak_ref (G_OBJECT (section->entry), reset_pointer_cb, name_selector);
		if (pango_parse_markup (name, -1, '_', NULL,
					&text, NULL, NULL))  {
			atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (section->entry)), text);
			g_free (text);
		}
		e_name_selector_entry_set_destination_store (section->entry, destination_store);

		/* Create a contact store for the entry and assign our already-open books to it */

		contact_store = e_contact_store_new ();

		for (i = 0; i < priv->source_books->len; i++) {
			SourceBook *source_book = &g_array_index (priv->source_books, SourceBook, i);

			if (source_book->is_completion_book && source_book->client)
				e_contact_store_add_client (contact_store, source_book->client);
		}

		e_name_selector_entry_set_contact_store (section->entry, contact_store);
		g_object_unref (contact_store);
	}

	return section->entry;
}

/**
 * e_name_selector_peek_section_list:
 * @name_selector: an #ENameSelector
 * @name: the name of the section to peek
 *
 * Gets the #ENameSelectorList for the section specified by @name.
 *
 * Returns: The #ENameSelectorList for the named section, or %NULL if it
 * doesn't exist in the #ENameSelectorModel.
 **/

ENameSelectorList *
e_name_selector_peek_section_list (ENameSelector *name_selector,
                                   const gchar *name)
{
	ENameSelectorPrivate *priv;
	ENameSelectorModel *model;
	EDestinationStore *destination_store;
	Section *section;
	gint     n;

	g_return_val_if_fail (E_IS_NAME_SELECTOR (name_selector), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	priv = E_NAME_SELECTOR_GET_PRIVATE (name_selector);
	model = e_name_selector_peek_model (name_selector);

	if (!e_name_selector_model_peek_section (
		model, name, NULL, &destination_store))
		return NULL;

	n = find_section_by_name (name_selector, name);
	if (n < 0)
		n = add_section (name_selector, name);

	section = &g_array_index (name_selector->priv->sections, Section, n);

	if (!section->entry) {
		EContactStore *contact_store;
		ESourceRegistry *registry;
		gchar         *text;
		gint           i;

		registry = name_selector->priv->registry;
		section->entry = (ENameSelectorEntry *)
			e_name_selector_list_new (registry);
		g_object_weak_ref (G_OBJECT (section->entry), reset_pointer_cb, name_selector);
		if (pango_parse_markup (name, -1, '_', NULL,
					&text, NULL, NULL))  {
			atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (section->entry)), text);
			g_free (text);
		}
		e_name_selector_entry_set_destination_store (section->entry, destination_store);

		/* Create a contact store for the entry and assign our already-open books to it */

		contact_store = e_contact_store_new ();
		for (i = 0; i < priv->source_books->len; i++) {
			SourceBook *source_book = &g_array_index (priv->source_books, SourceBook, i);

			if (source_book->is_completion_book && source_book->client)
				e_contact_store_add_client (contact_store, source_book->client);
		}

		e_name_selector_entry_set_contact_store (section->entry, contact_store);
		g_object_unref (contact_store);
	}

	return (ENameSelectorList *) section->entry;
}
