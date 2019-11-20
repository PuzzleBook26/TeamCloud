#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#define BUFSIZE 1024
char buf[BUFSIZE];
int client_send(int, int);
void error_handling(char* message);

int main(int argc, char** argv){
	int fd_socket, fd_file, result;
	struct sockaddr_in serv_addr;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> \n", argv[0]);
		exit(1);
	}

	// socket() 
	fd_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (fd_socket == -1)
		error_handling("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	// connect()
	if (connect(fd_socket, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	while (1) {
		result = client_send(fd_file, fd_socket);
	}
	close(fd_socket);
	return 0;
}

int client_send(int fd_file, int fd_socket){
	int readnum;
	//memset(buf, 0x00, BUFSIZE);
	printf("file name :");
	scanf("%s", buf);
	printf("sending ...  : %s\n", buf);
	if((fd_file = open(buf, O_RDONLY)) == -1){
		perror("open");
	}
	send(fd_socket, buf,strlen(buf), 0);
	memset(buf, 0x00, BUFSIZE);
	while((readnum = read(fd_file, buf, BUFSIZE)) > 0){
		if(write(fd_socket, buf, readnum) != readnum){
			perror("write");
			break;
		}
	}
	if(readnum == -1){
		perror("read");
		return -1;
	}
	close(fd_file);
	return 0;
}
void error_handling(char* message){

	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
