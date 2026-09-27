#include "../include/main.h"
#include <limits.h>
#include "../include/locate.h"
int is_executable(char* path){
    return access(path, X_OK)==0;
}
int search_directory(char* directory, char* filename){
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", directory, filename);
    if(is_executable(path)){
        char absolute[4096];
        if(realpath(path, absolute)!=NULL){
            printf("%s\n", absolute);
            return 1;
        }
    }
    return 0;
}
void locate_one(char* filename){
    char cwd[4096];
    if(getcwd(cwd, sizeof(cwd))!=NULL) search_directory(cwd, filename);
    char* path_env = getenv("PATH");
    if(path_env==NULL){
        printf("locate: command not found (%s)\n", filename);
        return;
    }
    char* path_copy = strdup(path_env);
    if(path_copy==NULL) return;
    char* directory = strtok(path_copy, ":");
    while(directory != NULL){
        if(directory[0]=='\0') search_directory(".", filename);
        else search_directory(directory, filename);
        directory = strtok(NULL, ":");
    }
    free(path_copy);
}

int locate(char** args, int count){
    for(int i=0;i<count;i++){
        int found = 0;
        char cwd[4096];
        if(getcwd(cwd, sizeof(cwd))!=NULL){
            if(search_directory(cwd, args[i])) found = 1;
        }
        char *path_env = getenv("PATH");
        if(path_env!=NULL){
            char *path_copy = strdup(path_env);
            if(path_copy!=NULL){
                char *directory = strtok(path_copy, ":");
                while(directory != NULL){
                    if(directory[0]=='\0'){
                        if(search_directory(".", args[i])) found = 1;
                    }
                    else{
                        if(search_directory(directory, args[i])) found = 1;
                    }
                    directory = strtok(NULL, ":");
                }
                free(path_copy);
            }
        }
        if(!found) printf("locate: command not found (%s)\n", args[i]);
    }
    return 1;
}
