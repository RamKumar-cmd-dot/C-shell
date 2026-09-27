#ifndef BUILTINS_H
#define BUILTINS_H

int is_builtin(char* name);
int run_builtin(char* name, char** args, int argc);

#endif