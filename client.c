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
char *root = "/home/kyj0609/바탕화면/";


int main(int argc, char** argv){
	int fd_socket, result, len;
	struct sockaddr_in serv_addr;
    	char dirname[50];
	char initpath[100]; 

	if (argc != 4) {
		printf("Usage : %s <IP> <port> <dirname>\n", argv[0]);
		exit(1);
	}
    strcpy(dirname, argv[3]);

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
    
    	//dirname을 상대방에게 알려주는 부분
        write(fd_socket, dirname, strlen(dirname));
	while(1){
		printf("\033[1;32;99m");
		memset(buf, 0, BUFSIZE);
		printf("동기화 모드 (push, pull, cancel) : ");
		scanf("%s", buf);
		printf("\033[0m");
		write(fd_socket, buf, strlen(buf));
		getchar();
        	if(!strcmp(buf, "push")){
        	    chdir(root);
        	    sync_send(fd_socket, dirname );
		    break;
        	}
        	else if(!strcmp(buf, "pull")){
        	    chdir(root);
        	    sync_recv(fd_socket);
		    break;
        	}
		else if(!strcmp(buf, "cancel")){
		    printf("동기화 취소\n");
		    chdir(dirname);
		    break;		
		}
		else{
		    printf("잘못된 동기화 명령어 입력\n");
		}
	}
	strcpy(initpath, root);
	strcat(initpath, dirname);
        chdir(initpath);
	while (1) {  // client main
		memset(buf, 0, BUFSIZE);
		printf("\033[1;32;99m");
		printf("\n명령어 입력 (upload, download, remove, ls, myls, cd, pwd, mypwd, quit) : ");
		scanf("%s", buf);
		getchar();
		write(fd_socket, buf,strlen(buf)); // which command? to server
		//getchar();
		printf("\033[0m");
		if(!strcmp(buf,"upload")){
			if((result = client_upload(fd_socket)) == -1){
				printf("업로드 실패 - 존재하지 않는 파일 이름입니다.\n");
			}
		}

		else if(!strcmp(buf, "download")){
			if((result = client_download(fd_socket)) == -1){
				printf("다운로드 실패 - 존재하지 않는 파일 이름입니다.\n");
			}

		}
		else if(!strcmp(buf, "pwd")){
			memset(buf, 0, BUFSIZE);
			read(fd_socket, buf, BUFSIZE);
			printf("----------현재 서버 작업 경로----------\n%s\n", buf);
			printf("-------------------------------------\n");
		}
		else if(!strcmp(buf, "mypwd")){
			memset(buf, 0, BUFSIZE);
			getcwd(buf,BUFSIZE);
			printf("----------현재 클라이언트 작업 경로----------\n%s\n", buf);
			printf("------------------------------------------\n");
				
		}
		else if(!strcmp(buf, "ls")){
			if((result = client_ls(fd_socket)) == -1){
				printf("ls error\n");
			}
		}
		else if(!strcmp(buf, "myls")){
			if((result = client_myls()) == -1){
				printf("myls error\n");
			}
		}
		else if(!strcmp(buf, "remove")){
			if((result = client_rm(fd_socket)) == -1){
				printf("삭제 실패 - 존재하지 않는 파일 또는 디렉토리 이름입니다.\n");
			}
		}

		else if(!strcmp(buf, "cd")){
			if((result = client_cd(fd_socket)) == -1){
				printf("경로 이동 실패 - 존재하지 않는 경로입니다.\n");
			}
		}
		else if(!strcmp(buf, "quit")){
			printf("서버와의 연결 종료\n");
			break;
		}
		else{
			printf("잘못된 명령어 입력\n");
		}
	}

	close(fd_socket);
	return 0;
}



int client_rm(int fd_socket){ 
	int len;
	int result;
	printf("삭제할 파일 또는 디렉토리 이름 : ");
	scanf("%s", buf);
	len = strlen(buf);

	write(fd_socket, &len, sizeof(int));
	write(fd_socket, buf, len);         // server에게 삭제할 파일의 이름을 전송 
	read(fd_socket, &result, sizeof(int));
	printf("%s : 삭제 완료\n", buf);
	return result;
}
int client_myls(){
	FILE* fp;
	fp = popen("ls | sort", "r");
	printf("----------현재 클라이언트 파일 목록----------\n");
	while(fgets(buf, BUFSIZE-1, fp) != NULL){
		printf("%s",buf);
	}
	printf("-------------------------------------------\n");
	return 0;
        
}
int client_ls(int fd_socket){
	int readnum;
	int len;
	
	printf("----------현재 서버 파일 목록----------\n");

	while(1){
		read(fd_socket, &len, sizeof(int));  // server로부터 ls | sort output을 받아 print
		if(len == 0) break;
		readnum = read(fd_socket, buf, len);
		printf("%s\n", buf);
		memset(buf, 0, BUFSIZE);
	}
	if(readnum == -1){
		perror("read");
		return -1;
	}

	printf("-------------------------------------\n");
	return 0;

	
        
}
int client_cd(int fd_socket){
	memset(buf, 0, BUFSIZE);
	printf("이동할 경로 : ");
	scanf("%s", buf);
	getchar();
	write(fd_socket, buf,strlen(buf));
	
	if(chdir(buf) == 0)
		return 0;
	else
		return -1;
}
int client_upload(int fd_socket){
	int fd_file;
	int readnum;
	int size, result=0;
	char filename[100];
	
	struct stat info;
	memset(buf, 0, BUFSIZE);
	printf("업로드 할 파일 : ");
	scanf("%s", filename);
	getchar();
	size = strlen(filename);
	write(fd_socket, &size, sizeof(int));
	write(fd_socket, filename, strlen(filename));

	if((fd_file = open(filename, O_RDONLY)) == -1){
		result = -1;
		write(fd_socket, &result, sizeof(int));
		return -1;
		
	}
	write(fd_socket, &result, sizeof(int));
	printf("%s : 업로드 중  ...\n", filename);
	stat(filename, &info);
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
	printf("%s : 업로드 완료 \n", filename);
	
	return 0;
}
int client_download(int fd_socket){
	int fd_file, readnum, size, result=0;
	char filename[100];
	mode_t st_mode;
	memset(buf, 0, BUFSIZE);
	printf("다운로드 할 파일 : ");
	scanf("%s", filename);
	getchar();
	size = strlen(filename);
	write(fd_socket, &size, sizeof(int));
	write(fd_socket, filename, strlen(filename));
	read(fd_socket, &result, sizeof(int));

	if(result == -1)
		return -1;

	read(fd_socket, &st_mode, sizeof(st_mode));
	if((fd_file = creat(filename, st_mode )) == -1){
		return -1;
	}
	printf("%s : 다운로드 중  ...\n", filename);
	memset(buf, 0, BUFSIZE);
	
	while(1){
		memset(buf, 0, BUFSIZE);
		read(fd_socket, &size, sizeof(int));
		
		if(size == 0) break;
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

	printf("%s : 다운로드 완료\n", filename);
	return 0;

   
	
}
void error_handling(char* s1, char* s2){
	fprintf(stderr, "Error : %s ", s1);
	perror(s2);
	exit(1);
}
