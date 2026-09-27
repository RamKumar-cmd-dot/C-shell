#ifndef FRECENCY_H
#define FRECENCY_h

void load_frecency(void);
void save_frecency(void);
void update_frecency(char* path);
int find_frecency(const char* name, char* result);
void init_frecency(const char* home);

#endif