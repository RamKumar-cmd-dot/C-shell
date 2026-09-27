#ifndef PIPELINE_H
#define PIPELINE_H

typedef struct {
    char** args;
    int arg_count;
    char** infiles;
    int infile_count;
    char** outfiles;
    int* append_flags;
    int outfile_count;
} Command;

int execute_pipeline(Command* commands, int command_number);
int execute_pipeline_bg(Command* commands, int command_number);
extern pid_t pgid;
#endif