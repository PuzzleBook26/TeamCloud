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

Stack stack;



int main(int ac, char *av[]){
    if(ac != 2){
        printf("usage <app> <path>\n");
        exit(1);
    }
    else{
        
        DFS();
    }
}

