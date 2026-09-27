#include "../include/main.h"
int is_valid_nonneg_int(char* s, long* out){
    if(*s=='\0') return 0;
    for(char* p = s; *p; p++){
        if(!isdigit((unsigned char)*p)) return 0;
    }
    char* end;
    long v = strtol(s, &end, 10);
    if(*end != '\0') return 0;
    *out = v;
    return 1;
}
int ping_command(char** args, int count){
    if(count != 2){
        printf("ping: invalid syntax\n");
        return 0;
    }
    char* target = args[0];
    char* signal_arg = args[1];
    long sig_num;
    if(!is_valid_nonneg_int(signal_arg, &sig_num)){
        printf("ping: invalid syntax\n");
        return 0;
    }
    int actual_signal = (int)(sig_num % 64);
    if(target[0] == '%'){
        long job_num;
        if(!is_valid_nonneg_int(target + 1, &job_num)){
            printf("ping: no such process found\n");
            return 0;
        }
        int idx = -1;
        for(int i=0;i<256;i++){
            if(jobs[i].in_use && jobs[i].job_number == (int)job_num){
                idx = i;
                break;
            }
        }
        if(idx==-1){
            printf("ping: no such process found\n");
            return 0;
        }
        if(kill(-jobs[idx].pgid, actual_signal) < 0){
            printf("ping: no such process found\n");
            return 0;
        }
        printf("Sent signal %ld to %s\n", sig_num, target);
        return 1;
    }
    else{
        long pid_num;
        if(!is_valid_nonneg_int(target, &pid_num)){
            printf("ping: no such process found\n");
            return 0;
        }
        pid_t target_pid = (pid_t)pid_num;
        int found = 0;
        for(int i=0;i<256 && !found;i++){
            if(!jobs[i].in_use) continue;
            for(int m=0;m<jobs[i].member_count;m++){
                if(!jobs[i].members[m].exited && jobs[i].members[m].pid == target_pid){
                    found = 1;
                    break;
                }
            }
        }
        if(!found){
            printf("ping: no such process found\n");
            return 0;
        }
        if(kill(target_pid, actual_signal) < 0){
            printf("ping: no such process found\n");
            return 0;
        }
        printf("Sent signal %ld to %s\n", sig_num, target);
        return 1;
    }
}