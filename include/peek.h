#ifndef PEEK_H
#define PEEK_H

int peek(char** args, int count);
int open_file(char* filename);
void print_stream(void);
int number_stdin(void);
void reverse_stdin_helper(void);
void reverse_number_stdin_helper(int *line_number);
int reverse_number_stdin(void);
int reverse_stdin(void);
int print_file(char* filename);
void reverse_helper(FILE* file);
int count_nonempty_lines(int fd);
void print_segment(const char *data, long len, int numbering, int *current_number);
int reverse_seek_output(int fd, int numbering);
int print_reverse(char* filename);
void reverse_numbering_helper(FILE* file, int* line_number);
int reverse_numbering(char* filename);
int number_line(char* filename);

#endif