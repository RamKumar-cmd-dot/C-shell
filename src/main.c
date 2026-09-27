#include "../include/main.h"
#include "../include/terminal.h"
#include "../include/builtins.h"

char g_home[256];
char g_previous_directory[256] = "";

void format_path(char* result, char home[], char current_directory[]){
    int n = strlen(home);
    if(strcmp(home, current_directory)==0) strcpy(result, "~");
    else if(strncmp(current_directory, home, n)==0 && current_directory[n]=='/'){
        sprintf(result, "~%s", current_directory+n);
    }
    else strcpy(result, current_directory);
}

int main(){
    char* username = getenv("USER");
    char hostname[256];
    char current_directory[256];
    char path[256];

    getcwd(g_home, sizeof(g_home));
    init_frecency(g_home);
    load_frecency();
    gethostname(hostname, sizeof(hostname));
    init_jobs();
    install_sigchld_handler();
    install_shell_signal_handlers();

    while(1){
        clearerr(stdin);
        getcwd(current_directory, sizeof(current_directory));
        format_path(path, g_home, current_directory);
        char prompt_buf[4096];
        snprintf(prompt_buf, sizeof(prompt_buf), "<%s@%s:%s> ", username, hostname, path);
        set_current_prompt(prompt_buf);
        printf("%s", prompt_buf);
        fflush(stdout);

        char input[1024];
        if(fgets(input, sizeof(input), stdin)==NULL){
            if(errno==EINTR){
                printf("\n");
                continue;
            }
            printf("\n");
            break;
        }

        int count;
        Token* token = lexer(input, &count);
        if(token==NULL) continue;
        if(count == 0) continue;
        if(!parse(token, count)){
            printf("Shell : inavlid syntax\n");
        }
        else if(is_builtin(token[0].value)){
            int simple = 1;
            for(int i=0;i<count;i++){
                if(token[i].type != WORD){ simple = 0; break; }
            }
            if(simple){
                char* args[count-1];
                for(int i=1;i<count;i++) args[i-1] = token[i].value;
                run_builtin(token[0].value, args, count-1);
            }
            else{
                sequential_execution(token, 0, count);
            }
        }
        else if(strcmp(token[0].value, "activities")==0){
            print_activities();
        }
        else if(strcmp(token[0].value, "resume")==0){
            char* args[count-1];
            for(int i=1;i<count;i++) args[i-1] = token[i].value;
            if(!resume_command(args, count-1)) printf("resume: invalid syntax\n");
        }
        else if(strcmp(token[0].value, "ping")==0){
            int ok = 1;
            for(int i=1;i<count;i++) if(token[i].type != WORD){ ok = 0; break; }
            if(!ok) printf("ping: invalid syntax\n");
            else{
                char* args[count-1];
                for(int i=1;i<count;i++) args[i-1] = token[i].value;
                ping_command(args, count-1);
            }
        }
        else if(strcmp(token[0].value, "spy")==0){
            int ok = 1;
            for(int i=1;i<count;i++) if(token[i].type != WORD){ ok = 0; break; }
            if(!ok) printf("spy: invalid syntax\n");
            else{
                char* args[count-1];
                for(int i=1;i<count;i++) args[i-1] = token[i].value;
                spy_command(args, count-1);
            }
        }
        else if(strcmp(token[0].value, "snoop")==0){
            int ok = 1;
            for(int i=1;i<count;i++) if(token[i].type != WORD){ ok = 0; break; }
            if(!ok) printf("snoop: invalid syntax\n");
            else{
                char* args[count-1];
                for(int i=1;i<count;i++) args[i-1] = token[i].value;
                snoop_command(args, count-1);
            }
        }
        else{
            sequential_execution(token, 0, count);
        }
    }
    return 0;
}