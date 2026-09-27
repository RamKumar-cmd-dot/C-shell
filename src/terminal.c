#include "../include/main.h"
void sigint_handler(int sig){
    (void)sig;
}
void sigtstp_handler(int sig){
    (void)sig;
}
void install_shell_signal_handlers(){
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* deliberately no SA_RESTART: a blocked fgets() at the
                         idle prompt must return (EINTR) on Ctrl-C so the
                         shell can redraw the prompt immediately instead of
                         waiting for Enter */
    sigaction(SIGINT, &sa, NULL);

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);

    signal(SIGTTOU, SIG_IGN);
}
void print_stopped_job(int job_number, char* command_name){
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "\n[%d] + Stopped    %s\n", job_number, command_name);
    if(len > 0) write(STDOUT_FILENO, buf, (size_t)len);
}
void hangup_all_jobs(){
    for(int i=0;i<256;i++){
        if(jobs[i].in_use && jobs[i].active_count>0) kill(-jobs[i].pgid, SIGHUP);
    }
}
int any_stopped_jobs(void){
    for(int i=0;i<256;i++){
        if(!jobs[i].in_use || jobs[i].active_count <= 0) continue;
        for(int m=0;m<jobs[i].member_count;m++){
            if(!jobs[i].members[m].exited && jobs[i].members[m].state == PROC_STOPPED) return 1;
        }
    }
    return 0;
}
void give_terminal_to(pid_t pgid){
    if(tcsetpgrp(STDIN_FILENO, pgid) < 0) perror("tcsetpgrp give");
}
void reclaim_terminal(void){
    if(tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) perror("tcsetpgrp reclaim");
}