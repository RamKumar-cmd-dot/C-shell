#include "../include/main.h"

int hop_home(const char* home){
    if(chdir(home)!=0) return 0;
    return 1;
}

int hop_parent(void){
    if(chdir("..")!=0) return 0;
    return 1;
}

int hop_previous(char* previous_directory){
    if(previous_directory[0]=='\0') return 1;
    if(chdir(previous_directory)!=0) return 0;
    return 1;
}

int hop_path(char* path){
    if(chdir(path)!=0) return 0;
    return 1;
}

int hop(char** args, int count, const char* home, char* previous_directory){
    
    if(count==0){
        char current[256];
        char new_path[256];
        if(getcwd(current, sizeof(current))==NULL) return 0;
        if(chdir(home)!=0) return 0;
        if(getcwd(new_path, sizeof(new_path))==NULL) return 0;
        update_frecency(new_path);
        save_frecency();
        strcpy(previous_directory, current);
        return 1;
    }
    for(int i=0;i<count;i++){
        char current[256];
        if(getcwd(current, sizeof(current))==NULL) return 0;
        if(strcmp(args[i], "~")==0){
            if(hop_home(home)==0) return 0;
        }
        else if(strcmp(args[i],".")==0) continue;
        else if(strcmp(args[i],"..")==0){
            char before[256];
            char after[256];
            if(getcwd(before, sizeof(before))==NULL) return 0;
            if(hop_parent()==0) return 0;
            if(getcwd(after, sizeof(after))==NULL) return 0;
            if(strcmp(before, after)==0) continue;
        }
        else if(strcmp(args[i],"-")==0){
            if (hop_previous(previous_directory)==0) return 0;
        }
        else{
            char found_path[256];
            if(chdir(args[i])!=0){
                if(find_frecency(args[i], found_path)!=1){
                    return 0;
                }
                if(chdir(found_path)!=0){
                    return 0;
                }
            }
        }
        char new_path[256];
        if(getcwd(new_path, sizeof(new_path))==NULL) return 0;
        update_frecency(new_path);
        save_frecency();
        strcpy(previous_directory, current);
    }
    return 1;
}