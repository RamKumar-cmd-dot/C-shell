#ifndef LOCATE_H
#define LOCATE_H

int locate(char **args, int count);
int is_executable(char* path);
int search_directory(char* directory, char* filename);
void locate_one(char* filename);

#endif