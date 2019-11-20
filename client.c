#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#define BUFSIZE 1024

void error_handling(char* message);
int main(int argc, char** argv){
	int fd_sock;
	int fd_source;
	char file_buf[BUFSIZE];
	int size = sizeof(file_buf);
	char file_name[BUFSIZE];
	
	int readnum;
	struct sockaddr_in serv_addr;
	//char message[BUFSIZE];
	int str_len;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> \n", argv[0]);
		exit(1);
	}

	// socket() 
	fd_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (fd_sock == -1)
		error_handling("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// connect()
	if (connect(fd_sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	while (1) {
		printf("file name :");
		scanf("%s", file_name);
		printf(" --> %s\n", file_name);
		//memset(file_name, 0x00, BUFSIZE);

		if((fd_source = open(file_name, O_RDONLY)) == -1){
			perror("open");
		}
		send(fd_sock, file_name,strlen(file_name), 0);
		//memset(file_name, 0x00, BUFSIZE);
		while((readnum = read(fd_source, file_buf, BUFSIZE)) > 0){
			if(write(fd_sock, file_buf, readnum) != readnum){
				printf("<<write?>>\n");
				perror("write");
				//break;
			}
		}
		printf("chk");
		if(readnum == -1){
			perror("read");
		}
		close(fd_source);	
	}
	close(fd_sock);
	return 0;
}
void error_handling(char* message){

	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
