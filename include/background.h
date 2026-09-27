#ifndef BACKGROUND_H
#define BACKGROUND_H

void init_jobs(void);
void install_sigchld_handler(void);
int register_job(pid_t pid, char* command_name);
void flush_pending_job_reports(void);
void register_job_members(int job_number, pid_t pgid, char** names, pid_t* pids, int count);
void update_member_status(pid_t pid, int stopped, int continued, int exited);
void print_activities(void);
extern volatile sig_atomic_t g_foreground_active;
extern char current_prompt[1024];
void set_current_prompt(char* prompt);

#define MAX_MEMBERS 16
typedef enum{
    PROC_RUNNING, PROC_STOPPED
}ProcState;
typedef struct{
    pid_t pid;
    char name[64];
    ProcState state;
    int exited;
}JobMember;
typedef struct{
    int job_number;
    pid_t pgid;
    JobMember members[MAX_MEMBERS];
    int member_count;
    int active_count;
    char command_name[64];
    pid_t pid;
    int in_use;
} Job;
typedef struct{
    pid_t pid;
    char command_name[64];
    int normal;
} PendingReport;
extern Job jobs[256];
#endif