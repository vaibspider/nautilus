
/* fm-directory-view.h
 *
 * Copyright (C) 1999, 2000  Free Software Foundaton
 * Copyright (C) 2000  Eazel, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Rebecca Schulman <rebecka@eazel.com>
 */

#ifndef NAUTILUS_CLIPBOARD_H
#define NAUTILUS_CLIPBOARD_H

#include <gtk/gtk.h>

void nautilus_clipboard_clear_if_colliding_uris    (GtkWidget          *widget,
						    const GList        *item_uris,
						    GdkAtom             copied_files_atom);
GtkClipboard* nautilus_clipboard_get                (GtkWidget          *widget);
GList* nautilus_clipboard_get_uri_list_from_selection_data
						   (GtkSelectionData   *selection_data,
						    gboolean           *cut,
						    GdkAtom             copied_files_atom);

#endif /* NAUTILUS_CLIPBOARD_H */
