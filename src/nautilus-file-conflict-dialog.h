
/* nautilus-file-conflict-dialog: dialog that handles file conflicts
   during transfer operations.

   Copyright (C) 2008, Cosimo Cecchi

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.
   
   Authors: Cosimo Cecchi <cosimoc@gnome.org>
*/

#ifndef NAUTILUS_FILE_CONFLICT_DIALOG_H
#define NAUTILUS_FILE_CONFLICT_DIALOG_H

#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define NAUTILUS_TYPE_FILE_CONFLICT_DIALOG \
	(nautilus_file_conflict_dialog_get_type ())
#define NAUTILUS_FILE_CONFLICT_DIALOG(o) \
	(G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG,\
				     NautilusFileConflictDialog))
#define NAUTILUS_FILE_CONFLICT_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG,\
				 NautilusFileConflictDialogClass))
#define NAUTILUS_IS_FILE_CONFLICT_DIALOG(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG))
#define NAUTILUS_IS_FILE_CONFLICT_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE ((k), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG))
#define NAUTILUS_FILE_CONFLICT_DIALOG_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS ((o), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG,\
				    NautilusFileConflictDialogClass))

typedef struct _NautilusFileConflictDialog        NautilusFileConflictDialog;
typedef struct _NautilusFileConflictDialogClass   NautilusFileConflictDialogClass;
typedef struct _NautilusFileConflictDialogDetails NautilusFileConflictDialogDetails;

struct _NautilusFileConflictDialog {
	GtkDialog parent;
	NautilusFileConflictDialogDetails *details;
};

struct _NautilusFileConflictDialogClass {
	GtkDialogClass parent_class;
};

enum
{
	CONFLICT_RESPONSE_SKIP = 1,
	CONFLICT_RESPONSE_REPLACE = 2,
	CONFLICT_RESPONSE_RENAME = 3,
};

GType nautilus_file_conflict_dialog_get_type (void) G_GNUC_CONST;

NautilusFileConflictDialog* nautilus_file_conflict_dialog_new (GtkWindow *parent);

void nautilus_file_conflict_dialog_set_text (NautilusFileConflictDialog *fcd,
                                             gchar *primary_text,
                                             gchar *secondary_text);
void nautilus_file_conflict_dialog_set_images (NautilusFileConflictDialog *fcd,
                                               GdkPixbuf *source_pixbuf,
                                               GdkPixbuf *destination_pixbuf);
void nautilus_file_conflict_dialog_set_file_labels (NautilusFileConflictDialog *fcd,
                                                    gchar *destination_label,
                                                    gchar *source_label);
void nautilus_file_conflict_dialog_set_conflict_name (NautilusFileConflictDialog *fcd,
                                                      gchar *conflict_name);
void nautilus_file_conflict_dialog_set_replace_button_label (NautilusFileConflictDialog *fcd,
                                                             gchar *label);

void nautilus_file_conflict_dialog_disable_skip (NautilusFileConflictDialog *fcd);
void nautilus_file_conflict_dialog_disable_apply_to_all (NautilusFileConflictDialog *fcd);

char*      nautilus_file_conflict_dialog_get_new_name     (NautilusFileConflictDialog *dialog);
gboolean   nautilus_file_conflict_dialog_get_apply_to_all (NautilusFileConflictDialog *dialog);

#endif /* NAUTILUS_FILE_CONFLICT_DIALOG_H */
