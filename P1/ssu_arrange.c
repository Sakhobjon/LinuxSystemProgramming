#include "ssu_header.h"


// 파일 복사 함수 (중복이면 _1, _2 등을 붙여 저장)
int copy_file(const char *src, const char *dest) {
    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) {
        perror("fopen src");
        return -1;
    }
    char destPath[PATH_MAX];
    strncpy(destPath, dest, PATH_MAX);
    
    struct stat st;
    int suffix = 1;
    while (stat(destPath, &st) == 0) {
        snprintf(destPath, PATH_MAX, "%s_%d", dest, suffix++);
    }
    
    FILE *fdest = fopen(destPath, "wb");
    if (!fdest) {
        perror("fopen dest");
        fclose(fsrc);
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fsrc)) > 0) {
        if (fwrite(buffer, 1, bytes, fdest) != bytes) {
            perror("fwrite");
            fclose(fsrc);
            fclose(fdest);
            return -1;
        }
    }
    fclose(fsrc);
    fclose(fdest);
    return 0;
}


// 파일 확장자 추출 (없으면 "noext" 반환)
char* get_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot && dot != filename)
        return strdup(dot + 1);
    return strdup("noext");
}



// is_excluded: -x 옵션 적용. parameter->exclude에 쉼표로 구분된 문자열이 있다면,
// 그 중 하나라도 relPath에 포함되어 있으면 제외(true) 반환.
int is_excluded(const char *relPath, command_parameter *param) {
    if (!(param->commandopt == OPT_X) || param->excludes == NULL)
        return 0;
    char *excludes = strdup(param->excludes);
    if (!excludes)
        return 0;
    int result = 0;
    char *token = strtok(excludes, ",");
    while (token) {
        if (strstr(relPath, token) != NULL) {
            result = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(excludes);
    return result;
}

// extension_allowed: -e 옵션 적용. parameter->extention에 쉼표로 구분된 문자열이 있다면,
// file_ext가 목록에 존재하면 허용(true), 아니면 false.
int extension_allowed(const char *file_ext, command_parameter *param) {
    if (!(param->commandopt == OPT_E) || param->extensions == NULL)
        return 1;
    char *exts = strdup(param->extensions);
    if (!exts)
        return 1;
    int allowed = 0;
    char *token = strtok(exts, ",");
    while (token) {
        if (strcasecmp(file_ext, token) == 0) {
            allowed = 1;
            break;
        }
        token = strtok(NULL, ",");
    }
    free(exts);
    return allowed;
}

DirNode *create_node(const char *name, const char *fullPath, int is_directory) {
    DirNode *node = (DirNode *)malloc(sizeof(DirNode));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->name = strdup(name);
    if (!node->name) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    strncpy(node->fullPath, fullPath, PATH_MAX);
    node->is_directory = is_directory;
    node->child = NULL;
    node->sibling = NULL;
    return node;
}

// build_tree: 입력 디렉터리(baseDir)에서 재귀적으로 트리 구축.
// relPath: 현재 상대 경로, param: 옵션(Parameter) 사용
DirNode *build_tree(const char *baseDir, const char *relPath, command_parameter *param) {
    char path[PATH_MAX];
    if (strlen(relPath) == 0)
        snprintf(path, PATH_MAX, "%s", baseDir);
    else
        snprintf(path, PATH_MAX, "%s/%s", baseDir, relPath);
    
    DIR *dir = opendir(path);
    if (!dir) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode))
            return create_node(relPath, path, 0);
        perror("opendir");
        return NULL;
    }
    
    DirNode *currentDir = create_node(relPath[0] ? relPath : baseDir, path, 1);
    DirNode *firstChild = NULL, *lastChild = NULL;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char newRelPath[PATH_MAX];
        if (strlen(relPath) == 0)
            snprintf(newRelPath, PATH_MAX, "%s", entry->d_name);
        else
            snprintf(newRelPath, PATH_MAX, "%s/%s", relPath, entry->d_name);
        
        // -x 옵션 적용
        if (is_excluded(newRelPath, param))
            continue;
        
        char fullPath[PATH_MAX];
        snprintf(fullPath, PATH_MAX*2, "%s/%s", baseDir, newRelPath);
        
        struct stat st;
        if (stat(fullPath, &st) < 0) {
            perror("stat");
            continue;
        }
        
        DirNode *childNode = NULL;
        if (S_ISDIR(st.st_mode)) {
            childNode = build_tree(baseDir, newRelPath, param);
        } else if (S_ISREG(st.st_mode)) {
            // -t 옵션: 시간 조건 검사 (마지막 수정 시간이 param->time 초 이내)
            if (param->commandopt == OPT_T) {
                time_t now = time(NULL);
                if (difftime(now, st.st_mtime) > param->time)
                    continue;
            }
            // -e 옵션: 확장자 필터 검사
            char *fileExt = get_extension(entry->d_name);
            if (!extension_allowed(fileExt, param)) {
                free(fileExt);
                continue;
            }
            free(fileExt);
            childNode = create_node(entry->d_name, fullPath, 0);
        }
        if (childNode) {
            if (!firstChild) {
                firstChild = childNode;
                lastChild = childNode;
            } else {
                lastChild->sibling = childNode;
                lastChild = childNode;
            }
        }
    }
    closedir(dir);
    currentDir->child = firstChild;
    return currentDir;
}

