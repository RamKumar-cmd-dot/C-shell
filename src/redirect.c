#include "../include/main.h"

int output_redirection(char* outfile, int append){
    if(outfile==NULL) return -1;
    int flags = O_WRONLY | O_CREAT;
    if(append) flags |= O_APPEND;
    else flags |= O_TRUNC;
    int fd = open(outfile, flags, 0644);
    if(fd<0){
        printf("cshell: cannot create file\n");
        return -2;
    }
    return fd;
}

int input_redirection(char** args, int count){
    if(count==0) return -1;
    char tmp_path[] = "/tmp/cshell_redirXXXXXX";
    int tmp_fd = mkstemp(tmp_path);
    if(tmp_fd<0){
        perror("mkstemp");
        return -2;
    }
    unlink(tmp_path);
    char buffer[4096];
    for(int i=0;i<count;i++){
        int src_fd = open(args[i], O_RDONLY);
        if(src_fd<0){
            printf("cshell: no such file or directory\n");
            close(tmp_fd);
            return -2;
        }
        ssize_t n;
        while((n = read(src_fd, buffer, sizeof(buffer)))>0){
            if(write(tmp_fd, buffer, n)!=n){
                perror("write");
                close(src_fd);
                close(tmp_fd);
                return -2;
            }
        }
        close(src_fd);
    }
    lseek(tmp_fd, 0, SEEK_SET);
    return tmp_fd;
}