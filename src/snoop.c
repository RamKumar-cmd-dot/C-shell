#include "../include/main.h"
typedef struct { long num; int calls; double time; int order; } SyscallStat;
static SyscallStat stats[512];
static int stat_count = 0;
extern void report_now(char *name, int pid, int normal);
static volatile sig_atomic_t snoop_interrupted = 0;
static void snoop_sigint_handler(int sig){
    (void)sig;
    snoop_interrupted = 1;
}

static SyscallStat* find_or_add(long num){
    for(int i=0;i<stat_count;i++) if(stats[i].num == num) return &stats[i];
    stats[stat_count].num = num;
    stats[stat_count].calls = 0;
    stats[stat_count].time = 0;
    stats[stat_count].order = stat_count;
    return &stats[stat_count++];
}

static int cmp_stats(const void* a, const void* b){
    const SyscallStat* sa = a; const SyscallStat* sb = b;
    if(sb->calls != sa->calls) return sb->calls - sa->calls;
    return sa->order - sb->order;
}

static void print_stats(void){
    qsort(stats, stat_count, sizeof(SyscallStat), cmp_stats);
    printf("syscall calls time\n");
    for(int i=0;i<stat_count;i++){
        const char* name = syscall_name(stats[i].num);
        if(name) printf("%s %d %.3fs\n", name, stats[i].calls, stats[i].time);
        else printf("syscall_%ld %d %.3fs\n", stats[i].num, stats[i].calls, stats[i].time);
    }
}

static double diff_sec(struct timespec* a, struct timespec* b){
    return (b->tv_sec - a->tv_sec) + (b->tv_nsec - a->tv_nsec) / 1e9;
}

static int trace_loop(pid_t pid){
    int status;
    if(waitpid(pid, &status, 0) < 0) return 0;
    if(WIFEXITED(status)) return 1;
    if(WIFSIGNALED(status)) return 0;

    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
    struct sigaction sa, old_sa;
    sa.sa_handler = snoop_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, &old_sa);
    snoop_interrupted = 0;

    int in_syscall = 0;
    long current_num = -1;
    struct timespec entry_time;
    int final_normal = 0;

    while(!snoop_interrupted){
        if(ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) break;
        if(waitpid(pid, &status, 0) < 0) break;
        if(WIFEXITED(status)){ final_normal = 1; break; }
        if(WIFSIGNALED(status)){ final_normal = 0; break; }
        if(!WIFSTOPPED(status)) continue;

        struct user_regs_struct regs;
        if(ptrace(PTRACE_GETREGS, pid, 0, &regs) < 0) break;

        if(!in_syscall){
            current_num = regs.orig_rax;
            clock_gettime(CLOCK_MONOTONIC, &entry_time);
            in_syscall = 1;
        }
        else{
            struct timespec exit_time;
            clock_gettime(CLOCK_MONOTONIC, &exit_time);
            double elapsed = diff_sec(&entry_time, &exit_time);
            SyscallStat* s = find_or_add(current_num);
            s->calls++;
            s->time += elapsed;
            in_syscall = 0;
        }
    }

    sigaction(SIGINT, &old_sa, NULL);

    if(snoop_interrupted){
        kill(pid, SIGKILL);
        int wstatus;
        waitpid(pid, &wstatus, 0);
        return -1;
    }
    return final_normal;
}

static void notify_job_exit(pid_t pid, int normal){
    int found_index = -1;
    for(int i=0;i<256;i++){
        if(jobs[i].in_use && jobs[i].pid == pid){ found_index = i; break; }
    }
    if(found_index == -1) return;
    char name[64];
    strcpy(name, jobs[found_index].command_name);
    update_member_status(pid, 0, 0, 1);
    jobs[found_index].in_use = 0;
    report_now(name, pid, normal);
}

int snoop_command(char** args, int count){
    stat_count = 0;
    if(count < 1){
        printf("snoop: invalid syntax\n");
        return 0;
    }

    if(strcmp(args[0], "-p") == 0){
        if(count != 2){
            printf("snoop: invalid syntax\n");
            return 0;
        }
        char* end;
        long v = strtol(args[1], &end, 10);
        if(args[1][0]=='\0' || *end!='\0' || v<0){
            printf("snoop: invalid syntax\n");
            return 0;
        }
        pid_t pid = (pid_t)v;
        char procdir[64];
        snprintf(procdir, sizeof(procdir), "/proc/%d", pid);
        struct stat st;
        if(stat(procdir, &st) != 0){
            printf("snoop: no such process\n");
            return 0;
        }
        if(ptrace(PTRACE_ATTACH, pid, 0, 0) < 0){
            printf("snoop: no such process\n");
            return 0;
        }
        int result = trace_loop(pid);
        print_stats();
        if(result != -1) notify_job_exit(pid, result);
        return 1;
    }
    else{
        char* resolved = resolve_command_path(args[0]);
        if(resolved == NULL){
            printf("snoop: command not found\n");
            return 0;
        }
        pid_t pid = fork();
        if(pid < 0){
            perror("fork");
            free(resolved);
            return 0;
        }
        if(pid == 0){
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            char* exec_args[count + 1];
            for(int i=0;i<count;i++) exec_args[i] = args[i];
            exec_args[count] = NULL;
            execv(resolved, exec_args);
            perror("execv");
            _exit(127);
        }
        free(resolved);
        trace_loop(pid);
        print_stats();
        return 1;
    }
}