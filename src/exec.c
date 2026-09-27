#include "../include/main.h"

int has_slash(char* name){
    return strchr(name, '/') != NULL;
}
char* resolve_command_path(char* name){
    if(has_slash(name)){
        if(is_executable(name)) return strdup(name);
        return NULL;
    }
    int skip = 0;
    if(name[0]=='%') skip = 1;
    char* lookup_name = skip ? name + 1 : name;
    char path[8192];
    if(!skip){
        char current[4096];
        if(getcwd(current, sizeof(current))!=NULL){
            snprintf(path, sizeof(path), "%s/%s", current, lookup_name);
            if(is_executable(path)) return strdup(path);
        }
    }
    char* path_env = getenv("PATH");
    if(path_env==NULL) return NULL;
    char* path_copy = strdup(path_env);
    if(path_copy==NULL) return NULL;
    char* directory = strtok(path_copy, ":");
    while(directory!=NULL){
        if(directory[0]=='\0') snprintf(path, sizeof(path), "./%s", lookup_name);
        else snprintf(path, sizeof(path), "%s/%s", directory, lookup_name);
        if(is_executable(path)){
            char* result = strdup(path);
            free(path_copy);
            return result;
        }
        directory = strtok(NULL, ":");  
    }
    free(path_copy);
    return NULL;
}

int execute_command(char** args, int count, char** infiles, int file_count, 
    char** outfile, int* append_flags, int outfile_count){
    if(count == 0) return 0;
    int redirect_fd = input_redirection(infiles, file_count);
    if(redirect_fd==-2) return 0;
    int out_fds[1024];
    if (outfile_count > 0) {
        int success = open_output_files(outfile, append_flags, outfile_count, out_fds);
        if (!success) {
            if (redirect_fd >= 0) {
                close(redirect_fd);
            }
            return 0;
        }
    }
    char* raw_name = args[0];
    char* display_name;
    if(!has_slash(raw_name) && raw_name[0]=='%') display_name = raw_name + 1;
    else display_name = raw_name;
    char* resolved = resolve_command_path(raw_name);
    if(resolved==NULL){
        printf("cshell: command not found (%s)\n", display_name);
        if(redirect_fd>=0) close(redirect_fd);
        if(outfile_count>=0) close_fds(out_fds, outfile_count);
        return 0;
    }
    char* exec_args[count + 1];
    exec_args[0] = resolved;
    for(int i=1;i<count;i++) exec_args[i] = args[i];
    exec_args[count] = NULL;
    int pipe_fd[2];
    if(outfile_count>0 && pipe(pipe_fd)<0){
        perror("pipe");
        free(resolved);
        if(redirect_fd>=0) close(redirect_fd);
        close_fds(out_fds, outfile_count);
        return 0;
    }
    pid_t pid = fork();
    if(pid<0){
        perror("fork");
        free(resolved);
        if(redirect_fd>=0) close(redirect_fd);
        if(outfile_count>0){
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            close_fds(out_fds, outfile_count);
        }
        return 0;
    }
    if(pid==0){
        if(redirect_fd>=0){
            dup2(redirect_fd, STDIN_FILENO);
            close(redirect_fd);
        }
        if(outfile_count>0){
            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
            close_fds(out_fds, outfile_count);
        }
        execv(resolved, exec_args);
        perror("execv");
        _exit(127);
    }
    if(redirect_fd>=0) close(redirect_fd);
    if(outfile_count>0){
        close(pipe_fd[1]);
        char buffer[4096];
        ssize_t n;
        while((n = read(pipe_fd[0], buffer, sizeof(buffer))) > 0){
            for(int i=0;i<outfile_count;i++) write(out_fds[i], buffer, n);
        }
        close(pipe_fd[0]);
        close_fds(out_fds, outfile_count);
    }
    int status;
    waitpid(pid, &status, 0);
    free(resolved);
    if(WIFEXITED(status)) return WEXITSTATUS(status);
    else return 1;
}