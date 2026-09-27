#define _POSIX_C_SOURCE 200809L
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<ctype.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<limits.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/ptrace.h>
#include<sys/user.h>
#include<time.h>
#include<errno.h>
#include "ping.h"
#include "parser.h"
#include "lexer.h"
#include "exec.h"
#include "frecency.h"
#include "hop.h"
#include "locate.h"
#include "output_redirect.h"
#include "peek.h"
#include "pipeline.h"
#include "redirect.h"
#include "reveal.h"
#include "seq_exec.h"
#include "background.h"
#include "terminal.h"
#include "resume.h"
#include "spy.h"
#include "snoop.h"
#include "builtins.h"

void format_path(char* result, char home[], char current_directory[]);
extern char g_home[256];
extern char g_previous_directory[256];