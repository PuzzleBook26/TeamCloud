#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


#define BUFSIZE 1024
char cur_path[100] = "/home/kyj0609/sysprac/TeamCloud/cloud_server/";
char buf[BUFSIZE];
void error_handling(char *message);
int server_download(int);
int server_upload(int);
int server_cd(int);
int server_pwd(int);
int server_ls(int);
int server_rm(char*, int);
int main(int argc, char **argv)
{

    int clnt_sock;
    int serv_sock;
    int len;
    int result;
    int write_len;
    void *buf_ptr;
    char location[50] = "/home/kyj0609/sysprac/TeamCloud/cloud_server/";
    char filename[50];
    char command[100];

    int in_fd, out_fd, n_char;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;


    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
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
    printf("Cloud Sync ...\n");

    while (1) {
	memset(buf, 0, BUFSIZE);
	printf("wait for command ... \n");
	read(clnt_sock, buf, BUFSIZE); // command from client , which command?
	printf("%s command accepted\n", buf);

	if(!strcmp(buf,"upload")){
		if((result = server_download(clnt_sock)) == -1){
			printf("Upload error : check the file name\n");
		}
		else
			printf("Upload complete !\n");
	}
	else if(!strcmp(buf, "download")){
		if((result = server_upload(clnt_sock)) == -1){
			printf("Download error : check the file name\n");
		}
		else
			printf("Download complete !\n");
			
	}
	else if(!strcmp(buf, "ls")){
		if((result = server_ls(clnt_sock)) == -1){
			printf("ls error\n");
		}
		else
			printf("ls complete !\n");
	}
	else if(!strcmp(buf, "pwd")){
		getcwd(buf, BUFSIZE);
		write(clnt_sock, buf, strlen(buf));
	}
	else if(!strcmp(buf, "cd")){
		if((result = server_cd(clnt_sock)) == -1){
			printf("cd error : check the directory name\n");
		}
		else
			printf("cd complete !\n");
	}
	else if(!strcmp(buf, "remove")){
		memset(buf, 0, BUFSIZE);
		read(clnt_sock, &len, sizeof(int));
		read(clnt_sock, buf, BUFSIZE);
		printf("%s\n", buf);
		if((result = server_rm(buf, clnt_sock)) == -1){
			write(clnt_sock, &result, sizeof(int)); 
			printf("remove error : check the directory name\n");		
		}
		else{	
			write(clnt_sock, &result, sizeof(int));
			printf("remove complete : %s\n", buf);
		}
	}
	else if(!strcmp(buf, "quit")){
		printf("bye !\n");
		break;
	}
	else{
		printf("Invalid command\n");
	}
}
	close(clnt_sock);
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
    exit(1);
}
