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

char *root = "/home/puzzlebook/";
char *username = "Client";

int main(int argc, char **argv){
    int sock;
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

        sync_send(sock, username);

        printf("%s sync is clear!!! exit connection\n",username);
        break;



    }
    close(sock);
    return 0;
}



