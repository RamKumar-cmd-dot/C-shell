#include "../include/main.h"

Job jobs[256];
int next_job = 1;
PendingReport pending[256];
sig_atomic_t pending_count = 0;
volatile sig_atomic_t g_foreground_active = 0;
char current_prompt[1024] = "";
void init_jobs(void){
    for(int i=0;i<256;i++) jobs[i].in_use = 0;
    next_job = 1;
}
int register_job(pid_t pid, char* command_name){
    int slot = -1;
    for(int i=0;i<256;i++){
        if(!jobs[i].in_use){ 
            slot = i;
            break;
        }
    }
    if(slot==-1) return -1;
    jobs[slot].pid = pid;
    jobs[slot].job_number = next_job++;
    strncpy(jobs[slot].command_name, command_name, sizeof(jobs[slot].command_name)-1);
    jobs[slot].command_name[sizeof(jobs[slot].command_name)-1] = '\0';
    jobs[slot].in_use = 1;

    jobs[slot].pgid = pid;
    jobs[slot].member_count = 1;
    jobs[slot].active_count = 1;
    jobs[slot].members[0].pid = pid;
    jobs[slot].members[0].state = PROC_RUNNING;
    jobs[slot].members[0].exited = 0;
    strncpy(jobs[slot].members[0].name, command_name, sizeof(jobs[slot].members[0].name)-1);
    jobs[slot].members[0].name[sizeof(jobs[slot].members[0].name)-1] = '\0';
    return jobs[slot].job_number;
}
void register_job_members(int job_number, pid_t pgid, char** names, pid_t* pids, int count){
    for(int i=0;i<256;i++){
        if(jobs[i].in_use && jobs[i].job_number==job_number){
            jobs[i].pgid = pgid;
            jobs[i].member_count = count;
            jobs[i].active_count = count;
            for(int k=0;k<count && k<MAX_MEMBERS;k++){
                jobs[i].members[k].pid = pids[k];
                jobs[i].members[k].state = PROC_RUNNING;
                jobs[i].members[k].exited = 0;
                strncpy(jobs[i].members[k].name, names[k], sizeof(jobs[i].members[k].name)-1);
                jobs[i].members[k].name[sizeof(jobs[i].members[k].name)-1] = '\0';
            }
            return ;
        }
    }
}
void update_member_status(pid_t pid, int stopped, int continued, int exited){
    for(int i=0;i<256;i++){
        if(!jobs[i].in_use) continue;
        for(int m=0;m<jobs[i].member_count;m++){
            if(jobs[i].members[m].pid!=pid) continue;
            if(stopped) jobs[i].members[m].state = PROC_STOPPED;
            else if(continued) jobs[i].members[m].state = PROC_RUNNING;
            else if(exited){
                jobs[i].members[m].exited = 1;
                jobs[i].active_count--;
            }
            return;
        }
    }
}
void print_activities(void){
    for(int i=0;i<256;i++){
        if(!jobs[i].in_use) continue;
        if(jobs[i].active_count<=0) continue;
        printf("[%d] pgid %d\n", jobs[i].job_number, jobs[i].pgid);
        for(int m=0;m<jobs[i].member_count;m++){
            if(jobs[i].members[m].exited) continue;
            if (jobs[i].members[m].state == PROC_RUNNING){
                printf("  %d %s Running\n", jobs[i].members[m].pid, jobs[i].members[m].name);
            }
            else{
                printf("  %d %s Stopped\n", jobs[i].members[m].pid, jobs[i].members[m].name);
            }
        }
    }
}
void set_current_prompt(char* prompt){
    strncpy(current_prompt, prompt, sizeof(current_prompt)-1);
    current_prompt[sizeof(current_prompt)-1] = '\0';
}
void report_now(char* name, pid_t pid, int normal){
    char buf[256];
    int len;
    if(normal) len = snprintf(buf, sizeof(buf), "\n%s with pid %d exited normally\n%s", name, pid, current_prompt);
    else len = snprintf(buf, sizeof(buf), "\n%s with pid %d exited abnormally\n%s", name, pid, current_prompt);
    if(len>0) write(STDOUT_FILENO, buf, (size_t)len);
}
void sigchld_handler(int sig){
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG| WUNTRACED | WCONTINUED)) > 0){
        if(WIFSTOPPED(status)){ 
            update_member_status(pid, 1, 0, 0);
            continue;
        }
        if(WIFCONTINUED(status)){
            update_member_status(pid, 0, 1, 0);
            continue;
        }
        int normal = WIFEXITED(status) ? 1 : 0;
        int job_index = -1;
        for(int i=0;i<256;i++){
            if (jobs[i].in_use && jobs[i].pid==pid){
                job_index = i;
                break;
            }
        }
        if(job_index == -1) continue;
        char name[64];
        update_member_status(pid, 0, 0, 1);
        strcpy(name, jobs[job_index].command_name);
        jobs[job_index].in_use = 0;
        if(g_foreground_active){
            if(pending_count<256){
                strcpy(pending[pending_count].command_name, name);
                pending[pending_count].pid = pid;
                pending[pending_count].normal = normal;
                pending_count++;
            }
        }
        else {
            report_now(name, pid, normal);
        }
    }
    errno = saved_errno;
}
void flush_pending_job_reports(void){
    while(pending_count > 0){
        pending_count--;
        report_now(pending[pending_count].command_name, pending[pending_count].pid, pending[pending_count].normal);
    }
}
void install_sigchld_handler(void){
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}