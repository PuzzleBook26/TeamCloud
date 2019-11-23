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

// for stack
typedef struct Node{
    char        filename[100];
    struct Node *next;
}Node;

typedef struct Stack{
    Node *top;
}Stack;

//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

