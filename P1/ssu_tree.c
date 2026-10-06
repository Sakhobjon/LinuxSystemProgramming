#include "ssu_header.h"

int total_dirs = 0;
int total_files = 0;

/* 파일/디렉토리 권한 문자열 생성 */
void get_permissions(mode_t mode, char *permStr) {
    permStr[0] = S_ISDIR(mode) ? 'd' : '-';
    permStr[1] = (mode & S_IRUSR) ? 'r' : '-';
    permStr[2] = (mode & S_IWUSR) ? 'w' : '-';
    permStr[3] = (mode & S_IXUSR) ? 'x' : '-';
    permStr[4] = (mode & S_IRGRP) ? 'r' : '-';
    permStr[5] = (mode & S_IWGRP) ? 'w' : '-';
    permStr[6] = (mode & S_IXGRP) ? 'x' : '-';
    permStr[7] = (mode & S_IROTH) ? 'r' : '-';
    permStr[8] = (mode & S_IWOTH) ? 'w' : '-';
    permStr[9] = (mode & S_IXOTH) ? 'x' : '-';
    permStr[10] = '\0';
}

int cmp(const void *a, const void *b) {
    // a와 b는 각각 char* 요소에 대한 포인터이므로, char**로 캐스팅한 후 역참조하여 문자열을 얻는다.
    const char *pa = *(const char **)a;
    const char *pb = *(const char **)b;
    return strcmp(pa, pb);
}

/*
 * print_tree 함수: 주어진 디렉토리 경로 아래의 파일 및 디렉토리를 계층적으로 출력.
 * showSize: -s 옵션 (크기 출력) 활성화 여부.
 * showPerm: -p 옵션 (권한 출력) 활성화 여부.
 */
void print_tree(const char *path, const char *prefix, int bit) {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "디렉토리 열기 실패: %s (%s)\n", path, strerror(errno));
        return;
    }

    int capacity = 10;
    int count = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return;
    }

    // 디렉토리 내 항목 읽기 (".", ".." 제외)
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
		names[count] = strdup(entry->d_name);
        if (!names[count]) {
            perror("strdup");
            continue;
        }
        count++;
		
		if (count >= capacity) {
            capacity *= 2;
            char **temp = realloc(names, capacity * sizeof(char *));
            if (!temp) {
                perror("realloc");
                break;
            }
            names = temp;
        }
    }
    closedir(dir);

    // 사전순 정렬
	qsort(names, count, sizeof(char *), cmp);
	// 각 항목 출력
    for (int i = 0; i < count; i++) {
        int isLast = (i == count - 1);
        const char *connector = isLast ? "└─" : "├─";
        char newPath[PATHMAX];
        snprintf(newPath, sizeof(newPath), "%s/%s", path, names[i]);

        struct stat st;
        if (stat(newPath, &st) < 0) {
            perror("stat");
            continue;
        }
        
        // 기본 정보: 이름, 디렉토리인 경우 슬래시 추가
        char suffix[2] = "";
        if (S_ISDIR(st.st_mode)) {
            strcpy(suffix, "/");
        }

        // 추가 정보를 담을 버퍼들
        char sizeStr[SIZE_STR] = "";
        char permStr[PERM_STR] = "";
		char resultPerm[13] = "";

        if (bit == OPT_S) {
            snprintf(sizeStr, sizeof(sizeStr), "[%ld] ", st.st_size);
        }
		else if (bit == OPT_P) {
            get_permissions(st.st_mode, permStr);
			snprintf(resultPerm, sizeof(resultPerm), "[%s]", permStr);

        }
		else if ((bit & (OPT_S | OPT_P)) == (OPT_S | OPT_P)){
			get_permissions(st.st_mode, permStr);
			snprintf(sizeStr, sizeof(sizeStr), "%ld", st.st_size);
		}

        // 출력: 접두사 + connector + 추가 정보 + 이름 + suffix
		if (permStr[0] != '\0' && sizeStr[0] != '\0') {  // 두 문자열이 비어있지 않은 경우
            printf("%s%s[%s %s]%s%s\n", prefix, connector, permStr, sizeStr, names[i], suffix);
        } else {
            printf("%s%s%s%s%s%s\n", prefix, connector, resultPerm, sizeStr, names[i], suffix);
        }


        if (S_ISDIR(st.st_mode)) {
            total_dirs++;
            char newPrefix[PATHMAX];
            snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "│   ");
            print_tree(newPath, newPrefix, bit);
        } else {
            total_files++;
        }
        free(names[i]);
    }
    free(names);
}


//Function to execute add command
int tree_process(command_parameter *parameter) {

      //Set full_path for child process
       if((full_path = realpath2(parameter->filename)) == NULL) {
           fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
           exit(1);
       }
      // 입력한 디렉토리의 정보 출력하기
       struct stat st;
       if (stat(full_path, &st) < 0) {
          perror("stat");
          return 1;
       }


		//입력한 dir에 대해 추가 정보
        char sizeStr[SIZE_STR] = "";
        char permStr[PERM_STR] = "";
        char resultPerm[13] = "";

        if (parameter->commandopt == OPT_S) {
            snprintf(sizeStr, sizeof(sizeStr), "[%ld] ", st.st_size);
        }
        else if (parameter->commandopt == OPT_P) {
            get_permissions(st.st_mode, permStr);
            snprintf(resultPerm, sizeof(resultPerm), "[%s]", permStr);

        }
        else if ((parameter->commandopt & (OPT_S | OPT_P)) == (OPT_S | OPT_P)){
            get_permissions(st.st_mode, permStr);
            snprintf(sizeStr, sizeof(sizeStr), "%ld", st.st_size);
        }


		// 디렉토리 정보 출력
		if (permStr[0] != '\0' && sizeStr[0] != '\0') {  
			// 두 문자열이 비어있지 않은 경우
            printf("[%s %s]%s\n", permStr, sizeStr, full_path );
        } else {
            printf("%s%s%s\n", resultPerm, sizeStr, full_path);
        }
		
		//tree 
        print_tree(full_path, "", parameter->commandopt);
        printf("\n총 디렉토리: %d개, 총 파일: %d개\n", total_dirs+1, total_files);


        return 0; 
}

