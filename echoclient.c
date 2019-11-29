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
void FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);


//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

//for dir system
ino_t   get_inode(char *);
void    do_ls(char []);
void    dostat(char *);
void    DFS();
int     is_dir(char *);
void 	error_handling(char *message);

const int FALSE = 0;
const int TRUE = 1;

char *root = "/home/puzzlebook/";
char *username = "Client";

Stack stack;
List list;


//상대방이 어떤식으로 데이터를 처리해야할지 알려주는 부분
const char* MSG_chdir  = "1";
const char* MSG_search = "2";
const char* MSG_wait   = "3";
const char* MSG_EXIT   = "4";
const char* MSG_END    = "5";
char* command_length[10];
int sock;


Stack stack;
static int count = 0;  // stack순회를 위해 필요한 부분. 트리의 말단 까지 간 후 올라올떄 사용





int main(int argc, char **argv){
    struct sockaddr_in serv_addr;
    char message[BUFSIZE];
    int str_len;

    char* rootdir[30];

    if(argc != 3) {
        printf("Usage : %s <IP> <port> \n", argv[0]);
        exit(1);
    }

    printf("please type dir that want to get service!! ::  ");
    //  scanf("%s", &rootdir);  
    //strcpy(rootdir, "/home/puzzlebook/Client/");

    sock = socket(PF_INET, SOCK_STREAM, 0);   /* 서버 접속을 위한 소켓 생성 */
    if(sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if( connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("done??\n");

    printf("==%s=%s===\n", root, username);
    chdir(root);

    while(1) {
        printf("server connected!!\n");


        // ============================tour============================//
        if( !is_dir(username) ){
            printf("dir your typed is not dir!! :( \n");
            exit(1);
        }

        Push(username, &stack);
        //    chdir(username);
        DFS(username);

        printf("%s sync is clear!!! exit connection\n",username);
        break;



    }
    close(sock);
    return 0;
}

// 현재 디렉토리 부터 디렉토리를 순회하면서
// 동기화를 위한 정보를 상대 node로 보내주는 부분
void DFS(){
    char *dirname;
    do{
        dirname = Pop(&stack);
        count++;
        printf("\n============In %s============\n", dirname);
        do_ls(dirname);

    }while( !(IsEmpty(&stack)) );
    write(sock, MSG_END, sizeof(char));

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
                dostat(direntp->d_name);
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
void dostat(char *filename){
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


