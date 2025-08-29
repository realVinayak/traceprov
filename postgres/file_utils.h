#ifndef __TRACEPROV_FILE_UTILS__
#define __TRACEPROV_FILE_UTILS__

int remove_if_exists(const char *);
char *get_injected_str(const char *, int, void *);
int remove_and_create(const char *, int);

#endif