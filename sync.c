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

// 이 코드에서 핵심적인 함수들
void    sync_send(int, char *);
void	sync_recv(int);//, char *);

//for list
int IsListEmpty(List *);
int FindFile(List *, char *);
void PrintList(List *);
void AddList(List *, char *);
char* getHead(List *);

//for stack
int     IsEmpty(Stack *);
char*   Pop(Stack *);
void    Push(char[], Stack *);

//for dir system
void    do_ls(int, char []);
void    dostat(int, char *);
int     is_dir(char *);
int     ExitsFile(List *, char *, int);
void    FillList(List *);
int     getSize(char *);
int     file_rm(int, char*);

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

// sync를 맞추기 위해서 자신의 디렉토리 구조를 DFS순회를 하면서 상대방에게
// 디렉토리와 파일 구조를 보내주는 부분
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
void sync_recv(int sock){//, char *username){
    int str_len;
    char filename[50];
    char currunt_path[256];
    char path[100];

    while(1) {
        // 앞으로 받을 메세지 유형을 읽는 부분
        memset(message, 0x00, sizeof(message));
        read(sock, message, sizeof(char));

        // 상대방에게 디렉토리 이름을 전달 받은 경우에 동작을 기술
        // 만약 ..메세지를 받았을 경우 단순히 같이 chdir ..을 수행함
        // 그 외에 다른 메세지를 받았을 경우 chdir를 시도해 보고 실패할 경우 
        // 해당 이름의 디렉토리가 없다는 의미이므로, 디렉토리를 만든 후 이동
        // 후에 이동한 디렉토리 내에 있는 파일들을 전부 list로 넣어줘서 다음 동작을 준비
        if( (strcmp(message, "1")) == 0 ){ 
            read(sock, message, sizeof(int));
            str_len = atoi(message);
            read(sock, message, str_len);

            if(strcmp(message, "..") == 0){
                chdir("..");
                continue;
            }
            if(chdir(message) != 0){
                mkdir(message, 0755);
                chdir(message);
            }
            FillList(&list);
        }

        // 메세지로 파일의 이름을 받은 경우
        // 존재한다면 pass, 존재하지 않는다면 파일 전송 요청
        else if( strcmp(message, "2") == 0 ){
            read(sock, message, sizeof(char));
            while(strcmp(message, "3") == 0){
                // 파일의 이름을 읽어옴
                memset(message, 0x00, sizeof(message));
                read(sock, message, sizeof(int));
                
                str_len = atoi(message);
                read(sock, message, str_len);

                // 만약 들어온 파일의 이름이 디렉토리일 경우
                // case 1번에서 알아서 디렉토리가 생성되기 때문에
                // list에서 해당디렉토리를 지워주고 다시 상대방의 메세지 대기
                if(is_dir(message) == 1){
                    write(sock, "2", sizeof(char));
                    FindFile(&list, message);
                    memset(message, 0x00, sizeof(message));
                    read(sock, message, sizeof(char)); 
                    continue;
                }
                // 만약 들어온 파일이 존재하지 않을 경우
                // 이 부분에서 상대방에게 파일 전송을 요청 함
                // 이후에 다시 상대방 메세지 대기
                if(ExitsFile(&list, message, 2) == FALSE){
                    write(sock, "1", sizeof(char));
                    // todo 
                    memset(message, 0x00, sizeof(message));
                    read(sock, message, sizeof(char)); 
                    continue;
                }
                write(sock, "0", sizeof(char));
                strcpy(filename, message);

                // 파일의 크기를 읽는 부분
                memset(message, 0x00, sizeof(message));
                str_len = read(sock, message, sizeof(int));

                str_len = atoi(message);
                read(sock, message, str_len);
                // 만약 위에서 파일이 존재한다고 여겨졌지만
                // 해당 파일과 상대방이 보낸 파일의 크기가 서로 다른경우
                // 이 경우에는 파일이 업데이트 되었다는 것을 의미하므로
                // 상대방에게 파일 전송을 요청하게됨
                if(getSize(filename) != atoi(message)){
                    write(sock, "1", sizeof(char));

                    printf("Request : %s for size\n ", filename);

                    //todo
                    memset(message, 0x00, sizeof(message));
                    read(sock, message, sizeof(char));
                    continue;
                }                
                write(sock, "0", sizeof(char));

                // 다음 행동을 알려주는 메세지 읽는 부분
                memset(message, 0x00, sizeof(message));
                read(sock, message, sizeof(char));
            }
            // 만약 지금까리 리스트에 남아있는 내용이 있다는 의미는
            // 나에게는 존재하지만 상대방에게는 존재하지 않는 내용이므로
            // 서로 동기화를 위해 삭제해야함
            // 따라서 이 부분에는 리스트에 남아있는 내용을 하나씩 꺼내오면서 삭제하게됨
            while(list.head){
                realpath(".", currunt_path);
                getcwd(path, 100);
                strcat(path, "/");
                strcat(path, getHead(&list));
                file_rm(sock, path);
                chdir(currunt_path);
            }
            //상대방의 통신 종료 메세지를 받고 통신이 종료되는 부분
        }else if( strcmp(message, "5") == 0 ){
            printf("통신 is over!! \n");
            break;
        }
        continue;
    }
}


