#include <fcntl.h>
#include "traceprov.h"
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <dirent.h>


int remove_if_exists(const char *file){
    int rc = 0;
    int can_access = access(file, F_OK);
    if (can_access != 0) return 0;

    if ((rc = remove(file)) != 0){
        return rc;
    }
    return 0;
}

char *get_injected_str(const char *file_template_name, int number, void *buffer){

    char *file_name = (char*)buffer;
    
    size_t file_name_size = (strlen(file_template_name)
        // We can put in 32 chars at max.
        + (32 * 8) 
        + 4
    );

    if (file_name == NULL){

        // In some cases, we can avoid the repeated memory allocation by using previously allocated
        // buffer. The caller assumes all the responsiblity of making sure the buffer is correctly sized.

        file_name = (char *)malloc(file_name_size);
        if (file_name == NULL){
            return NULL;
        }
    }


    memset(file_name, 0, file_name_size);
    sprintf(file_name, file_template_name, number);

    return file_name;
}

char *get_bi_injected_str(const char *file_template_name, int first, int second, void *buffer){
    char *file_name = (char*)buffer;
    
    size_t file_name_size = (strlen(file_template_name)
        // We can put in 32 chars at max.
        + (32 * 8) 
        + 4
    );

    if (file_name == NULL){

        // In some cases, we can avoid the repeated memory allocation by using previously allocated
        // buffer. The caller assumes all the responsiblity of making sure the buffer is correctly sized.

        file_name = (char *)malloc(file_name_size);
        if (file_name == NULL){
            return NULL;
        }
    }


    memset(file_name, 0, file_name_size);
    sprintf(file_name, file_template_name, first, second);

    return file_name;
}

int remove_and_create(const char *file_name, int size){
    
    if (remove_if_exists(file_name)) return -1;

    int fd = open(file_name, O_CREAT | O_RDWR, TRACEPROV_FILE_PERMISSION);

    if (fd < 0){
        return -1;
    }

    if (ftruncate(fd, size)){
        return -1;
    }

    return fd;
}

// Taken from https://stackoverflow.com/questions/11007494/how-to-delete-all-files-in-a-folder-but-not-delete-the-folder-using-nix-standar
int remove_files_from_dir(const char *dir){
    // These are data types defined in the "dirent" header
    DIR *folder = opendir(dir);
    struct dirent *next_file;
    char filepath[256];

    while ( (next_file = readdir(folder)) != NULL )
    {
        // skip "." and ".." entries
        if (strcmp(next_file->d_name, ".")==0 || strcmp(next_file->d_name, "..")==0)
            continue;

        // build the path for each file in the folder
        sprintf(filepath, "%s/%s", dir, next_file->d_name);
        if (remove(filepath)){
            return 1;
        }
    }
    closedir(folder);
    return 0;
}