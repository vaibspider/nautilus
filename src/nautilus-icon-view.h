/* nautilus-icon-view.h
 *
 * Copyright (C) 2016 Carlos Soriano <csoriano@gnome.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NAUTILUS_ICON_VIEW_H
#define NAUTILUS_ICON_VIEW_H

#include <glib.h>
#include <gtk/gtk.h>
#include "nautilus-files-view.h"
#include "nautilus-window-slot.h"

G_BEGIN_DECLS

#define NAUTILUS_TYPE_ICON_VIEW (nautilus_icon_view_get_type())

G_DECLARE_DERIVABLE_TYPE (NautilusIconView, nautilus_icon_view, NAUTILUS, ICON_VIEW, NautilusFilesView)

struct _NautilusIconViewClass
{
  NautilusFilesViewClass parent;
};

NautilusIconView *nautilus_icon_view_new (NautilusWindowSlot *slot);

G_END_DECLS

#endif /* NAUTILUS_ICON_VIEW_H */

