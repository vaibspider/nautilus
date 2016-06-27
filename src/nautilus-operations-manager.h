#ifndef NAUTILUS_OPERATIONS_MANAGER_H
#define NAUTILUS_OPERATIONS_MANAGER_H


typedef struct {
    int id;
    char *new_name;
    gboolean apply_to_all;
} FileConflictResponse;


FileConflictResponse * get_copy_move_file_conflict_response (GtkWindow *parent_window,
                                                             GFile     *src,
                                                             GFile     *dest,
                                                             GFile     *dest_dir);
FileConflictResponse * get_extract_file_conflict_response (GtkWindow *parent_window,
                                                           GFile     *src,
                                                           GFile     *dest,
                                                           GFile     *dest_dir);

typedef struct {
    int id;
    char *new_name;
} NameConflictResponse;

NameConflictResponse * get_extract_name_conflict_response (GtkWindow *parent_window,
                                                           GFile     *source);

#endif
