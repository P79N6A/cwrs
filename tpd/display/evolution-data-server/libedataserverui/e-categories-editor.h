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

#if !defined (__LIBEDATASERVERUI_H_INSIDE__) && !defined (LIBEDATASERVERUI_COMPILATION)
#error "Only <libedataserverui/libedataserverui.h> should be included directly."
#endif

#ifndef E_CATEGORIES_EDITOR_H
#define E_CATEGORIES_EDITOR_H

#include <gtk/gtk.h>

/* Standard GObject macros */
#define E_TYPE_CATEGORIES_EDITOR \
	(e_categories_editor_get_type ())
#define E_CATEGORIES_EDITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_CATEGORIES_EDITOR, ECategoriesEditor))
#define E_CATEGORIES_EDITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_CATEGORIES_EDITOR, ECategoriesEditorClass))
#define E_IS_CATEGORIES_EDITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_CATEGORIES_EDITOR))
#define E_IS_CATEGORIES_EDITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_CATEGORIES_EDITOR))
#define E_CATEGORIES_EDITOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_CATEGORIES_EDITOR, ECategoriesEditorClass))

G_BEGIN_DECLS

typedef struct _ECategoriesEditor ECategoriesEditor;
typedef struct _ECategoriesEditorClass ECategoriesEditorClass;
typedef struct _ECategoriesEditorPrivate ECategoriesEditorPrivate;

/**
 * ECategoriesEditor:
 *
 * Contains only private data that should be read and manipulated using the
 * functions below.
 *
 * Since: 3.2
 **/
struct _ECategoriesEditor {
	GtkTable parent;
	ECategoriesEditorPrivate *priv;
};

struct _ECategoriesEditorClass {
	GtkTableClass parent_class;

	void		(*entry_changed)	(GtkEntry *entry);
};

GType		e_categories_editor_get_type	(void);
GtkWidget *	e_categories_editor_new		(void);
gchar *		e_categories_editor_get_categories
						(ECategoriesEditor *editor);
void		e_categories_editor_set_categories
						(ECategoriesEditor *editor,
						 const gchar *categories);
gboolean	e_categories_editor_get_entry_visible
						(ECategoriesEditor *editor);
void		e_categories_editor_set_entry_visible
						(ECategoriesEditor *editor,
						 gboolean entry_visible);

G_END_DECLS

#endif /* E_CATEGORIES_EDITOR_H */
