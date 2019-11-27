#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define BUFSIZE 1024
char buf[BUFSIZE];
int client_upload(int);
int client_download(int);
int client_cd(int);
int client_pwd(int);
int client_ls(int);
int client_rm(int);
void error_handling(char*, char*);
char cur_path[100] = "/home/kyj0609/sysprac/TeamCloud/cloud_client";
int main(int argc, char** argv){
	int fd_socket, result, len;
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
		memset(buf, 0, BUFSIZE);
		printf("type command (upload, download, remove, ls, cd, pwd, quit) : ");
		scanf("%s", buf);
		getchar();
		write(fd_socket, buf,strlen(buf)); // which command? to server
		//getchar();
		printf("command : <<%s>>\n",buf);
		if(!strcmp(buf,"upload")){
			if((result = client_upload(fd_socket)) == -1){
				printf("Upload error : Check the file name\n");
			}
			else
				printf("Upload complete !\n");
		}
		else if(!strcmp(buf, "download")){
			if((result = client_download(fd_socket)) == -1){
				printf("Download error : Check the file name\n");
			}
			else
				printf("Download complete !\n");
			
		}
		else if(!strcmp(buf, "pwd")){
			read(fd_socket, buf, BUFSIZE);
			printf("Current path of the Cloud Directory :%s\n", buf);
		}
		else if(!strcmp(buf, "ls")){
			if((result = client_ls(fd_socket)) == -1){
				printf("ls error\n");
			}
		}
		else if(!strcmp(buf, "remove")){
			if((result = client_rm(fd_socket)) == -1){
				printf("rm error : check the directory name\n");
			}
			else
				printf("rm complete !\n");
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
int client_rm(int fd_socket){
	int len, result;
	printf("remove file name : ");
	scanf("%s", buf);
	len = strlen(buf);
	write(fd_socket, &len, sizeof(int));
	write(fd_socket, buf, len);
	read(fd_socket, &result, sizeof(int));
	return result;
}
int client_ls(int fd_socket){
	int readnum;
	int len;
	//while(1){
	//	//read(fd_socket, &size, sizeof(int));
	//	readnum = read(fd_socket, &len, sizeof(int));
	//	if(len == 0) return 0;
	//	readnum = recv(fd_socket, buf, len, 0);
	//	printf("reanum : %d\n", readnum);
	//	printf("%s\n", buf);
	//	
	//	if(readnum == 0)
	//		return 0;
	//	else if(readnum == -1)
	//		return -1;
	//	//memset(buf, 0, BUFSIZE);
	//}
	//read(fd_socket, &size, sizeof(int));

	while(1){
		read(fd_socket, &len, sizeof(int));
		if(len == 0) break;
		readnum = read(fd_socket, buf, len);
		//printf("<%d>\n", readnum);
		//printf("<%d>\n", strlen(buf));
		printf("%s\n", buf);
		memset(buf, 0, BUFSIZE);
	}
	if(readnum == -1){
		perror("read");
		return -1;
	}
	return 0;

	//memset(buf, 0, BUFSIZE);
	//int readnum, len, fd_file;
	//char* file;
	//recv(fd_socket, &len, sizeof(int), 0);
	//file = malloc(len);
	//recv(fd_socket, file, len, 0);
	//fd_file = creat("ls.txt", O_WRONLY);
	//write(fd_file, file, len);
	//close(fd_file);
	//printf("--The Remote Directory List--\n");
	//system("cat ls.txt");	
        
}
int client_cd(int fd_socket){
	memset(buf, 0, BUFSIZE);
	printf("path to go : ");
	scanf("%s", buf);
	getchar();
	write(fd_socket, buf,strlen(buf));
	printf("path = %s\n", buf);	
	if(chdir(buf) == 0)
		return 0;
	else
		return -1;
}
int client_upload(int fd_socket){
	int fd_file, readnum, size;
	struct stat info;
	memset(buf, 0, BUFSIZE);
	printf("file to upload : ");
	scanf("%s", buf);
	size = strlen(buf);
	write(fd_socket, &size, sizeof(int));
	printf("uploading ...  : %s\n", buf);
	if((fd_file = open(buf, O_RDONLY)) == -1){
		perror("open");
		return -1;
		
	}
	write(fd_socket, buf, strlen(buf));
	stat(buf, &info);
	write(fd_socket, &info.st_mode, sizeof(info.st_mode));
	memset(buf, 0, BUFSIZE);
	while((readnum = read(fd_file, buf, BUFSIZE)) > 0){
		memset(buf, 0, BUFSIZE);
		write(fd_socket, &readnum, sizeof(int));
		if(write(fd_socket, buf, readnum) != readnum){
			perror("write");
			return -1;
		}
	}
	if(readnum == -1){
		perror("read");
		return -1;
	}
	readnum = 0;
	write(fd_socket, &readnum, sizeof(int));
	close(fd_file);
	return 0;
}
int client_download(int fd_socket){
	int fd_file, readnum;
	memset(buf, 0, BUFSIZE);
	printf("file to download : ");
	scanf("%s", buf);
	printf("downloading ... : %s\n", buf);
	write(fd_socket, buf,strlen(buf));
	if((fd_file = open(buf, O_RDWR | O_CREAT,0644)) == -1){
		perror("open");
		return -1;
	}
	//send(fd_socket, buf,strlen(buf), 0);
	memset(buf, 0, BUFSIZE);
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
