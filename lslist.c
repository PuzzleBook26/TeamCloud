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


int IsListEmpty(List *);
void FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);

int IsListEmpty(List *list){
    if(list->head == NULL)
        return 1;
    else
        return 0;
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

/* ==================Main=================== */
int main(){
     List *list = NULL;
     PrintList(&list);
     
     AddList(&list, "A");
     AddList(&list, "B");
     AddList(&list, "C"); 
     AddList(&list, "D"); 
     AddList(&list, "E"); 
     AddList(&list, "F");

     PrintList(&list);
printf("=============================\n");
     FindFile(&list, "B");
     FindFile(&list, "F");
     FindFile(&list, "A");
     FindFile(&list, "asdf");
printf("==========================\n");
     PrintList(&list);
}



















