#include "../include/main.h"

static const char* type_str(const char* path){
    struct stat st;
    if(lstat(path, &st)!=0) return "unknown";
    if(S_ISDIR(st.st_mode)) return "DIR";
    if(S_ISCHR(st.st_mode)) return "CHR";
    if(S_ISBLK(st.st_mode)) return "BLK";
    if(S_ISFIFO(st.st_mode)) return "FIFO";
    if(S_ISSOCK(st.st_mode)) return "SOCK";
    if(S_ISREG(st.st_mode)) return "REG";
    return "unknown";
}

int spy_command(char** args, int count){
    if(count > 1){
        printf("spy: invalid syntax\n");
        return 0;
    }
    pid_t pid;
    if(count==0) pid = getpid();
    else{
        char* end;
        long v = strtol(args[0], &end, 10);
        if(args[0][0]=='\0' || *end!='\0' || v<0){
            printf("spy: invalid syntax\n");
            return 0;
        }
        pid = (pid_t)v;
    }
    char procdir[64];
    snprintf(procdir, sizeof(procdir), "/proc/%d", pid);
    struct stat st;
    if(stat(procdir, &st)!=0){
        printf("spy: no such process\n");
        return 0;
    }

    int permission_denied = 0;
    char link[4096], target[4096];
    ssize_t n;

    printf("PID FD TYPE PATH\n");

    snprintf(link, sizeof(link), "/proc/%d/cwd", pid);
    n = readlink(link, target, sizeof(target)-1);
    if(n>0){
        target[n]='\0';
        printf("%d cwd %s %s\n", pid, type_str(target), target);
    }
    else if(errno == EACCES || errno == EPERM){
        permission_denied = 1;
    }

    snprintf(link, sizeof(link), "/proc/%d/exe", pid);
    n = readlink(link, target, sizeof(target)-1);
    if(n>0){
        target[n]='\0';
        printf("%d txt %s %s\n", pid, type_str(target), target);
    }
    else if(errno == EACCES || errno == EPERM){
        permission_denied = 1;
    }

    snprintf(link, sizeof(link), "/proc/%d/maps", pid);
    FILE* f = fopen(link, "r");
    if(f){
        char line[1024];
        char seen[128][512];
        int seen_count = 0;
        while(fgets(line, sizeof(line), f)){
            char* path = strchr(line, '/');
            if(!path) continue;
            size_t len = strlen(path);
            if(len && path[len-1]=='\n') path[len-1]='\0';
            int dup = 0;
            for(int i=0;i<seen_count;i++){
                if(strcmp(seen[i], path)==0){ dup = 1; break; }
            }
            if(dup) continue;
            if(seen_count < 128) strncpy(seen[seen_count++], path, sizeof(seen[0])-1);
            printf("%d mem %s %s\n", pid, type_str(path), path);
        }
        fclose(f);
    }
    else if(errno == EACCES || errno == EPERM){
        permission_denied = 1;
    }

    snprintf(link, sizeof(link), "/proc/%d/fd", pid);
    DIR* d = opendir(link);
    if(d){
        int fds[4096];
        int fd_count = 0;
        struct dirent* de;
        while((de = readdir(d)) != NULL){
            if(de->d_name[0] == '.') continue;
            if(fd_count < 4096) fds[fd_count++] = atoi(de->d_name);
        }
        closedir(d);
        for(int i=0;i<fd_count;i++){
            for(int j=i+1;j<fd_count;j++){
                if(fds[j]<fds[i]){
                    int t=fds[i];
                    fds[i]=fds[j];
                    fds[j]=t;
                }
            }
        }
        for(int i=0;i<fd_count;i++){
            snprintf(link, sizeof(link), "/proc/%d/fd/%d", pid, fds[i]);
            n = readlink(link, target, sizeof(target)-1);
            if(n>0){
                target[n]='\0';
                printf("%d %d %s %s\n", pid, fds[i], type_str(target), target);
            }
        }
    }
    else if(errno == EACCES || errno == EPERM){
        permission_denied = 1;
    }

    if(permission_denied){
        printf("spy: permission denied\n");
        return 0;
    }
    return 1;
}