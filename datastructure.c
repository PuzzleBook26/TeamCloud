#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

typedef struct Node{
    char    filename[100];
    struct Node *next;

}Node;

typedef struct List{
    Node *head;

}List;
typedef struct Stack{
    Node *top;
}Stack;

//for list
int IsListEmpty(List *);
void FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);


//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

const int FALSE = 0;
const int TRUE = 1;



// implementation of list
int IsListEmpty(List *list){
    if(list->head == NULL)
        return TRUE;
    else
        return FALSE;
}

void FindFile(List *list, char *filename){
    Node *prev;
    Node *now;
    
    if(IsListEmpty(list)){
        printf("List is Empty in find file\n");
        return;
    }

    prev = list->head;
    now = list->head;

    while(now){
        if( (strcmp(now->filename, filename)) == 0 ){
            printf("Find File :: %s\n", now->filename);
            
            if(now == list->head){
               list->head = now->next; 
            }
            prev->next = now->next;
            free(now);
            now = prev->next;
            
            return;
        }
        else{
            prev = now;
            now = now->next;
        }
    }
    printf("Not Find %s :(\n",filename);
}

void PrintList(List *list){
    Node *now;
    now = list->head;

    if(IsListEmpty(list)){
        printf("List is Empty in Print\n");
        return;
    }
    while(now != NULL){
        printf("Print List :: %s\n", now->filename);
        now = now->next;
    }
}

void AddList(List *list, char *filename){
    Node *temp;

    temp = (Node*)malloc(sizeof(Node));

    if( (strcpy(temp->filename, filename)) == NULL ){
        printf("Error occur in strcpy of AddList Function");
        exit(1);
    }
    temp->next = list->head;
    list->head = temp;
}



//implementation of stack
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