// 파일이름을 받아서 재귀적으로 파일을 삭제하는 함수
int file_rm(int sock, char *filename){

    struct stat info;
    DIR* dir_ptr;
    struct dirent* direntp;
    char tmp[1000];
    if(stat(filename, &info) == -1)
        return -1;

    if(!(S_ISDIR(info.st_mode))){ // 파일 삭제
        if( unlink(filename) != 0)
            printf("%s 삭제 오류\n", filename);
        return 0;
    }

    if((dir_ptr = opendir(filename)) == NULL)
        return -1;
    else{ 			      
        while((direntp = readdir(dir_ptr)) != NULL){
            printf("삭제 요청된 디렉토리 : %s\n", filename);

            if( (!(strcmp(direntp->d_name, "."))) || (!(strcmp(direntp->d_name, "..")))) 
                continue;
            sprintf(tmp, "%s/%s", filename, direntp->d_name);

            if(stat(tmp, &info) == -1)
                return -1;

            if(S_ISDIR(info.st_mode)) // 디렉토리일 경우 server_rm을 재귀호출
                file_rm(sock, tmp); //recursive point

            else{ // 파일의 경우 unlink()호출.
                if(unlink(tmp) != 0)
                    printf("dirent안에서 파일 삭제 오류 %s \n", direntp->d_name);
                continue;
            }
        }
        if(rmdir(filename) != 0)
            printf("디렉토리 삭제 오류 !! %s\n", filename);
        closedir(dir_ptr);
        return 0;

    }

}

// 현제 디렉토리의 파일과 디렉토리 이름을 list에 채우는 함수
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


// mode 1 :: 디렉토리가 존재하는지 찾는 함수
// mode 2 :: 파일이 존재하는지 찾는 함수
// list에 해당 파일이 존재하는지 찾아보는 함수
//
int ExitsFile(List *list, char *filename, int mode){
    int result;

    if(mode == 1){  // dir
        if(FindFile(&list, filename)){
            chdir(filename);
        }
        else{
            if(mkdir(filename, 0755) == 0){
                chdir(filename);
            }
            else{
                return FALSE;
            }
        }
    }
    else if(mode == 2){  // file
        if(FindFile(&list, filename)){
            return TRUE;
        }
        else{
            return FALSE;
        }
    }
    return TRUE;
}


// 파일 이름을 매개변수로 받아서 파일의 크기를 반환해주는 함수
int getSize(char *filename){
    struct stat info;
    if(stat(filename, &info) == -1){
        printf("Error in getSize\n");
        exit(1);
    }
    return info.st_size;
}

// input string이 디렉토리인지 확인하는 함수.
// dir이면 TRUE(1), 아니면 FALSE(0)
int is_dir(char *filename){
    int mode;
    struct stat info;

    if(stat(filename, &info) == -1)
        return;
    mode = info.st_mode;

    if(S_ISDIR(mode)){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

// 입력으로 들어온 디렉토리를 열어서 해당 디렉토리 안에 있는 요소들의 이름을
// 하나하나 상대방에게 전달해주는 함수
void do_ls(int sock, char dirname[]){
    DIR             *dir_ptr;
    struct  dirent  *direntp;
    char            *backUp;
    static  char *updir = "..";

    while(count != 0){
        if( (dir_ptr = opendir(dirname)) == NULL ){
            count --;
            chdir("..");        // 스택 말단까지 간 후 위로 올라가는 부분
            
            write(sock, MSG_chdir, sizeof(char));
            sprintf(command_length, "%d", strlen(updir));
            write(sock, command_length, sizeof(int));
            write(sock, "..", sizeof(".."));
        }
        else
            break;
    }

    if(count != 0){
        write(sock, MSG_chdir, sizeof(char));
        sprintf(command_length, "%d", strlen(dirname));
        write(sock, command_length, sizeof(int));
        write(sock, dirname, strlen(dirname));
        chdir(dirname);

        write(sock, MSG_search, sizeof(char));
        while( (direntp = readdir(dir_ptr)) != NULL ){
            if( ( (strcmp(direntp->d_name, ".")) && (strcmp(direntp->d_name, ".."))  ) == 0)
                continue;
            else{
                if(is_dir(direntp->d_name)){
                    Push(direntp->d_name, &stack);
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
    char *returnMsg[50];

    if(stat(filename, &info) == -1){
        printf("Do not stat\n");
    }
    else{           // 디렉토리 안의 내용을 말해주고 디렉토리를 스택에 넣는 부분
        sprintf(command_length, "%d", strlen(filename));
        write(sock, command_length, sizeof(int));

        write(sock, filename, strlen(filename));

        read(sock, returnMsg, sizeof(char));
        if(strcmp(returnMsg, "1") == 0){
            printf("Send file %s !! \n", filename);
            //todo
            return;
        }
        else if(strcmp(returnMsg, "2") == 0){
            return;
        }

        filesize = info.st_size;
        sprintf(filesizeMsg, "%d", filesize);

        sprintf(command_length, "%d", strlen(filesizeMsg));
        write(sock, command_length, sizeof(int));

        write(sock, filesizeMsg, strlen(filesizeMsg)); // 오류 가능성

        //여기서 서버측 응답 한번 기다려서
        //응답에 따라서 파일 전송 혹은 다음라인
        read(sock, returnMsg, sizeof(char));
        if(strcmp(returnMsg, "1") == 0){
            printf("Send file %s !! \n", filename);
            // todo
            return;
        }
    }
}

/*=================================data structure ========================*/


// ---------------- implementation of list --------------//
char* getHead(List *list){
    Node *now;
    char *str;

    if(IsListEmpty(list))
        return;

    now = list->head;
    list->head = now->next;

    str = malloc(sizeof(char) * strlen(now->filename));
    strcpy(str, now->filename);
    free(now);

    printf("getHead의 반환 스트링이야 %s\n", str);
    return str;
}

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
        //printf("List is Empty in find file\n");
        return FALSE;
    }

    prev = list->head;
    now = list->head;

    while(now){
        if( (strcmp(now->filename, filename)) == 0 ){
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
    //printf("Not Find %s :(\n",filename);
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



//--------------------implementation of stack -------------------//
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

