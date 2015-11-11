//Aleksandr Dobrev

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "makeargv.c"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <pwd.h>
#include <signal.h>

//Macros

#define MAX_PATH_SIZE 2048

