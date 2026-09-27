#ifndef EXEC_H
#define EXEC_H

char* resolve_command_path(char* name);
int execute_command(char** args, int count, char** infiles, int file_count, char** outfile, 
    int* append_flags, int outfile_count);
int has_slash(char* name);

#endif