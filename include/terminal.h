#ifndef TERMINAL_H
#define TERMINAL_H
#include "main.h"

void install_shell_signal_handlers(void);
void print_stopped_job(int job_number, char* command_name);
void hangup_all_jobs(void);
int any_stopped_jobs(void);
void give_terminal_to(pid_t pgid);
void reclaim_terminal(void);

#endif 