#include "../include/main.h"

int open_output_files(char** outfiles, int* append_flags, int file_count, int* out_fds){
    for(int i=0;i<file_count;i++){
        int flags = O_WRONLY | O_CREAT;
        if(append_flags[i]) flags |= O_APPEND;
        else flags |= O_TRUNC;
        int fd = open(outfiles[i], flags, 0644);
        if(fd<0){
            printf("cshell: unable to create file for writing\n");
            for(int j=0;j<i;j++) close(out_fds[j]);
            return 0;
        }
        out_fds[i] = fd;
    }
    return 1;
}

void close_fds(int* fds, int count){
    for(int i=0;i<count;i++) close(fds[i]);
}