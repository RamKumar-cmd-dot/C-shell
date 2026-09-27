#include "../include/main.h"

FrecencyEntry frecency[1000];
int frecency_count = 0;
int visit_counter = 0;
char frecency_file[256];

void init_frecency(const char* start_directory){
    snprintf(frecency_file, sizeof(frecency_file), "%s/.hop_history.log", start_directory);
}

void load_frecency(void){
    FILE* file = fopen(frecency_file,"r");
    if(file==NULL) return ;
    char line[256];
    while(fgets(line, sizeof(line), file)!=NULL){
        char* path = strtok(line, "|");
        char* freq = strtok(NULL, "|");
        char* last = strtok(NULL, "|\n");
        if((path==NULL) | (freq==NULL) | (last==NULL)) continue;
        strcpy(frecency[frecency_count].path, path);
        frecency[frecency_count].frequency = atoi(freq);
        frecency[frecency_count].last_visit = atoi(last);
        if(frecency[frecency_count].last_visit>visit_counter){
            visit_counter = frecency[frecency_count].last_visit;
        }
        frecency_count ++ ;
        if(frecency_count>1000) break;
    }
    fclose(file);
}

void update_frecency(char* path){
    visit_counter++;
    for(int i=0;i<frecency_count;i++){
        if(strcmp(frecency[i].path, path)==0){
            frecency[i].frequency++;
            frecency[i].last_visit = visit_counter;
            return ;
        }
    }
    if(frecency_count>1000) return;

    strcpy(frecency[frecency_count].path, path);
    frecency[frecency_count].frequency = 1;
    frecency[frecency_count].last_visit = visit_counter;

    frecency_count++;
}

void save_frecency(void){
    FILE* file = fopen(frecency_file, "w");
    if(file==NULL) return ;
    for(int i=0;i<frecency_count;i++){
        fprintf(file, "%s|%d|%d\n", frecency[i].path, frecency[i].frequency, frecency[i].last_visit);
    }
    fclose(file);
}

int find_frecency(const char* name, char* result){
    int best_score = -1;
    int best_idx = -1;
    for(int i=0;i<frecency_count;i++){
        if(strstr(frecency[i].path, name)==NULL) continue;;
        if(access(frecency[i].path, F_OK)!=0) continue;
        int score = frecency[i].frequency*100 + frecency[i].last_visit;
        if(best_score<score){
            best_idx = i;
            best_score = score;
        }
    }
    if(best_idx==-1) return 0;
    strcpy(result, frecency[best_idx].path);
    return 1;
}
