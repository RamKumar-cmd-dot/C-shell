#ifndef OUTPUT_REDIRECT_H
#define OUTPUT_REDIRECT_H

int open_output_files(char** outfiles, int* append_flags, int file_count, int* out_fds);
void close_fds(int* fds, int count);

#endif