// traverse_tree: 트리 순회, 파일 노드인 경우 출력 디렉터리(outBase) 내 확장자 폴더에 복사
void traverse_tree(DirNode *node, const char *outBase) {
    if (!node)
        return;
    if (!node->is_directory) {
        char *fileExt = get_extension(node->name);
        char outDir[PATH_MAX];
        snprintf(outDir, PATH_MAX, "%s/%s", outBase, fileExt);
        free(fileExt);
        
        struct stat st;
        if (stat(outDir, &st) < 0) {
            if (mkdir(outDir, 0755) < 0) {
                perror("mkdir sub-directory");
                return;
            }
        }
        char destPath[PATH_MAX];
        snprintf(destPath, PATH_MAX*2, "%s/%s", outDir, node->name);
        copy_file(node->fullPath, destPath);
        
    }
    traverse_tree(node->child, outBase);
    traverse_tree(node->sibling, outBase);
}

// free_tree: 트리 메모리 해제
void free_tree(DirNode *node) {
    if (!node)
        return;
    free_tree(node->child);
    free_tree(node->sibling);
    free(node->name);
    free(node);
}



int arrange_process(command_parameter *parameter) {


	//Set full_path for child process
       if((full_path = realpath2(parameter->filename)) == NULL) {
           fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
           exit(1);
       }

	   // 기본 출력 디렉터리 이름: <basename>_arranged
	   char defaultOut[PATH_MAX];
	   {
        char *baseName = strrchr(full_path, '/');
        if (baseName)
            baseName++;
        else
            baseName = full_path;
        snprintf(defaultOut, PATH_MAX, "%s_arranged", baseName);
	    }

	    // 출력 디렉터리 설정: -d 옵션이 있으면 parameter->outdir 사용, 없으면 기본 "<basename>_arranged"
    char outBase[PATH_MAX];
    if ((parameter->commandopt == OPT_D) && parameter->outdir) {
        snprintf(outBase, PATH_MAX, "%s", parameter->outdir);
	}else {
        snprintf(outBase, PATH_MAX, "%s", defaultOut);
    }
    
    struct stat st;
    if (stat(outBase, &st) < 0) {
        if (mkdir(outBase, 0755) < 0) {
            perror("mkdir output directory");
            free(full_path);
            return -1;
        }
    }

	printf("%s\n", defaultOut);

	// 디렉터리 트리 구축 (링크드 리스트/트리)
    DirNode *root = build_tree(full_path, "", parameter);
    if (!root) {
        fprintf(stderr, "Failed to build directory tree for %s\n", full_path);
        free(full_path);
        return -1;
    }

    // 트리 순회하며 파일 복사
    traverse_tree(root, outBase);
    free_tree(root);
    free(full_path);
    return 0;
}


