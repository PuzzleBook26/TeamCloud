#pragma once
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tourDir.h"
#include "stack.h"
#include "serveroption.h"
//for dir system
ino_t   get_inode(char *);
void    do_ls(char []);
void    dostat(char *);
void    DFS();
int     is_dir(char *);



