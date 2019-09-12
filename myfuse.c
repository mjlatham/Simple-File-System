/* Do not change! */
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
/******************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>

#include "myfilesystem.h"

char * file_data_file_name = NULL;
char * directory_table_file_name = NULL;
char * hash_data_file_name = NULL;

int myfuse_getattr(const char * name, struct stat * result) {
    memset(result, 0, sizeof(struct stat));
    if (strcmp(name, "/") == 0) {
        result->st_mode = S_IFDIR;
    } else {
        result->st_mode = S_IFREG;
        result->st_size = (off_t) file_size((char *) name, fuse_get_context()->private_data);
    }
    return 0;
}

int myfuse_readdir(const char * name, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
    if (strcmp(name, "/") == 0) {
        filler(buf, "test_file", NULL, 0);
    }
    return 0;
}

int myfuse_unlink(const char * name) {
    remove(name);

    int ret = delete_file((char *) name, fuse_get_context()->private_data);

    return ret;
}

int myfuse_rename(const char * old, const char * new) {
    return rename_file((char *) old, (char *) new, fuse_get_context()->private_data);
}

int myfuse_truncate(const char * name, off_t length) {
    return resize_file((char *) name, (size_t) length, fuse_get_context()->private_data);
}

int myfuse_open(const char * name, struct fuse_file_info * info) {
    FILE *file = fopen(name, "r");

    if (!file || name == NULL) {
        perror("File does not exist");
        return 0;
    }

    fclose(file);

    return 1;
}

int myfuse_read(const char * name , char * buf, size_t count, off_t offset, struct fuse_file_info * info) {
    return read_file((char *) name, (size_t) offset, count, buf, fuse_get_context()->private_data);
}

int myfuse_write(const char * name, const char * buf, size_t count, off_t offset, struct fuse_file_info * info) {
    return write_file((char *) name, (size_t) offset, count, (char *) buf, fuse_get_context()->private_data);
}

int myfuse_release(const char * name, struct fuse_file_info * info) {
    FILE *file = fopen(name, "r");

    if (!file || name == NULL) {
        perror("File does not exist");
        return 0;
    }

    fclose(file);

    return 1;
}

void * myfuse_init(struct fuse_conn_info * conn) {
    init_fs("file_data_file_name", "directory_table_file_name", "hash_data_file_name", 1);
    //helper = fuse_get_context()->private_data;
    //return helper;
    return fuse_get_context()->private_data;
}

void myfuse_destroy(void * something) {
    return close_fs(fuse_get_context()->private_data);
}

int myfuse_create(const char * name, mode_t mode, struct fuse_file_info * info) {
    return create_file((char *) name, 0, fuse_get_context()->private_data);
}

struct fuse_operations operations = {
    .getattr = myfuse_getattr,
    .readdir = myfuse_readdir,
    /* FILL OUT BELOW FUNCTION POINTERS */
    .unlink = myfuse_unlink,
    .rename = myfuse_rename,
    .truncate = myfuse_truncate,
    .open = myfuse_open,
    .read = myfuse_read,
    .write = myfuse_write,
    .release = myfuse_release,
    .init = myfuse_init,
    .destroy = myfuse_destroy,
    .create = myfuse_create
    /**/
};

int main(int argc, char * argv[]) {
    // MODIFY (OPTIONAL)
    if (argc >= 5) {
        if (strcmp(argv[argc-4], "--files") == 0) {
            file_data_file_name = argv[argc-3];
            directory_table_file_name = argv[argc-2];
            hash_data_file_name = argv[argc-1];
            argc -= 4;
        }
    }
    // After this point, you have access to file_data_file_name, directory_table_file_name and hash_data_file_name
    int ret = fuse_main(argc, argv, &operations, NULL);
    return ret;
}
