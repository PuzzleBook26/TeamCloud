#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 1025

void error_handling(char *message);

int main(int argc, char **argv)
{

    int serv_sock;
    int clnt_sock;
    int str_len;
    int write_len;
    void *buf_ptr;
    char location[50] = "/home/puzzlebook/temp/";
    char filename[50];

    char buf[BUFSIZE];
    int in_fd, out_fd, n_char;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;


    if(argc != 2) {
        printf("Usage : &s <port>\n", argv[0]);
        exit(1);
    }


    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");


    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));


    if( bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr) )==-1)
        error_handling("bind() error");


    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("client listen!!\n");
    clnt_addr_size = sizeof(clnt_addr);


    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

    if(clnt_sock == -1)
        error_handling("accept() error");


    printf("Client Accept!!\n");
    memset(buf, 0x00, BUFSIZE);

    //read file name
    if( (str_len = read(clnt_sock, buf, BUFSIZE)) > 0 ){
        strcpy(filename, buf);
        printf("file name is %s\n", filename);
    }
    strcat(location, filename);
    memset(buf, 0x00, BUFSIZE);
    if( (out_fd = open(location, O_RDWR | O_CREAT, 0666)) == -1 )
        error_handling("file create and open error");


    printf("begin read file content! \n");
    // read and create file content
    while((str_len = read(clnt_sock, buf, BUFSIZE)) > 0 ){
        // strcpy(location,"/home/puzzlebook/temp/test.txt");
        printf("\nfile location is %s, read file content %d :: %s\n", location, str_len, buf );

        buf_ptr = buf;

        while(str_len > 0){
            if((write_len = write(out_fd, buf, str_len)) <= 0 )
                error_handling("Write error");

            printf("save buffer\n");
            str_len -= write_len;
            buf_ptr += write_len;

            printf("======str_len :: %d  ==write_len :: %d============\n",str_len, write_len);
        }


        if(str_len == EOF ){
            printf("finish file save\n");
            break;
        }
        memset(buf, 0x00, BUFSIZE);


    }

    if(close(out_fd) == -1)
        error_handling("file close Error");
    printf("close???\n");

    //      write(clnt_sock, message, str_len);
    //      write(1, message, str_len);


    close(clnt_sock);
    return 0;

}



void error_handling(char *message)
{
    fprintf(stderr, "Error is %s :: %s", message, strerror(errno));
    exit(1);
}
