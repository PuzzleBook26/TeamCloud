#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>


#define BUFSIZE 1024

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

//*****//

void    sync_send(int, char *);
void	sync_recv(int, char *);

//for list
int IsListEmpty(List *);
int FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);


//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

//for dir system
ino_t   get_inode(char *);
void    do_ls(int, char []);
void    dostat(int, char *);
int     is_dir(char *);
void    error_handling(char *message);
void    ExitsFile(char *, int);
void    FillList(List *);

const int FALSE = 0;
const int TRUE = 1;

//상대방이 어떤식으로 데이터를 처리해야할지 알려주는 부분
const char* MSG_chdir  = "1";
const char* MSG_search = "2";
const char* MSG_wait   = "3";
const char* MSG_EXIT   = "4";
const char* MSG_END    = "5";
char* command_length[10];

static int count = 0;  // stack순회를 위해 필요한 부분. 트리의 말단 까지 간 후 올라올떄 사용
char message[BUFSIZE];
Stack stack;
List list;

// sync를 맞추기 위해서 자신의 디렉토리 내용을 메세지로 보내는 부
void sync_send(int sock, char *username){
    char *dirname;

    Push(username, &stack);
    do{
        dirname = Pop(&stack);
        count++;
        do_ls(sock, dirname);

    }while( !(IsEmpty(&stack)) );

    write(sock, MSG_END, sizeof(char));

}

// 싱크를 맞추기 위해서 받은 메세지를 처리하는 부분
void sync_recv(int sock, char *username){
    int str_len;
    chdir(username);

    while(1) {

        // 앞으로 받을 메세지 유형을 읽는 부분
        memset(message, 0x00, sizeof(message));
        read(sock, message, sizeof(char));
	printf("command is %s\n",message);
        // 디렉토리 이름을 읽어 존재하면 chdir, 없으면 mkdir
        if( (strcmp(message, "1")) == 0 ){ 
            read(sock, message, sizeof(int));
            str_len = atoi(message);
            read(sock, message, str_len);

            if(strcmp(message, "..") == 0){
                chdir("..");
                continue;
            }
            // 작업디렉토리에 있는 내용을 읽어서 input으로 들어온 파일 있는지 확인
            FillList(&list);
            ExitsFile(message, 1);  
        }

        // 파일이 존재하는지 물어보는 부분
        // 존재한다면 pass, 존재하지 않는다면 파일 전송 요청
        else if( strcmp(message, "2") == 0 ){
            read(sock, message, sizeof(char));

            // 곧 파일을 보낼테니 준비하라는 부분
            while(strcmp(message, "3") == 0){
                // 파일의 이름을 읽는 부분
                memset(message, 0x00, sizeof(message));
                read(sock, message, sizeof(int));

                str_len = atoi(message);
                read(sock, message, str_len);

                //여기에 들어온 input 에 대해서 파일 있는지 확인
                //만약 파일이 없으면 여기서 한번 메세지 보내서 통신 종료후 파일 요청
                FillList(&list);
                ExitsFile(message, 2);

                // 파일의 크기를 읽는 부분
                memset(message, 0x00, sizeof(message));
                str_len = read(sock, message, sizeof(int));

                str_len = atoi(message);
                read(sock, message, str_len);
                //printf("3--search file size :: %s\n", message);
                //여기에 들어온 size랑 비교해서 값이 업데이트 되었는지 확인
                //여기서도 메세지 전송해서 비교 후 파일 요청 혹은 다음라인

                // 다음 행동을 정해주는 부분
                memset(message, 0x00, sizeof(message));
                read(sock, message, sizeof(char));

            }
            //이 while문 밖에서 case2번의 경우 선택되지 않은 파일들 모두 삭제
        }else if( strcmp(message, "5") == 0 ){
            printf("통신 is over!! \n");
            break;
        }
        continue;


    }

}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


void FillList(List *filelist){
    DIR             *dir_ptr;
    struct  dirent  *direntp;
    char            *backUp;

    filelist->head = NULL;

    if( (dir_ptr = opendir(".")) == NULL){
        printf("Open dir is error in FILL list function\n");
        exit(1);
    }

    while( (direntp = readdir(dir_ptr)) != NULL ){
        if( ( (strcmp(direntp->d_name, ".")) && (strcmp(direntp->d_name, ".."))  ) == 0)
            continue;
        else{
            AddList(filelist, direntp->d_name);
        }
    }
    closedir(dir_ptr);


}

//mode 1 :: chdir
//mode 2 :: open dir
void ExitsFile(char *filename, int mode){
    int result;

    if(mode == 1){  // dir
        if(FindFile(&list, filename)){
            chdir(filename);
        }
        else{
            if(mkdir(filename, 0755) == 0){
                chdir(filename);
                printf("Complete mkdir %s +++++++++++++++++\n\n", filename);
            }
            else
                printf("????   %s mkdir error in ExitsFile\n", filename);
        }
    }
    else if(mode == 2){  // file
        if(FindFile(&list, filename)){
            return;
        }
        else{
            printf("REQUEST]  %s is not exist in this dir!!\n\n", filename);
        }
    }
}


