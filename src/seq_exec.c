#include "../include/main.h"

int execution(Token* token, int start, int end, int background){
    int count = end-start;
    if(count<=0) return 0;
    int num_commands = 1;
    for(int i=start; i<end; i++){
        if(token[i].type == OP_PIPE) num_commands++;
    }
    Command commands[num_commands];
    char* args_storage[num_commands][count];
    char* infiles_storage[num_commands][count];
    char* outfiles_storage[num_commands][count];
    int append_storage[num_commands][count];

    int cmd_idx = 0;
    int cmd_count = 0;
    int file_count = 0;
    int outfile_count = 0;
    for(int i=start; i<end; i++){
        if(token[i].type==WORD){
            args_storage[cmd_idx][cmd_count++] = token[i].value;
        }
        else if(token[i].type==OP_LT){
            i++;
            infiles_storage[cmd_idx][file_count++] = token[i].value;
        }
        else if(token[i].type==OP_GT){
            i++;
            outfiles_storage[cmd_idx][outfile_count] = token[i].value;
            append_storage[cmd_idx][outfile_count] = 0;
            outfile_count++;
        }
        else if(token[i].type==OP_GTGT){
            i++;
            outfiles_storage[cmd_idx][outfile_count] = token[i].value;
            append_storage[cmd_idx][outfile_count] = 1;
            outfile_count++;
        }
        else if(token[i].type == OP_PIPE){
            commands[cmd_idx].args = args_storage[cmd_idx];
            commands[cmd_idx].arg_count = cmd_count;
            commands[cmd_idx].infiles = infiles_storage[cmd_idx];
            commands[cmd_idx].infile_count = file_count;
            commands[cmd_idx].outfiles = outfiles_storage[cmd_idx];
            commands[cmd_idx].append_flags = append_storage[cmd_idx];
            commands[cmd_idx].outfile_count = outfile_count;
            cmd_idx++;
            cmd_count = 0;
            file_count = 0;
            outfile_count = 0;
        }
    }
    commands[cmd_idx].args = args_storage[cmd_idx];
    commands[cmd_idx].arg_count = cmd_count;
    commands[cmd_idx].infiles = infiles_storage[cmd_idx];
    commands[cmd_idx].infile_count = file_count;
    commands[cmd_idx].outfile_count = outfile_count;
    commands[cmd_idx].outfiles = outfiles_storage[cmd_idx];
    commands[cmd_idx].append_flags = append_storage[cmd_idx];
    if(background) return execute_pipeline_bg(commands, num_commands);
    return execute_pipeline(commands, num_commands);
}

int sequential_execution(Token* token, int start, int end){
    int i = start;
    while(i<end){
        int seg_start = i;
        while(i<end && token[i].type!=OP_SEMI && token[i].type != OP_AMP) i++;
        int seg_end = i;
        int is_bg = 0;
        if(i<end && token[i].type==OP_AMP) is_bg = 1;
        int result = execution(token, seg_start, seg_end, is_bg);
        if(!is_bg && result == -1) break;
        if(i<end) i++;
    }
    return 1;
}