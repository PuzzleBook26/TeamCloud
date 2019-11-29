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

//for list
int IsListEmpty(List *);
int FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);


//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

const int FALSE = 0;
const int TRUE = 1;



//for dir system
ino_t   get_inode(char *);
void    do_ls(char []);
void    dostat(char *);
void    DFS();
int     is_dir(char *);
void error_handling(char *message);
void    ExitsFile(char *, int);
void    FillList(List *);

static int count = 0;

char *root = "/home/puzzlebook/";
char *username = "Server";

Stack stack;
List list;

int main(int argc, char **argv){
    int serv_sock;
    int clnt_sock;
    char message[BUFSIZE];
    int str_len;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;

    if(argc != 2) {
        printf("Usage : &s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);    /* 서버 소켓 생성 */
    if(serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    /* 소켓에 주소 할당 */
    if( bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr) )==-1)
        error_handling("bind() error");

    if(listen(serv_sock, 5) == -1)  /* 연결 요청 대기 상태로 진입 */
        error_handling("listen() error");

    clnt_addr_size = sizeof(clnt_addr);

    /* 연결 요청 수락 */
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1)
        error_handling("accept() error");
    printf("connection is good\n");

    chdir(root);
    chdir(username);

    while(1) {

        memset(message, 0x00, sizeof(message));
        read(clnt_sock, message, sizeof(char));
        //printf("1--command :: %s\n", message);

        if( (strcmp(message, "1")) == 0 ){ // chdir
            str_len = read(clnt_sock, message, sizeof(int));
            //printf("%s\n", message);

            str_len = atoi(message);
            read(clnt_sock, message, str_len);
            //printf("2--change directory :: %s\n", message);
            
            if(strcmp(message, "..") == 0){
                chdir("..");
                continue;
            }
            // 여기에 들어온 input에 대해서 chdir 혹은 mkdir
            FillList(&list);
            ExitsFile(message, 1);  
        }

        else if( strcmp(message, "2") == 0 ){ // saerch
            read(clnt_sock, message, sizeof(char));
            // printf("2번 속 판별 %s\n", message);
            while(strcmp(message, "3") == 0){
                memset(message, 0x00, sizeof(message));
                str_len = read(clnt_sock, message, sizeof(int));
                // printf(">>%s\n", message);

                str_len = atoi(message);
                read(clnt_sock, message, str_len);
                //printf("3--search file name :: %s\n", message);
                
                //여기에 들어온 input 에 대해서 파일 있는지 확인
                //만약 파일이 없으면 여기서 한번 메세지 보내서 통신 종료후 파일 요청
                FillList(&list);
                ExitsFile(message, 2);

                memset(message, 0x00, sizeof(message));
                str_len = read(clnt_sock, message, sizeof(int));
                //printf(">>>>%s\n", message);

                str_len = atoi(message);
                read(clnt_sock, message, str_len);
                //printf("3--search file size :: %s\n", message);
                //여기에 들어온 size랑 비교해서 값이 업데이트 되었는지 확인
                //여기서도 메세지 전송해서 비교 후 파일 요청 혹은 다음라인
                memset(message, 0x00, sizeof(message));
                read(clnt_sock, message, sizeof(char));
                //printf("판별 :: %s\n", message);

            }
            //이 while문 밖에서 case2번의 경우 선택되지 않은 파일들 모두 삭제
        }else if( strcmp(message, "5") == 0 ){
            printf("통신 is over!! \n");
            break;
        }


        continue;


    }
    close(clnt_sock);       /* 연결 종료 */

    return 0;
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

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
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


// do_ls가 읽어서 넘겨준 input string에 대한 파일 이름과 파일 크기를 전송하고
// 만약 디렉토리일 경우 이후의 순회를 위해서 스택에 넣어주는 부분
// 사용 시스템콜 :: stat
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




