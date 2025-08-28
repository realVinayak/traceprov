#include <fcntl.h>
#include "traceprov.h"

int remove_if_exists(const char *file){
    int rc = 0;
    int can_access = access(file, F_OK);
    if (can_access != 0) return 0;

    if ((rc = remove(file)) != 0){
        PRINT_ON_DEBUG("Error removing file!\n");
        return rc;
    }else{
        PRINT_ON_DEBUG("Removed file successfully!\n");
    }
    return 0;
}
