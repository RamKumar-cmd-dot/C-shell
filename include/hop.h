#ifndef HOP_H
#define HOP_H

typedef struct {
    char path[256];
    int frequency;
    int last_visit;
} FrecencyEntry;

int hop_home(const char* home);
int hop_current(void);
int hop_parent(void);
int hop_previous(char* previous_directory);
int hop(char** args, int count, const char* home, char* previous_directory);

#endif