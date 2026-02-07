#ifndef __TP_FILE_UTILS__
#define __TP_FILE_UTILS__


int remove_if_exists(const char *file);
char *get_injected_str(const char *file_template_name, const char *dir, int number, void *buffer);
char *get_bi_injected_str(const char *file_template_name, const char *dir, int first, int second, void *buffer);
int remove_and_create(const char *file_name, int size);
int create_dir_if_not_exists(const char *dir, int mode);
int remove_files_from_dir(const char *dir);

#endif