// input string이 dir인지 확인하는 함수.
// dir이면 TRUE(1), 아니면 FALSE(0)
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

// dir인 input string으로 chdir 한 이후 그 디렉토리를 open (opendir and readdir)
// 이후 그 디렉토리 내부의 값들을 하나씩 읽어옴
// 사용 시스템콜 :: opendir,  readdir,  closedir
void do_ls(int sock, char dirname[]){
    DIR             *dir_ptr;
    struct  dirent  *direntp;
    char            *backUp;
    static  char *updir = "..";

    while(count != 0){
        if( (dir_ptr = opendir(dirname)) == NULL ){
            count --;
            chdir("..");        // 스택 말단까지 간 후 위로 올라가는 부분
            printf(" change dir to .. \n");

            write(sock, MSG_chdir, sizeof(char));

            sprintf(command_length, "%d", strlen(updir));
            write(sock, command_length, sizeof(int));
            write(sock, "..", sizeof(".."));
        }
        else
            break;
    }

    if(count != 0){
        //디렉토리 이동 후
        //이동하라는 명령과 함께 이동할 디렉토리 이름 전송
        chdir(dirname);
        printf("change dir to %s\n", dirname);

        //명령어 처리모드 > 파일이름사이즈 > 파일이름
        write(sock, MSG_chdir, sizeof(char));
        sprintf(command_length, "%d", strlen(dirname));
        write(sock, command_length, sizeof(int));
        write(sock, dirname, strlen(dirname));

        write(sock, MSG_search, sizeof(char));
        while( (direntp = readdir(dir_ptr)) != NULL ){
            if( ( (strcmp(direntp->d_name, ".")) && (strcmp(direntp->d_name, ".."))  ) == 0)
                continue;
            else{
                if(is_dir(direntp->d_name)){
                    Push(direntp->d_name, &stack);
                    continue;
                }
                write(sock, MSG_wait, sizeof(char));
                dostat(sock, direntp->d_name);
            }
        }
        write(sock, MSG_EXIT, sizeof(char));
        closedir(dir_ptr);
    }
    else
        return;
}



// do_ls가 읽어서 넘겨준 input string에 대한 파일 이름과 파일 크기를 전송하고
// 만약 디렉토리일 경우 이후의 순회를 위해서 스택에 넣어주는 부분
// 사용 시스템콜 :: stat
void dostat(int sock, char *filename){
    struct stat info;
    int filesize;
    char *filesizeMsg[10];

    if(stat(filename, &info) == -1){
        printf("Do not stat\n");
    }
    else{           // 디렉토리 안의 내용을 말해주고 디렉토리를 스택에 넣는 부분
        sprintf(command_length, "%d", strlen(filename));
        write(sock, command_length, sizeof(int));
        printf("%s\n", command_length);

        write(sock, filename, strlen(filename));
        printf("%s\n", filename);

        //여기서 서버의 응답을 한번 기다려서 파일이 있는지 없는지 확인
        //없으면 파일 보내주고 있으면 다음라인으로

        filesize = info.st_size;
        sprintf(filesizeMsg, "%d", filesize);

        sprintf(command_length, "%d", strlen(filesizeMsg));
        write(sock, command_length, sizeof(int));
        printf("%s\n", command_length);

        write(sock, filesizeMsg, strlen(filesizeMsg)); // 오류 가능성
        printf("%s\n", filesizeMsg);
        //여기서 서버측 응답 한번 기다려서
        //응답에 따라서 파일 전송 혹은 다음라인
    }
}

// input string에 대한 inode를 읽어오는 부분.
// 현재는 미사용
ino_t get_inode(char *filename){
    struct stat info;
    if(stat(filename, &info) == -1){
        fprintf(stderr, "Cannot stat");
        perror(filename);
        exit(1);
    }
    return info.st_ino;
}


/*=================================data structure ========================*/


// implementation of list
int IsListEmpty(List *list){
    if(list->head == NULL)
        return TRUE;
    else
        return FALSE;
}

int FindFile(List *list, char *filename){
    Node *prev;
    Node *now;

    if(IsListEmpty(list)){
        printf("List is Empty in find file\n");
        return FALSE;
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

            return TRUE;
        }
        else{
            prev = now;
            now = now->next;
        }
    }
    printf("Not Find %s :(\n",filename);
    return FALSE;
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
        return 0;
    }
    else{
        now = stack->top;
        if(strcpy(str, now->filename) == 0){
            printf("Error occur in strcpy of Stack pop\n");
            return 0;
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

