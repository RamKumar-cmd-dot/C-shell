#ifndef REVEAL_H
#define REVEAL_H

int reveal(char **args, int count, char *home, char *previous_directory);
void list_recursive(char* directory, char* prefix, int show_hidden);
int list_directory(char* path, int show_hidden);
int compare_entries(const struct dirent **a, const struct dirent **b);
int is_directory(char* path);
int resolve_path(char* arg, char* home, char* previous_directory, char* result);

#endif