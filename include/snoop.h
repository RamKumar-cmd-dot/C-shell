#ifndef SNOOP_H
#define SNOOP_H

int snoop_command(char** args, int count);

typedef struct {
    long num;
    const char* name;
} SyscallEntry;

static const SyscallEntry syscall_table[] = {
    {0,"read"},{1,"write"},{2,"open"},{3,"close"},{4,"stat"},{5,"fstat"},
    {8,"lseek"},{9,"mmap"},{10,"mprotect"},{11,"munmap"},{12,"brk"},
    {21,"access"},{22,"pipe"},{35,"nanosleep"},{39,"getpid"},
    {57,"fork"},{58,"vfork"},{59,"execve"},{60,"exit"},{61,"wait4"},
    {62,"kill"},{63,"uname"},{89,"readlink"},{158,"arch_prctl"},
    {201,"time"},{218,"set_tid_address"},{230,"clock_nanosleep"},
    {231,"exit_group"},{257,"openat"},{262,"newfstatat"},
    {273,"set_robust_list"},{302,"prlimit64"},{318,"getrandom"},{334,"rseq"}
};

static const int syscall_table_count = sizeof(syscall_table)/sizeof(syscall_table[0]);

static inline const char* syscall_name(long num){
    for(int i=0;i<syscall_table_count;i++)
        if(syscall_table[i].num == num) return syscall_table[i].name;
    return NULL;
}

#endif