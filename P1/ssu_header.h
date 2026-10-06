#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <libgen.h>
#include <utime.h>
#include <ctype.h>

#define true 1
#define false 0

#define HASH_MD5  33

#define STUDENT_NUM 20213379

#define CMD_TREE      0b00001
#define CMD_ARRANGE   0b00010
#define CMD_HELP     0b01000
#define CMD_EXIT     0b10000
#define NOT_CMD	     0b00000

#define OPT_S		0b001
#define OPT_P		0b010
#define OPT_D		0b100
#define OPT_T		0b101
#define OPT_X		0b110
#define OPT_E		0b111

#define STRMAX 4096
#define FILE_MAX 255
#define PATHMAX 4096

#define SIZE_STR 64
#define PERM_STR 11

char exeNAME[PATHMAX];
char exePATH[PATHMAX]; 
char pwd_path[PATHMAX];
char homePATH[PATHMAX];


int hash;

char *full_path;


typedef struct command_parameter {
  char *command;
  char *filename;
  int commandopt;
  int time;
  //for arrange
  char *outdir;
  char *excludes;
  char *extensions;
  char *argv[10];


} command_parameter;


/* --- 디렉터리 트리 (링크드 리스트) 구조 --- */

typedef struct DirNode {
    char *name;              // 파일 또는 디렉터리 이름
    char fullPath[PATH_MAX]; // 전체 경로
    int is_directory;        // 1: 디렉터리, 0: 파일
    struct DirNode *child;   // 첫 번째 자식
    struct DirNode *sibling; // 같은 부모의 다음 노드
} DirNode;

typedef struct _pathNode {
  char path_name[FILE_MAX];
  int depth;

  struct _pathNode *prev_path;
  struct _pathNode *next_path;

  struct _pathNode *head_path;
  struct _pathNode *tail_path;
} pathNode;

char *Tokenize(char *str, char *del);
char **GetSubstring(char *str, int *cnt, char *del);
int path_list_init(pathNode *curr_path, char *path);
void ParameterInit(command_parameter *parameter);
char *realpath2(char* path);
int check_path_access(char* path, int opt);
int Path_Type(const char* path);


//ssu_help.c
void help(int cmd_bit);
int help_process(command_parameter *parameter);

//tree_process.c
int tree_process(command_parameter *parameter);

//arrange_process.c
int arrange_process(command_parameter *parameter);


//ssu_cleanup.c
int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter);
void Init();
void CommandFun(char **arglist);
void CommandExec(command_parameter parameter);
void Prompt();


