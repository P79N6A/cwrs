/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* e-name-selector-entry.c - Single-line text entry widget for EDestinations.
 *
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
 *
 * Authors: Hans Petter Jansson <hpj@novell.com>
 */

#if !defined (__LIBEDATASERVERUI_H_INSIDE__) && !defined (LIBEDATASERVERUI_COMPILATION)
#error "Only <libedataserverui/libedataserverui.h> should be included directly."
#endif

#ifndef E_NAME_SELECTOR_ENTRY_H
#define E_NAME_SELECTOR_ENTRY_H

#include <gtk/gtk.h>
#include <libebook/libebook.h>

#include <libedataserverui/e-contact-store.h>
#include <libedataserverui/e-destination-store.h>
#include <libedataserverui/e-tree-model-generator.h>

/* Standard GObject macros */
#define E_TYPE_NAME_SELECTOR_ENTRY \
	(e_name_selector_entry_get_type ())
#define E_NAME_SELECTOR_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_NAME_SELECTOR_ENTRY, ENameSelectorEntry))
#define E_NAME_SELECTOR_ENTRY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_NAME_SELECTOR_ENTRY, ENameSelectorEntryClass))
#define E_IS_NAME_SELECTOR_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_NAME_SELECTOR_ENTRY))
#define E_IS_NAME_SELECTOR_ENTRY_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_NAME_SELECTOR_ENTRY))
#define E_NAME_SELECTOR_ENTRY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_NAME_SELECTOR_ENTRY, ENameSelectorEntryClass))

G_BEGIN_DECLS

typedef struct _ENameSelectorEntry ENameSelectorEntry;
typedef struct _ENameSelectorEntryClass ENameSelectorEntryClass;
typedef struct _ENameSelectorEntryPrivate ENameSelectorEntryPrivate;

struct _ENameSelectorEntry {
	GtkEntry parent;
	ENameSelectorEntryPrivate *priv;
};

struct _ENameSelectorEntryClass {
	GtkEntryClass parent_class;

	void (*updated) (ENameSelectorEntry *entry, gchar *email);

	gpointer reserved1;
	gpointer reserved2;
};

GType		e_name_selector_entry_get_type	(void);
ENameSelectorEntry *
		e_name_selector_entry_new	(ESourceRegistry *registry);
ESourceRegistry *
		e_name_selector_entry_get_registry
						(ENameSelectorEntry *name_selector_entry);
void		e_name_selector_entry_set_registry
						(ENameSelectorEntry *name_selector_entry,
						 ESourceRegistry *registry);
gint		e_name_selector_entry_get_minimum_query_length
						(ENameSelectorEntry *name_selector_entry);
void		e_name_selector_entry_set_minimum_query_length
						(ENameSelectorEntry *name_selector_entry,
						 gint length);
gboolean	e_name_selector_entry_get_show_address
						(ENameSelectorEntry *name_selector_entry);
void		e_name_selector_entry_set_show_address
						(ENameSelectorEntry *name_selector_entry,
						 gboolean show);
EContactStore *	e_name_selector_entry_peek_contact_store
						(ENameSelectorEntry *name_selector_entry);
void		e_name_selector_entry_set_contact_store
						(ENameSelectorEntry *name_selector_entry,
						 EContactStore *contact_store);
EDestinationStore *
		e_name_selector_entry_peek_destination_store
						(ENameSelectorEntry *name_selector_entry);
void		e_name_selector_entry_set_destination_store
						(ENameSelectorEntry *name_selector_entry,
						 EDestinationStore *destination_store);
EDestination *	e_name_selector_entry_get_popup_destination
						(ENameSelectorEntry *name_selector_entry);

/* TEMPORARY API - DO NOT USE */
void		e_name_selector_entry_set_contact_editor_func
						(ENameSelectorEntry *name_selector_entry,
						 gpointer func);
void		e_name_selector_entry_set_contact_list_editor_func
						(ENameSelectorEntry *name_selector_entry,
						 gpointer func);
gchar *		ens_util_populate_user_query_fields
						(GSList *user_query_fields,
						 const gchar *cue_str,
						 const gchar *encoded_cue_str);

G_END_DECLS

#endif /* E_NAME_SELECTOR_ENTRY_H */
