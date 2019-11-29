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



static int count = 0;
Stack stack;

void DFS(){
    char *dirname;
	Push(username, &stack);
        chdir(root);
    do{
        dirname = Pop(&stack);
	printf("Pop :: %s\n", dirname);
        count++;
        do_ls(dirname);
        printf("\n=========================\n");
    }while( !(IsEmpty(&stack)) );
}

int is_dir(char *filename){
    int mode;
    struct stat info;
    if(stat(filename, &info) == -1)
        perror(filename);
    mode = info.st_mode;

    if(S_ISDIR(mode)){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

void do_ls(char dirname[]){
    DIR             *dir_ptr;
    struct  dirent  *direntp;
    char            *backUp;

    while(count != 0){
        if( (dir_ptr = opendir(dirname)) == NULL ){
            count --;
            chdir("..");
        }
        else
            break;
    }

    if(count != 0){
        chdir(dirname);
        while( (direntp = readdir(dir_ptr)) != NULL ){
            if( ( (strcmp(direntp->d_name, ".")) && (strcmp(direntp->d_name, ".."))  ) == 0)
                continue;
            else
                dostat(direntp->d_name);
        }
        closedir(dir_ptr);
    }
    else
        return;
}

void dostat(char *filename){
    struct stat info;
    char* test;

    if(stat(filename, &info) == -1){
        printf("Do not stat\n");
    }
    else{
        printf("file name :: %s, file size ::%d\n",filename, info.st_size);
        if(is_dir(filename))
            Push(filename, &stack);
    }
}

ino_t get_inode(char *filename){
    struct stat info;
    if(stat(filename, &info) == -1){
        fprintf(stderr, "Cannot stat");
        perror(filename);
        exit(1);
    }
    return info.st_ino;
}

