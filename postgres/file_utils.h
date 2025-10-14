#ifndef __TRACEPROV_FILE_UTILS__
#define __TRACEPROV_FILE_UTILS__

int remove_if_exists(const char *);
char *get_injected_str(const char *, const char *, int, void *);
char *get_bi_injected_str(const char *, const char *, int, int, void *);
int remove_and_create(const char *, int);
int remove_files_from_dir(const char *);
int create_dir_if_not_exists(const char *, int);
#endif