#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#pragma once

// for stack
typedef struct Node{
    char        filename[100];
    struct Node *next;
}Node;

typedef struct Stack{
    Node *top;
}Stack;


//for dir system
ino_t   get_inode(char *);
void    do_ls(char []);
void    dostat(char *);
void    DFS();
int     is_dir(char *);


//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

Stack stack;

const int FALSE = 0;
const int TRUE = 1;
static int count = 0;

char *root = "/home/puzzlebook/";
char *username = "TR";



int main(int ac, char *av[]){
    if(ac != 2){
        printf("usage <app> <path>\n");
        exit(1);
    }
    else{
        Push(username, &stack);
        chdir(root);
        DFS();
    }
}

void DFS(){
    char *dirname;
    do{
        dirname = Pop(&stack);
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
            chdir("..");        // 스택 말단까지 간 후 위로 올라가는 부분
        }
        else
            break;
    }

    if(count != 0){
        chdir(dirname);         // dirname으로 디렉토리 바꾸는 부분
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
    else{           // 디렉토리 안의 내용을 말해주고 디렉토리를 스택에 넣는 부분
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




int     IsEmpty(Stack *stack){
    if(stack->top == NULL)
        return TRUE;
    else
        return FALSE;
}

char*   Pop(Stack *stack){
    Node *now;
    char *str;

    str = malloc(sizeof(char) * 50);

    if(IsEmpty(stack)){
        printf("Stack is Empty\n");
        return;
    }
    else{
        now = stack->top;
        if(strcpy(str, now->filename) == 0){
            printf("Error occur in strcpy of Stack pop\n");
            return;
        }

        stack->top = now->next;
        free(now);
        return str;
    }
}

void    Push(char filename[], Stack *stack){
    Node *temp;
    temp = (Node*)malloc(sizeof(Node));

    if( (strcpy(temp->filename, filename)) == NULL ){
        printf("Error occur in strcpy of Stack push");
        exit(1);
    }
    temp->next = stack->top;
    stack->top = temp;
}

