#include "../include/main.h"

int is_builtin(char* name)
{
    if(strcmp(name, "hop") == 0)
        return 1;

    if(strcmp(name, "reveal") == 0)
        return 1;

    if(strcmp(name, "peek") == 0)
        return 1;

    if(strcmp(name, "locate") == 0)
        return 1;

    return 0;
}

int run_builtin(char* name, char** args, int argc)
{
    int result = 0;

    if(strcmp(name, "hop") == 0)
    {
        if(argc == 0)
            result = hop(NULL, 0, g_home, g_previous_directory);
        else
            result = hop(args, argc, g_home, g_previous_directory);

        if(!result)
            printf("hop : no such directory\n");
    }

    else if(strcmp(name, "reveal") == 0)
    {
        if(argc == 0)
            result = reveal(NULL, 0, g_home, g_previous_directory);
        else
            result = reveal(args, argc, g_home, g_previous_directory);

        if(!result)
            printf("reveal : no such directory\n");
    }

    else if(strcmp(name, "peek") == 0)
    {
        if(argc == 0)
            result = peek(NULL, 0);
        else
            result = peek(args, argc);
    }

    else if(strcmp(name, "locate") == 0)
    {
        if(argc == 0)
            result = locate(NULL, 0);
        else
            result = locate(args, argc);
    }

    return result;
}