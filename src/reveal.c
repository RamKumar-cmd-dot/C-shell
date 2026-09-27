#include "../include/main.h"

int resolve_path(char* arg, char* home, char* previous_directory, char* result){
    char current[256];
    if(strcmp(arg, "~")==0){
        strcpy(result, home);
        return 1;
    }
    if(strcmp(arg, ".")==0){
        if(getcwd(result, sizeof(result))==NULL) return 0;
        return 1;
    }
    if(strcmp(arg, "..")==0){
        if(getcwd(current, sizeof(current)) == NULL) return 0;
        if(strcmp(current, "/")==0){
            strcpy(result, "/");
            return 1;
        }
        char *last = strrchr(current, '/');
        if(last==current) strcpy(result, "/");
        else{
            *last = '\0';
            strcpy(result, current);
        }
        return 1;
    }
    if(strcmp(arg, "-")==0){
        if(previous_directory==NULL || previous_directory[0]=='\0') return 0;
        strcpy(result, previous_directory);
        return 1;
    }
    if(arg[0] == '/'){
        strcpy(result, arg);
        return 1;
    }
    if(getcwd(current, sizeof(current)) == NULL) return 0;
    if(snprintf(result, 256, "%s/%s", current, arg)>=256) return 0;
    return 1;
}

int is_directory(char* path){
    struct stat st;
    if(stat(path, &st)!=0) return 0;
    return S_ISDIR(st.st_mode);
}

int compare_entries(const struct dirent **a, const struct dirent **b){
    return strcmp((*a)->d_name, (*b)->d_name);
}

int list_directory(char* path, int show_hidden){
    struct dirent **entries;
    int count = scandir(path, &entries, NULL, compare_entries);
    if(count < 0) return 0;
    for(int i=0;i<count;i++){
        char* name = entries[i]->d_name;
        if(strcmp(name, ".")==0 || strcmp(name, "..")==0){
            free(entries[i]);
            continue;
        }
        if(!show_hidden && name[0]=='.'){
            free(entries[i]);
            continue;
        }
        printf("%s\n", name);
        free(entries[i]);
    }
    free(entries);
    return 1;
}

void list_recursive(char* directory, char* prefix, int show_hidden){
    struct dirent** entries;
    int count = scandir(directory, &entries, NULL, compare_entries);
    if(count<0) return ;
    for(int i=0;i<count;i++){
        char* name = entries[i]->d_name;
        if(strcmp(name, ".")==0 || strcmp(name, "..")==0){
            free(entries[i]);
            continue;
        }
        if(!show_hidden && name[0]=='.'){
            free(entries[i]);
            continue;
        }
        char child[256];
        snprintf(child, sizeof(child), "%s/%s", directory, name);
        struct stat st;
        if(stat(child, &st)!=0){
            free(entries[i]);
            return ;
        }
        char current_path[256];
        if(prefix[0]=='\0') snprintf(current_path, sizeof(current_path), "%s", name);
        else snprintf(current_path, sizeof(current_path), "%s/%s", prefix, name);
        if(S_ISDIR(st.st_mode)) printf("%s/\n", current_path);
        else printf("%s\n", current_path);
        if(S_ISDIR(st.st_mode)) list_recursive(child, current_path, show_hidden);
        free(entries[i]);
    }
    free(entries);
}

int reveal(char** args, int count, char* home, char* previous_directory){
    char path[256];
    int show_hidden = 0;
    int recursive = 0;
    char *target = NULL;
    for(int i=0;i<count;i++){
        if(args[i][0]=='-' && args[i][1]!='\0'){
            for(int j=1;args[i][j]!='\0';j++){
                if(args[i][j]=='a') show_hidden = 1;
                else if(args[i][j]=='t') recursive = 1;
                else return 0;
            }
        }
        else{
            if(target!=NULL) return 0;
            target = args[i];
        }
    }
    if(target==NULL){
        if(getcwd(path, sizeof(path))==NULL) return 0;
    }
    else{
        if(!resolve_path(target, home, previous_directory, path)) return 0;
    }
    if(!is_directory(path)) return 0;
    if(recursive) list_recursive(path, "", show_hidden);
    else list_directory(path, show_hidden);
    return 1;
}