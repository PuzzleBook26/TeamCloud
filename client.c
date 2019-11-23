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
int client_upload(int);
int client_download(int);
int client_cd(int);
void error_handling(char*, char*);
char root_cloud[100] = "/home/kyj0609/sysprac/TeamCloud/cloud_client";
int main(int argc, char** argv){
	int fd_socket, result;
	struct sockaddr_in serv_addr;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> \n", argv[0]);
		exit(1);
	}

	// socket() 
	fd_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (fd_socket == -1)
		error_handling("socket() error", "socket");
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	// connect()
	if (connect(fd_socket, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error", "connect");

	printf("Cloud Sync ...\n");

	while (1) {
		printf("type command (upload, dowonload, ,remove, ls, cd, pwd, quit) : ");
		scanf("%s", buf);
		send(fd_socket, buf,strlen(buf), 0); // which command? to server
		getchar();
		if(!strcmp(buf,"upload")){
			if((result = client_upload(fd_socket)) == -1){
				printf("Upload error : check the file name\n");
			}
			else
				printf("Upload complete !\n");
		}
		else if(!strcmp(buf, "download")){
			if((result = client_cd(fd_socket)) == -1){
				printf("Download error : check the file name\n");
			}
			else
				printf("Download complete !\n");
			
		}
		else if(!strcmp(buf, "cd")){
			if((result = client_cd(fd_socket)) == -1){
				printf("cd error : check the directory name\n");
			}
			else
				printf("cd complete !\n");
		}
		else if(!strcmp(buf, "quit")){
			printf("bye !\n");
			break;
		}
		else{
			printf("Invalid command\n");
		}
	}
	close(fd_socket);
	return 0;
}
int client_cd(int fd_socket){
	int chk;
	char path[100];
	memset(buf, 0x00, BUFSIZE);
	printf("path to go : ");
	scanf("%s", buf);
	getchar();
	send(fd_socket, buf,strlen(buf), 0);
	read(fd_socket, &chk, BUFSIZE); // server success?
	if(chk){
		strcpy(path,"./");
		strcat(path,buf);
		strcat(path,"/");
		chdir(path);
		return 0;
	}
	return -1;
}
int client_upload(int fd_socket){
	int fd_file, readnum;
	memset(buf, 0x00, BUFSIZE);
	printf("file to upload : ");
	scanf("%s", buf);
	printf("uploading ...  : %s\n", buf);
	if((fd_file = open(buf, O_RDONLY)) == -1){
		perror("open");
		return -1;
		
	}
	send(fd_socket, buf,strlen(buf), 0);
	memset(buf, 0x00, BUFSIZE);
	while((readnum = read(fd_file, buf, BUFSIZE)) > 0){
		if(write(fd_socket, buf, readnum) != readnum){
			perror("write");
			return -1;
		}
	}
	if(readnum == -1){
		perror("read");
		return -1;
	}
	close(fd_file);
	return 0;
}
int client_download(int fd_socket){
	int fd_file, readnum;
	memset(buf, 0x00, BUFSIZE);
	printf("file to download : ");
	scanf("%s", buf);
	printf("downloading ... : %s\n", buf);
	if((fd_file = open(buf, O_RDWR | O_CREAT,0644)) == -1){
		perror("open");
		return -1;
	}
	send(fd_socket, buf,strlen(buf), 0);
	memset(buf, 0x00, BUFSIZE);
	while((readnum= read(fd_socket, buf, BUFSIZE)) > 0 ){
        	if((write(fd_file, buf, readnum)) != readnum){
           		perror("write");
			return -1;
		}
	if(readnum == -1){
		perror("read");
		return -1;
	}
	close(fd_file);
	return 0;

    }
	
}
void error_handling(char* s1, char* s2){
	fprintf(stderr, "Error : %s ", s1);
	perror(s2);
	exit(1);
}
