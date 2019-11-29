#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <wait.h>


#define BUFSIZE 1024
#define BACKLOG 10
char cur_path[100] = "/home/kyj0609/sysprac/TeamCloud/cloud_server/";

char buf[BUFSIZE];
void error_handling(char *message);
int server_download(int);
int server_upload(int);
int server_cd(int);
int server_pwd(int);
int server_ls(int);
int server_rm(char*, int);

void sig_child(int signal) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
	return;
}
int main(int argc, char** argv){

	int client_socket;
	int listen_socket;
	int len;
	int result;
	int write_len;
	void* buf_ptr;
	
	char filename[50];
	char command[100];
	pid_t pid;
	
	signal(SIGCHLD, sig_child);

	int in_fd, out_fd, n_char;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;


	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}


	listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1)
		error_handling("socket() error");


	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));


	if (bind(listen_socket, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");


	if (listen(listen_socket, BACKLOG) == -1)
		error_handling("listen() error");

	printf("클라이언트 연결 요청 대기 중...\n");
	// 서버에서 여러 Client를 관리하기 위한 Concurrent Server
	while (1) {
		clnt_addr_size = sizeof(clnt_addr);

		client_socket = accept(listen_socket, (struct sockaddr*) & clnt_addr, &clnt_addr_size);
		if (client_socket == -1)
			error_handling("accept() error");
		else{
			printf("새로운 클라이언트 연결 - IP : %s, Port : %d\n\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
		}
		// 새로운 Client 접속시 fork()를 호출하여 각 Client마다 Child process Server를 만들어준다.
		// 생성된 Child Server가 각 Client의 접속을 관리하게 된다.
		pid = fork(); 
		if (pid == -1) {
			close(client_socket);
			perror("fork");

		}
		if (pid == 0) {
			close(listen_socket);
			memset(buf, 0, BUFSIZE);
			printf("Cloud Sync ...\n");

			while (1) { // Client에서의 명령어 요청 대기.
				memset(buf, 0, BUFSIZE);
				printf("wait for command ... \n");
				read(client_socket, buf, BUFSIZE); // command from client , which command?
				printf("%s command accepted\n", buf);

				if (!strcmp(buf, "upload")) {
					if ((result = server_download(client_socket)) == -1) {
						printf("Upload error : check the file name\n");
					}
					else
						printf("Upload complete !\n");
				}
				else if (!strcmp(buf, "download")) {
					if ((result = server_upload(client_socket)) == -1) {
						printf("Download error : check the file name\n");
					}
					else
						printf("Download complete !\n");

				}
				else if (!strcmp(buf, "ls")) {
					if ((result = server_ls(client_socket)) == -1) {
						printf("ls error\n");
					}
					else
						printf("ls complete !\n");
				}
				else if (!strcmp(buf, "pwd")) {
					getcwd(buf, BUFSIZE);
					write(client_socket, buf, strlen(buf));
				}
				else if (!strcmp(buf, "cd")) {
					if ((result = server_cd(client_socket)) == -1) {
						printf("cd error : check the directory name\n");
					}
					else
						printf("cd complete !\n");
				}
				else if (!strcmp(buf, "remove")) {
					memset(buf, 0, BUFSIZE);
					read(client_socket, &len, sizeof(int));
					read(client_socket, buf, BUFSIZE);
					printf("%s\n", buf);
					if ((result = server_rm(buf, client_socket)) == -1) {
						write(client_socket, &result, sizeof(int));
						printf("remove error : check the directory name\n");
					}
					else {
						write(client_socket, &result, sizeof(int));
						printf("remove complete : %s\n", buf);
					}
				}
				else if (!strcmp(buf, "quit")) {
					printf("bye !\n");
					break;
				}
				else {
					printf("Invalid command\n");
				}
			}
			printf("클라이언트 연결 해제 - IP : %s, Port : %d\n\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
			close(client_socket);
			exit(0);
		}
		else {
			
			close(client_socket);
		}
	
	}
	printf("server close\n");
	close(listen_socket);
	return 0;
}


int server_rm(char* filename, int fd_socket){
	struct stat info;
	DIR* dir_ptr;
	struct dirent* direntp; 
	if(stat(filename, &info) == -1)
		return -1;
	if(!(S_ISDIR(info.st_mode))){ // file
		unlink(filename);
		return 0;
	}
	if((dir_ptr = opendir(filename)) == NULL)
		return -1;
	else{
		while((direntp = readdir(dir_ptr)) != NULL){
			printf("filename : %s\n", filename);
			if( (!(strcmp(direntp->d_name, "."))) || (!(strcmp(direntp->d_name, ".."))))
				continue;
			chdir(filename);
			if(stat(direntp->d_name, &info) == -1)
				return -1;
			
			if(S_ISDIR(info.st_mode))
				server_rm(direntp->d_name, fd_socket); //recursive
			
			else{
				unlink(direntp->d_name);
				continue;
			}
		}
		chdir("..");
		rmdir(filename);
		closedir(dir_ptr);
		return 0;
		
	}
	//rmdir(filename);
	//closedir(dir_ptr);

}
int server_ls(int fd_socket){
	//DIR *dir_ptr;
	//int len;
	//struct dirent *direntp;
	//if((dir_ptr = opendir(".")) == NULL)
	//	return -1;
	//else{
	//	while((direntp = readdir(dir_ptr)) != NULL){
	//		strcpy(buf, direntp->d_name);
	//		len = strlen(buf);
	//		write(fd_socket, &len, sizeof(int));
	//		printf("string : %s\n", buf);
	//		printf("streln : %d\n", strlen(buf));
	//		write(fd_socket, buf, strlen(buf));
	//	}
	//	closedir(dir_ptr);
	//}
	//len = 0;
	//write(fd_socket, &len, sizeof(int));
	//write(fd_socket, '\0', sizeof('\0'));
	//shutdownOutput();
	//fflush(fd_socket);
	//write(fd_socket, "\0", strlen("\0"));
	//return 0;

	FILE* fp;
	int len;
	fp = popen("ls | sort", "r");
	while(fgets(buf, BUFSIZE-1, fp) != NULL){
		buf[strlen(buf)-1] = '\0';
		//printf("<<%d>>\n", strlen(buf));
		len = strlen(buf);
		write(fd_socket, &len, sizeof(int));
		//if(!(strcmp(buf,"\n"))) break;
		write(fd_socket, buf, len);
		memset(buf, 0, BUFSIZE);
	}
	len = 0;
	write(fd_socket, &len, sizeof(int));
	//memset(buf, 0, BUFSIZE);
	//write(fd_socket, buf, strlen(buf));
	pclose(fp);
	return 0;

	//struct stat list;
	//int file_fd, len;
	//system("ls >ls.txt");
	//stat("ls.txt", &list);
	//len = list.st_size;
	//send(fd_socket, &len, sizeof(int), 0);
	//file_fd = open("ls.txt", O_RDONLY);
	//sendfile(fd_socket, file_fd, NULL, len);
	
}
int server_pwd(int fd_socket){
	memset(buf, 0, BUFSIZE);
	if((getcwd(buf, BUFSIZE)) == NULL)
		return -1;
	send(fd_socket, &buf, BUFSIZE, 0);
	return 0;
	
}
int server_download(int fd_socket){
	int fd_file, readnum, size, result;
	//struct stat info;
	mode_t st_mode;
	memset(buf, 0, BUFSIZE);
	read(fd_socket, &size, sizeof(int));
	read(fd_socket, buf, size);
	printf("download from client : %s\n", buf);
	printf("downloading ... : %s\n", buf);
	//strcpy(filename, buf);
   	//memset(buf, 0, BUFSIZE);
	read(fd_socket, &result, sizeof(int));
	if(result == -1)
		return -1;
	read(fd_socket, &st_mode, sizeof(st_mode));
	printf("stmode :%d\n", st_mode);
	if((fd_file = open(buf, O_RDWR | O_CREAT, st_mode )) == -1){
		perror("open");
		return -1;
	}
	memset(buf, 0, BUFSIZE);
	while(1){
		memset(buf, 0, BUFSIZE);
		read(fd_socket, &size, sizeof(int));
		printf("<<size : %d >>\n", size);
		if(size == 0){
			printf("here?\n");
			break;
		}
		readnum= read(fd_socket, buf, size);
        	if((write(fd_file, buf, readnum)) != readnum){
           		perror("write");
			return -1;
		}
		if(readnum == -1){
			perror("read");
			return -1;
		}
	}
	close(fd_file);
	return 0;

    
	
}
int server_upload(int fd_socket){
	int fd_file, readnum, size, result;
	struct stat info;
	memset(buf, 0, BUFSIZE);

	read(fd_socket, &size, sizeof(int));
	read(fd_socket, buf, size);
	printf("uploading ...  : %s\n", buf);
	if((fd_file = open(buf, O_RDONLY)) == -1){
		result = -1;
		write(fd_socket, &result, sizeof(int));
		perror("open");
		return -1;
	}
	stat(buf, &info);
	write(fd_socket, &info.st_mode, sizeof(info.st_mode));
	memset(buf, 0, BUFSIZE);
	while((readnum = read(fd_file, buf, BUFSIZE)) > 0){
		write(fd_socket, &readnum, sizeof(int));
		if(write(fd_socket, buf, readnum) != readnum){
			perror("write");
			return -1;
		}
		memset(buf, 0, BUFSIZE);
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
int server_cd(int fd_socket){
	memset(buf, 0, BUFSIZE);
	printf("path = %s\n", buf);
	read(fd_socket, buf, BUFSIZE);
	printf("path = %s\n", buf);
	if(chdir(buf) == 0)
		return 0;
	else
		return -1;
}
void error_handling(char *message)
{
    fprintf(stderr, "Error is %s :: %s", message, strerror(errno));
}
