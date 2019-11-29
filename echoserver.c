#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>


static int count = 0;

char *root = "/home/puzzlebook/";
char *username = "Server";

int main(int argc, char **argv){
    int serv_sock;
    int client_sock;
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
    client_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(client_sock == -1)
        error_handling("accept() error");
    printf("connection is good\n");

    chdir(root);
    sync_recv(client_sock, username);

    
    close(client_sock);       /* 연결 종료 */

    return 0;
}






