#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

ino_t   get_inode(char *);
void    do_ls(char []);
void    dostat(char *);
void    DFS(char []);
void    is_dir(struct stat *);


int main(int ac, char *av[]){
    if(ac != 2){
        printf("usage <app> <path>\n");
        exit(1);
    }
    else{
        printf("%s :: \n",*av);
        //stack top에 *av추가
        DFS(*av);
    }
}

//스택 top에 있는 디렉토리를 하나 꺼네서 do_ls 실행
void DFS(char dirname[]){
    do_ls(dirname); //do_ls(stack top에 있는 디렉토리)
    //스택 탑에있는 디렉토리 판별 후 top 디렉토리로 chdir후 pop해줌
}

//dostat에서 입력으로 들어온 stat을 판별하여 dir이면 stack에 추가
void is_dir(struct stat *info){
    int mode;
    mode = info->st_mode;

    if(S_ISDIR(mode)){
        //스택  top에 추가
        printf(" dir");
    }
    else{
        printf("not dir");
    }
}

//입력으로 들어온 dir에 대해서 그 dir의 엘리먼트들 전부 dostat으로 넘김
void do_ls(char dirname[]){
    DIR             *dir_ptr;
    struct  dirent  *direntp;

    if( (dir_ptr = opendir(dirname)) != NULL )
        fprintf(stderr, "do_ls error");
    else{
        while( (direntp = readdir(dir_ptr)) != NULL )
            //is_dir(direntp->d_name);
            dostat(direntp->d_name);
        closedir(dir_ptr);
    }
}

//파일 이름과 크기 출력 후 이 파일이 dir인지 확인하는 함수 호출
void dostat(char *filename){
    struct stat info;
    if(stat(filename, &info) == -1)
        perror(filename);
    else{
        printf("file name :: %s, file size ::%d\n",filename, info.st_size);
        is_dir(&info);
    }
}

ino_t get_inode(char *filename){
    struct stat info;
    if(stat(filename, &info) == -1){
        fprintf(stderr, "Cannot stat");
        perror(filename);
        exit(1);
    }
    return info.st_ino;
}
