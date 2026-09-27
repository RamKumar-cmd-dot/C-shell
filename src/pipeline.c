#include "../include/main.h"
#include "../include/builtins.h"

int execute_pipeline_internal(Command* commands, int command_number, int background){
    if(command_number <= 0) return 0;
    int num_pipes = command_number - 1;
    int pipe_fds[num_pipes>0?num_pipes:1][2];
    for(int i=0;i<num_pipes;i++){
        if(pipe(pipe_fds[i])<0){
            perror("pipe");
            for(int j=0;j<i;j++){
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            return 0;
        }
    }
    int use_tee = 0;
    if(commands[command_number-1].outfile_count>1) use_tee = 1;
    int tee_fd[2] = {-1, -1};
    if(use_tee && pipe(tee_fd) < 0){
        perror("pipe");
        for(int j=0;j<num_pipes;j++){
            close(pipe_fds[j][0]);
            close(pipe_fds[j][1]);
        }
        return 0;
    }
    int sync_fd[2] = {-1, -1};
    if(background){
        if(pipe(sync_fd) < 0){
            perror("pipe");
            for(int j=0;j<num_pipes;j++){
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            return 0;
        }
    }

    sigset_t block, prev;
    if(background){
        sigemptyset(&block);
        sigaddset(&block, SIGCHLD);
        sigprocmask(SIG_BLOCK, &block, &prev);
    }
    pid_t pids[command_number];
    pid_t pgid = 0;
    for(int i=0;i<command_number;i++){
        pid_t pid = fork();
        if(pid<0){
            perror("fork");
            pids[i] = -1;
            continue;
        }
        if(pid==0){
            // NEW: block here until the parent releases us (closes sync_fd[1])
            if(background){
                close(sync_fd[1]);
                char gatebuf;
                read(sync_fd[0], &gatebuf, 1);
                close(sync_fd[0]);
            }
            setpgid(0, pgid);
            if(background){
                sigset_t childset;
                sigemptyset(&childset);
                sigaddset(&childset, SIGCHLD);
                sigprocmask(SIG_UNBLOCK, &childset, NULL);
            }
            if(i>0) dup2(pipe_fds[i-1][0], STDIN_FILENO);
            else if(commands[i].infile_count > 0){
                int redirect_fd = input_redirection(commands[i].infiles, commands[i].infile_count);
                if(redirect_fd == -2) _exit(1);
                dup2(redirect_fd, STDIN_FILENO);
                close(redirect_fd);
            }
            if(i<command_number-1) dup2(pipe_fds[i][1], STDOUT_FILENO);
            else if(use_tee) dup2(tee_fd[1], STDOUT_FILENO);
            else if(commands[i].outfile_count == 1){
                int out_fd;
                if(!open_output_files(commands[i].outfiles, commands[i].append_flags, 1, &out_fd)) _exit(1);
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            for(int j=0;j<num_pipes;j++){
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            if(use_tee){
                close(tee_fd[0]);
                close(tee_fd[1]);
            }
            char* raw_name = commands[i].args[0];
            char* display_name = raw_name;
            if(!has_slash(raw_name) && raw_name[0]=='%') display_name = raw_name+1;

            if(is_builtin(raw_name)){
                int argc = commands[i].arg_count - 1;
                char** bargs = (argc > 0) ? &commands[i].args[1] : NULL;
                int r = run_builtin(raw_name, bargs, argc);
                _exit(r ? 0 : 1);
            }

            char* resolved = resolve_command_path(raw_name);
            if(resolved == NULL){
                printf("cshell: command not found (%s)\n", display_name);
                _exit(127);
            }
            char* exec_args[commands[i].arg_count + 1];
            for(int k=0;k<commands[i].arg_count;k++) exec_args[k] = commands[i].args[k];
            exec_args[commands[i].arg_count] = NULL;
            execv(resolved, exec_args);
            perror("execv");
            _exit(127);
        }
        pids[i] = pid;
        if(pid>0){
            if(i == 0) pgid = pid;
            setpgid(pid, pgid);
        }
    }
    for(int i=0;i<num_pipes;i++){
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    if(!background) give_terminal_to(pgid);
    if(use_tee){
        if(background){
            pid_t tee_pid = fork();
            if(tee_pid == 0){
                close(tee_fd[1]);
                int outfile_count = commands[command_number-1].outfile_count;
                int out_fds[outfile_count];
                if(open_output_files(commands[command_number-1].outfiles,
                    commands[command_number-1].append_flags, outfile_count, out_fds)){
                    char buffer[4096];
                    ssize_t n;
                    while((n = read(tee_fd[0], buffer, sizeof(buffer)))>0)
                        for(int i=0;i<outfile_count;i++) write(out_fds[i], buffer, n);
                    close_fds(out_fds, outfile_count);
                }
                close(tee_fd[0]);
                _exit(0);
            }
            close(tee_fd[0]);
            close(tee_fd[1]);
        }
        else{
            close(tee_fd[1]);
            int outfile_count = commands[command_number-1].outfile_count;
            int out_fds[outfile_count];
            if(open_output_files(commands[command_number-1].outfiles,commands[command_number-1].append_flags,
                outfile_count, out_fds)){
                char buffer[4096];
                ssize_t n;
                while((n = read(tee_fd[0], buffer, sizeof(buffer)))>0)
                    for(int i=0;i<outfile_count;i++) write(out_fds[i], buffer, n);
                close_fds(out_fds, outfile_count);
            }
            close(tee_fd[0]);
        }
    }
    if(background){
        char full_cmd[256] = "";
        for(int k=0;k<commands[0].arg_count;k++){
            if(k>0) strcat(full_cmd, " ");
            strcat(full_cmd, commands[0].args[k]);
        }
        int job_number = register_job(pids[0], full_cmd);
        char* names[command_number];
        for(int k=0;k<command_number;k++) names[k] = commands[k].args[0];
        register_job_members(job_number, pgid, names, pids, command_number);
        printf("[%d] %d\n", job_number, pids[0]);
        fflush(stdout);

        // NEW: release every gated child now that the job line has printed
        close(sync_fd[0]);
        close(sync_fd[1]);

        sigprocmask(SIG_SETMASK, &prev, NULL);
        return pids[0];
    }
    sigset_t fgblock, fgprev;
    sigemptyset(&fgblock);
    sigaddset(&fgblock, SIGCHLD);
    sigprocmask(SIG_BLOCK, &fgblock, &fgprev);
    int status = 0;
    int failed = 0;
    int stopped_flag = 0;
    int sigint_flag = 0;
    for(int i=0;i<command_number;i++){
        if(pids[i]>0){
            waitpid(pids[i], &status, WUNTRACED);
            if(WIFSTOPPED(status)) stopped_flag = 1;
            else if(WIFSIGNALED(status) && WTERMSIG(status)==SIGINT) sigint_flag = 1;
            else if(WIFEXITED(status) && WEXITSTATUS(status)==127) failed = 1;
        }
    }
    reclaim_terminal();
    sigprocmask(SIG_SETMASK, &fgprev, NULL);
    if(sigint_flag && !stopped_flag) printf("\n");
    if(stopped_flag){
        char full_cmd[256] = "";
        for(int k=0;k<commands[0].arg_count;k++){
            if(k>0) strcat(full_cmd, " ");
            strcat(full_cmd, commands[0].args[k]);
        }
        int job_number = register_job(pids[0], full_cmd);
        char* names[command_number];
        for(int k=0;k<command_number;k++) names[k] = commands[k].args[0];
        register_job_members(job_number, pgid, names, pids, command_number);
        for(int k=0;k<command_number;k++){
            if(pids[k]>0) update_member_status(pids[k], 1, 0, 0);
        }
        print_stopped_job(job_number, full_cmd);
        return -2;
    }
    return failed?-1:1;
}
int execute_pipeline(Command* commands, int command_number){
    sigset_t block, prev;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, &prev);
    g_foreground_active = 1;
    sigprocmask(SIG_SETMASK, &prev, NULL);
    int result = execute_pipeline_internal(commands, command_number, 0);
    sigprocmask(SIG_BLOCK, &block, &prev);
    g_foreground_active = 0;
    sigprocmask(SIG_SETMASK, &prev, NULL);
    flush_pending_job_reports();
    return result;
}
int execute_pipeline_bg(Command* commands, int command_number){
    return execute_pipeline_internal(commands, command_number, 1);
}