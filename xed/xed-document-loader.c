/*
 * xed-document-loader.c
 * This file is part of xed
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the xed Team, 2005-2007. See the AUTHORS file for a
 * list of people on the xed Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "xed-document-loader.h"
#include "xed-debug.h"
#include "xed-metadata-manager.h"
#include "xed-utils.h"
#include "xed-marshal.h"
#include "xed-enum-types.h"

/* Those are for the the xed_document_loader_new() factory */
#include "xed-gio-document-loader.h"

G_DEFINE_ABSTRACT_TYPE(XedDocumentLoader, xed_document_loader, G_TYPE_OBJECT)

/* Signals */

enum {
	LOADING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Properties */

enum
{
	PROP_0,
	PROP_DOCUMENT,
	PROP_URI,
	PROP_ENCODING,
	PROP_NEWLINE_TYPE
};

static void
xed_document_loader_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	XedDocumentLoader *loader = XED_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_return_if_fail (loader->document == NULL);
			loader->document = g_value_get_object (value);
			break;
		case PROP_URI:
			g_return_if_fail (loader->uri == NULL);
			loader->uri = g_value_dup_string (value);
			break;
		case PROP_ENCODING:
			g_return_if_fail (loader->encoding == NULL);
			loader->encoding = g_value_get_boxed (value);
			break;
		case PROP_NEWLINE_TYPE:
			loader->auto_detected_newline_type = g_value_get_enum (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
xed_document_loader_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	XedDocumentLoader *loader = XED_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value, loader->document);
			break;
		case PROP_URI:
			g_value_set_string (value, loader->uri);
			break;
		case PROP_ENCODING:
			g_value_set_boxed (value, xed_document_loader_get_encoding (loader));
			break;
		case PROP_NEWLINE_TYPE:
			g_value_set_enum (value, loader->auto_detected_newline_type);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
xed_document_loader_finalize (GObject *object)
{
	XedDocumentLoader *loader = XED_DOCUMENT_LOADER (object);

	g_free (loader->uri);

	if (loader->info)
		g_object_unref (loader->info);

	G_OBJECT_CLASS (xed_document_loader_parent_class)->finalize (object);
}

static void
xed_document_loader_dispose (GObject *object)
{
	XedDocumentLoader *loader = XED_DOCUMENT_LOADER (object);

	if (loader->info != NULL)
	{
		g_object_unref (loader->info);
		loader->info = NULL;
	}

	G_OBJECT_CLASS (xed_document_loader_parent_class)->dispose (object);
}

static void
xed_document_loader_class_init (XedDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = xed_document_loader_finalize;
	object_class->dispose = xed_document_loader_dispose;
	object_class->get_property = xed_document_loader_get_property;
	object_class->set_property = xed_document_loader_set_property;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							      "Document",
							      "The XedDocument this XedDocumentLoader is associated with",
							      XED_TYPE_DOCUMENT,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_URI,
					 g_param_spec_string ("uri",
							      "URI",
							      "The URI this XedDocumentLoader loads the document from",
							      "",
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_ENCODING,
					 g_param_spec_boxed ("encoding",
							     "Encoding",
							     "The encoding of the saved file",
							     XED_TYPE_ENCODING,
							     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY |
							     G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_NEWLINE_TYPE,
	                                 g_param_spec_enum ("newline-type",
	                                                    "Newline type",
	                                                    "The accepted types of line ending",
	                                                    XED_TYPE_DOCUMENT_NEWLINE_TYPE,
	                                                    XED_DOCUMENT_NEWLINE_TYPE_LF,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_STATIC_NAME |
	                                                    G_PARAM_STATIC_BLURB));

	signals[LOADING] =
		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XedDocumentLoaderClass, loading),
			      NULL, NULL,
			      xed_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);
}

static void
xed_document_loader_init (XedDocumentLoader *loader)
{
	loader->used = FALSE;
	loader->auto_detected_newline_type = XED_DOCUMENT_NEWLINE_TYPE_DEFAULT;
}

void
xed_document_loader_loading (XedDocumentLoader *loader,
			       gboolean             completed,
			       GError              *error)
{
	/* the object will be unrefed in the callback of the loading signal
	 * (when completed == TRUE), so we need to prevent finalization.
	 */
	if (completed)
	{
		g_object_ref (loader);
	}

	g_signal_emit (loader, signals[LOADING], 0, completed, error);

	if (completed)
	{
		if (error == NULL)
			xed_debug_message (DEBUG_LOADER, "load completed");
		else
			xed_debug_message (DEBUG_LOADER, "load failed");

		g_object_unref (loader);
	}
}

/* This is a factory method that returns an appopriate loader
 * for the given uri.
 */
XedDocumentLoader *
xed_document_loader_new (XedDocument       *doc,
			   const gchar         *uri,
			   const XedEncoding *encoding)
{
	XedDocumentLoader *loader;
	GType loader_type;

	g_return_val_if_fail (XED_IS_DOCUMENT (doc), NULL);

	/* At the moment we just use gio loader in all cases...
	 * In the future it would be great to have a PolicyKit
	 * loader to get permission to save systen files etc */
	loader_type = XED_TYPE_GIO_DOCUMENT_LOADER;

	loader = XED_DOCUMENT_LOADER (g_object_new (loader_type,
						      "document", doc,
						      "uri", uri,
						      "encoding", encoding,
						      NULL));

	return loader;
}

/* If enconding == NULL, the encoding will be autodetected */
void
xed_document_loader_load (XedDocumentLoader *loader)
{
	xed_debug (DEBUG_LOADER);

	g_return_if_fail (XED_IS_DOCUMENT_LOADER (loader));

	/* the loader can be used just once, then it must be thrown away */
	g_return_if_fail (loader->used == FALSE);
	loader->used = TRUE;

	XED_DOCUMENT_LOADER_GET_CLASS (loader)->load (loader);
}

gboolean 
xed_document_loader_cancel (XedDocumentLoader *loader)
{
	xed_debug (DEBUG_LOADER);

	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), FALSE);

	return XED_DOCUMENT_LOADER_GET_CLASS (loader)->cancel (loader);
}

XedDocument *
xed_document_loader_get_document (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->document;
}

/* Returns STDIN_URI if loading from stdin */
const gchar *
xed_document_loader_get_uri (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->uri;
}

goffset
xed_document_loader_get_bytes_read (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), 0);

	return XED_DOCUMENT_LOADER_GET_CLASS (loader)->get_bytes_read (loader);
}

const XedEncoding *
xed_document_loader_get_encoding (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), NULL);

	if (loader->encoding != NULL)
		return loader->encoding;

	g_return_val_if_fail (loader->auto_detected_encoding != NULL, 
			      xed_encoding_get_current ());

	return loader->auto_detected_encoding;
}

XedDocumentNewlineType
xed_document_loader_get_newline_type (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader),
			      XED_DOCUMENT_NEWLINE_TYPE_LF);

	return loader->auto_detected_newline_type;
}

GFileInfo *
xed_document_loader_get_info (XedDocumentLoader *loader)
{
	g_return_val_if_fail (XED_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->info;
}
