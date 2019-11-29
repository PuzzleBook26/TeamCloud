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

// SIGCHLD 는 child process가 종료되었을수때 발생하는 signal
// child process가 종료 될때마다 sig_child 함수호출
// waitpid는 특정 process 종료될때까지 기다린다.
// waitpid의 첫번째 인자를 -1로 지정하면 임의의 child process를 기다린다.
// 세번 째인자로 WNOHANG을 지정하면 다른 child process가 종료될때 까지 bloking되지않고 0을 반환하게 해준다.
// 이로 parent process가 wait에서 blocking 되지 않고 다른 client의 연결을 기다릴 수 있게 해준다.


void sig_child(int signal) {
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0);
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
	char path[100];
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

		client_socket = accept(listen_socket, (struct sockaddr*) & clnt_addr, &clnt_addr_size); // client socket 생
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
			close(listen_socket); // child process에서 listen socket은 close.
			memset(buf, 0, BUFSIZE);
			printf("Cloud Sync ...\n");

			while (1) { // Client에서의 명령어 요청 대성기.
				memset(buf, 0, BUFSIZE);
				printf("\nwait for command ... \n");
				read(client_socket, buf, BUFSIZE); // client가 입력한 명령을 받아옴.
				printf("%s command accepted .\n", buf);

				if (!strcmp(buf, "upload")) {
					if ((result = server_download(client_socket)) == -1) {
						printf("Upload error - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
					}
					else
						printf("Upload complete - Client IP :%s\n", inet_ntoa(clnt_addr.sin_addr));
				}
				else if (!strcmp(buf, "download")) {
					if ((result = server_upload(client_socket)) == -1) {
						printf("Download error - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
					}
					else
						printf("Download complete - Client IP :%s \n", inet_ntoa(clnt_addr.sin_addr));

				}
				else if (!strcmp(buf, "ls")) {
					if ((result = server_ls(client_socket)) == -1) {
						printf("ls error - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
					}
					else
						printf("ls complete - Client IP :%s \n", inet_ntoa(clnt_addr.sin_addr));
				}
				else if (!strcmp(buf, "pwd")) {
					getcwd(buf, BUFSIZE);
					write(client_socket, buf, strlen(buf));
					printf("pwd complete - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
				}
				else if (!strcmp(buf, "cd")) {
					if ((result = server_cd(client_socket)) == -1) {
						printf("cd error - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
					}
					else
						printf("cd complete - Client IP :%s \n", inet_ntoa(clnt_addr.sin_addr));
				}
				else if (!strcmp(buf, "remove")) {
					memset(buf, 0, BUFSIZE);
					read(client_socket, &len, sizeof(int));
					read(client_socket, buf, BUFSIZE);
					printf("%s\n", buf);
					getcwd(path, 100);
					strcat(path, "/");
					strcat(path, buf);
				

					if ((result = server_rm(path, client_socket)) == -1) {
						write(client_socket, &result, sizeof(int));
						printf("remove error - Client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
					}
					else {
						write(client_socket, &result, sizeof(int));
						printf("remove complete - Client IP :%s \n", inet_ntoa(clnt_addr.sin_addr));
					}
				}
				else if (!strcmp(buf, "quit")) {
					printf("Bye !\n");
					break;
				}
				else {
					printf("Invalid command\n");
				}
			}
			printf("클라이언트 연결 해제 - IP : %s, Port : %d\n\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
			close(client_socket); // server와의 연결 종료
			exit(0); // child process 종료
		}
		else {
			
			close(client_socket); // parent process에서는 client_socket 필요하지 않다.
		}

		// 각 process에서 필요하지 않거나 사용이 끝난 socket descriptor를 close 하지 않으면,  
		// process에서 연결을 종료해도 descripotr는 다른 process에서 fork()시에 공유되어 사용되고 있기 때문에
	 	// 여전히 descriptor의 reference count가 1이상의 값을 가져 실제로는 연결이 유지되고 있다.
	
	}
	printf("서버 종료\n");
	close(listen_socket);
	return 0;
}


int server_rm(char* path, int fd_socket){ // Cloud server의 파일, 디렉토리 삭제
	// 파일의 경우 unlink()를 호출,
	// 디렉토리일 경우 해당 디렉토리의 '.' , '..'를 제외한 모든 subdirectory와 file을 삭제해야한다.

	struct stat info;
	DIR* dir_ptr;
	struct dirent* direntp;
	char tmp[1000];
	if(stat(path, &info) == -1)
		return -1;
	if(!(S_ISDIR(info.st_mode))){ // 파일 삭제
		printf("삭제 요청된 파일 : %s\n", path);
		unlink(path);
		return 0;
	}
	printf("path :%s\n", path);
	if((dir_ptr = opendir(path)) == NULL)
		return -1;
	else{ 			      
		while((direntp = readdir(dir_ptr)) != NULL){
			printf("삭제 요청된 디렉토리 : %s\n", path);
			
			if( (!(strcmp(direntp->d_name, "."))) || (!(strcmp(direntp->d_name, "..")))) 
				continue;
			
			sprintf(tmp, "%s/%s", path, direntp->d_name);

			if(stat(tmp, &info) == -1){
				printf("here ?: %s\n", tmp);
				return -1;
			}
			
			if(S_ISDIR(info.st_mode)) // 디렉토리일 경우 server_rm을 재귀호출
				server_rm(tmp, fd_socket); //recursive point
			
			else{ // 파일의 경우 unlink()호출.
				unlink(tmp);
				continue;
			}
		}
		//chdir("..");
		rmdir(path); // '.', '..'을 제외한 모든 파일이 삭제 되었으므로 디렉토리 삭제.
		closedir(dir_ptr);
		return 0;
		
	}
	

}
int server_ls(int fd_socket){

	// 'popen()'을 사용하여 client에게 'ls | sort' 출력을 전달.

	// child process를 생성하여 shell에서 ls | sort 를 수행하도록 한다.
	// 이에 대한 output을 pipe를 통해 parent process로 전달한다.
	// parent process는 받은 output을 buf에 저장하여 client에 전달.
	FILE* fp;
	int len;
	fp = popen("ls | sort", "r");

	while(fgets(buf, BUFSIZE-1, fp) != NULL){
		buf[strlen(buf)-1] = '\0';
		
		len = strlen(buf);
		write(fd_socket, &len, sizeof(int));
		
		write(fd_socket, buf, len);
		memset(buf, 0, BUFSIZE);
	}
	len = 0;
	write(fd_socket, &len, sizeof(int));
	//memset(buf, 0, BUFSIZE);
	pclose(fp);
	return 0;

	
	
}
int server_pwd(int fd_socket){
	memset(buf, 0, BUFSIZE);
	if((getcwd(buf, BUFSIZE)) == NULL)
		return -1;
	send(fd_socket, &buf, BUFSIZE, 0);
	return 0;
	
}
int server_download(int fd_socket){
	int fd_file;
	int readnum;
	int size, result;
	
	mode_t st_mode;
	memset(buf, 0, BUFSIZE);
	read(fd_socket, &size, sizeof(int));
	read(fd_socket, buf, size);
	printf("클라이언트 -> 서버 : %s ...\n", buf);


	
	read(fd_socket, &result, sizeof(int));
	if(result == -1)
		return -1;

	read(fd_socket, &st_mode, sizeof(st_mode));
	
	if((fd_file = open(buf, O_RDWR | O_CREAT, st_mode )) == -1){ // client가 업로드할 파일이름을 받아 파일 생성.
		return -1;
	}

	memset(buf, 0, BUFSIZE);
	while(1){
		memset(buf, 0, BUFSIZE);
		read(fd_socket, &size, sizeof(int));

		if(size == 0){
			break;
		}
		readnum= read(fd_socket, buf, size);

        	if((write(fd_file, buf, readnum)) != readnum){ // client로부터 생성할 파일의 contents를 읽어서 write.
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
	int fd_file, readnum;
	int size, result;

	struct stat info;
	memset(buf, 0, BUFSIZE);

	read(fd_socket, &size, sizeof(int));
	read(fd_socket, buf, size);

	printf("서버 -> 클라이언트 : %s ...\n", buf);

	if((fd_file = open(buf, O_RDONLY)) == -1){ // client가 요청한 파일을 open
		result = -1;
		write(fd_socket, &result, sizeof(int));
		return -1;
	}

	stat(buf, &info);
	write(fd_socket, &info.st_mode, sizeof(info.st_mode));
	memset(buf, 0, BUFSIZE);

	while((readnum = read(fd_file, buf, BUFSIZE)) > 0){ // client에게 open한 파일의 contents를 전송 
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
	
	read(fd_socket, buf, BUFSIZE);
	// 클라이언트가 입력한 경로로 이동.
	if(chdir(buf) == 0)
		return 0;
	else
		return -1;
}
void error_handling(char *message)
{
    fprintf(stderr, "Error is %s :: %s", message, strerror(errno));
}
