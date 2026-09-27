#include "../include/main.h"

volatile sig_atomic_t resume_timed_out = 0;
void resume_alarm_handler(int sig){
    (void)sig;
    resume_timed_out = 1;
}
int find_resume_job(int job_number){
    for(int i=0;i<256;i++){
        if(jobs[i].in_use && jobs[i].job_number==job_number) return i;
    }
    return -1;
}
void mark_job_running(Job* job){
    for(int i = 0; i < job->member_count; i++){
        if(!job->members[i].exited) job->members[i].state = PROC_RUNNING;
    }
}
void mark_process_exited(Job* job, pid_t pid){
    for(int i=0;i<job->member_count;i++){
        if(job->members[i].pid==pid && !job->members[i].exited){
            job->members[i].exited = 1;
            job->active_count--;
            return;
        }
    }
}
int resume_bg(Job* job){
    if(kill(-job->pgid, SIGCONT)<0){
        perror("resume");
        return 0;
    }
    mark_job_running(job);
    printf("[%d] + Running    %s\n", job->job_number, job->command_name);
    return 1;
}
int resume_fg(Job* job, int timeout){
    sigset_t block, previous;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &previous);
    mark_job_running(job);
    give_terminal_to(job->pgid);
    if(kill(-job->pgid, SIGCONT)<0){
        perror("resume");
        reclaim_terminal();
        sigprocmask(SIG_SETMASK, &previous, NULL);
        return 0;
    }
    printf("%s\n", job->command_name);
    fflush(stdout);
    struct sigaction old_alarm;
    struct sigaction alarm_action;
    memset(&alarm_action, 0, sizeof(alarm_action));
    alarm_action.sa_handler = resume_alarm_handler;
    sigemptyset(&alarm_action.sa_mask);
    alarm_action.sa_flags = 0;
    sigaction(SIGALRM, &alarm_action, &old_alarm);
    resume_timed_out = 0;
    if(timeout>0) alarm((unsigned int)timeout);
    int remaining = job->active_count;
    int stopped = 0;
    while(remaining>0){
        int status;
        pid_t pid = waitpid(-job->pgid, &status, WUNTRACED);
        if(pid<0){
            if(errno==EINTR){
                if(resume_timed_out) break;
                continue;
            }
            if(errno==ECHILD) break;
            perror("waitpid");
            break;
        }
        if(WIFSTOPPED(status)){
            stopped = 1;
            for(int i=0;i<job->member_count;i++){
                if(job->members[i].pid==pid) job->members[i].state = PROC_STOPPED;
            }
            continue;
        }
        if(WIFEXITED(status) || WIFSIGNALED(status)){
            mark_process_exited(job, pid);
            remaining--;
        }
    }
    if(resume_timed_out && remaining > 0){
        alarm(0);
        kill(-job->pgid, SIGTERM);
        printf("resume: job timed out\n");
        fflush(stdout);
        while(remaining>0){
            int status;
            pid_t pid = waitpid(-job->pgid, &status, 0);
            if(pid<0){
                if(errno==EINTR) continue;
                if(errno == ECHILD) break;
                break;
            }
            if(WIFEXITED(status) || WIFSIGNALED(status)){
                mark_process_exited(job, pid);
                remaining--;
            }
        }
        job->in_use = 0;
    }
    else if(stopped) printf("\n[%d] + Stopped    %s\n", job->job_number, job->command_name);
    else if(remaining<=0) job->in_use = 0;
    alarm(0);
    sigaction(SIGALRM, &old_alarm, NULL);
    reclaim_terminal();
    sigprocmask(SIG_SETMASK, &previous, NULL);
    return 1;
}
int resume_command(char **args, int count){
    if(count<1) return 0;
    if(args[0][0] != '%') return 0;
    char *end;
    long number = strtol(args[0] + 1, &end, 10);
    if(*(args[0] + 1)=='\0' || *end!='\0' || number<=0) return 0;
    int job_number = (int)number;
    int mode_fg = 1;
    int timeout = 0;
    for(int i=1;i<count;i++){
        if(strcmp(args[i],"fg")==0) mode_fg = 1;
        else if(strcmp(args[i], "bg")==0) mode_fg = 0;
        else if(strcmp(args[i], "--timeout") == 0){
            if(!mode_fg || i + 1>=count) return 0;
            char *end_timeout;
            long t = strtol(args[i + 1], &end_timeout, 10);
            if(*end_timeout != '\0' || t <= 0) return 0;
            timeout = (int)t;
            i++;
        }
        else return 0;
    }
    int index = find_resume_job(job_number);
    if(index == -1){
        printf("resume: no such job\n");
        return 0;
    }
    Job *job = &jobs[index];
    if(mode_fg) return resume_fg(job, timeout);
    return resume_bg(job);
}