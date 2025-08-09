#include "mysql.h"
#include <string.h>
#include "mysql/udf_registration_types.h"
#include "current_thd.h"
#include "sql_class.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PROV_FILE "/usr/local/mysql/data/provfile.prov"
// #define PROV_FILE "provmap.map"

#define GIGA_BYTE 1024 * 1024 * 1024

struct mmap_init_row {
    long long primary_key;
    long long group_cnt;
};

struct mmap_later_row {
    long long in_result;
};

struct context {
    mmap_init_row *last_fwd_ptr;
    mmap_later_row *last_ptr;
    long long group_cnt;
};

int remove_if_exists(){
    int rc = 0;
    int can_access = access(PROV_FILE, F_OK);
    if (can_access != 0) return 0;

    if ((rc = remove(PROV_FILE)) != 0){
        printf("Error removing file!");
        return rc;
    }else{
        printf("Removed file successfully!");
    }
    return 0;
}

int create_file(){
    int fd = open(PROV_FILE, O_CREAT | O_RDWR);
    if (fd < 0) return 1;
    off_t moved = lseek(fd, GIGA_BYTE - 1, SEEK_SET);
    if (moved == -1) return 1;
    char buff[1] = {0};
    write(fd, buff, 1);
    void *ptr = mmap64(
        NULL, 
        GIGA_BYTE,
        PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (ptr == MAP_FAILED){
        close(fd);
        return 1;
    }
    printf("using the ptr: %p\n", ptr);
    fflush(stdout);
    // Set the ptr nicely.
    current_thd->provenance_ptr = ptr;
    return 0;
}


int set_up_mmap(char *message){
    // Already set-up correctly
    if (current_thd->provenance_ptr != nullptr) return 0;
    int rc = 0;

    if ((rc = remove_if_exists()) != 0) return rc;
    rc = create_file();
    return rc;
}

extern "C" bool trackprova_init(UDF_INIT *initd, UDF_ARGS *, char *message){
    return false;
    auto *data = new context;
    if (!data){
        strcpy(message, "Could not allocate memory!");
        return true;
    }

    if (set_up_mmap(message)) return true;
    
    initd->ptr = static_cast<char*>(static_cast<void *>(data));
    data->group_cnt = 0;
    void *prov_ptr = current_thd->provenance_ptr;
    printf("PROV PTR IS: %p\n", prov_ptr);
    data->last_fwd_ptr = static_cast<mmap_init_row*>(prov_ptr);
    data->last_ptr = static_cast<mmap_later_row*>(static_cast<void *>(static_cast<char *>(prov_ptr) + GIGA_BYTE));
    printf("FWD: %p, back: %p\n", data->last_fwd_ptr, data->last_ptr);
    fflush(stdout);
    return false;
}

extern "C" void trackprova_deinit(UDF_INIT *initd){
    auto *data = static_cast<context *>(static_cast<void *>(initd->ptr));
    delete data;
}

extern "C" void trackprova_add(
    UDF_INIT *initd, 
    UDF_ARGS *args, 
    unsigned char *, 
    unsigned char *){
        // Need the context ptr for adding rows
        auto *context_ptr = static_cast<context *>(static_cast<void *>(initd->ptr));
        void *arg0 = args->args[0];
        struct mmap_init_row forward_row = {
            .primary_key = *(static_cast<long long *>(arg0)),
            .group_cnt = context_ptr->group_cnt
        };

        auto row = context_ptr->last_fwd_ptr;
        row->group_cnt = forward_row.group_cnt;
        row->primary_key = forward_row.primary_key;
        // Increment the ptr for next usage.
        context_ptr->last_fwd_ptr += 1;
    }

extern "C" void trackprova_clear(
    UDF_INIT *initid, 
    unsigned char *,
    unsigned char *){
        auto *context_ptr = static_cast<context *>(static_cast<void *>(initid->ptr));
        // Increment the group counter.
        // Not sure if this is the correct place for it, yet.
        context_ptr->group_cnt += 1;
        context_ptr->last_ptr -= 1;
    }

extern "C" long long trackprova(
    UDF_INIT *initid, 
    UDF_ARGS *,
    unsigned char *, 
    unsigned char *
){
    auto *context_ptr = static_cast<context *>(static_cast<void *>(initid->ptr));
    return reinterpret_cast<long long>(context_ptr->last_ptr);
}