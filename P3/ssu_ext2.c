#include "ssu_header.h"

int total_dirs = 0;
int total_files = 0;

void get_permissions(uint16_t mode, char *buf) {
    buf[0] = (mode & 0xF000) == 0x4000 ? 'd' : '-';
    buf[1] = (mode & 0400) ? 'r' : '-';
    buf[2] = (mode & 0200) ? 'w' : '-';
    buf[3] = (mode & 0100) ? 'x' : '-';
    buf[4] = (mode & 0040) ? 'r' : '-';
    buf[5] = (mode & 0020) ? 'w' : '-';
    buf[6] = (mode & 0010) ? 'x' : '-';
    buf[7] = (mode & 0004) ? 'r' : '-';
    buf[8] = (mode & 0002) ? 'w' : '-';
    buf[9] = (mode & 0001) ? 'x' : '-';
    buf[10] = '\0';
}

// pread를 사용해 지정된 오프셋에서 읽기 수행 (EINTR 재시도)
ssize_t read_at(int fd, void *buf, size_t count, off_t offset) {
    ssize_t ret;
    while ((ret = pread(fd, buf, count, offset)) < 0 && errno == EINTR) ;
    return ret;
}

// 슈퍼블록 및 그룹 디스크립터를 로드
void load_super_and_group() {
    // 1) 슈퍼블록 읽기
    if (read_at(img_fd, &sb, sizeof(sb), EXT2_SUPERBLOCK_OFFSET) != sizeof(sb)) {
        perror("read superblock"); exit(1);
    }
    // 매직 넘버 검사
    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Not an ext2 fs (magic=0x%x)\n", sb.s_magic);
        exit(1);
    }

    // 블록 크기 계산 (1024 << s_log_block_size)
    block_size = 1024 << sb.s_log_block_size;
    // inode 크기 설정 (0이면 기본 128바이트)
    inode_size = (sb.s_inode_size == 0 ? INODE_SIZE : sb.s_inode_size);
    
    // 그룹 디스크립터 오프셋: 블록 크기가 1024면 2048, 아니면 블록 크기
    off_t gd_offset = (block_size == 1024 ? 2048 : block_size);
    
    // 3) raw 12바이트 덤프로 블록 번호 확인 (디버그용)
    uint8_t raw[12];
    if (read_at(img_fd, raw, sizeof(raw), gd_offset) != sizeof(raw)) {
        perror("read raw GD"); exit(1);
    }
    
    // 4) 그룹 디스크립터 구조체로 읽기
    if (read_at(img_fd, &gd, sizeof(gd), gd_offset) != sizeof(gd)) {
        perror("read group desc"); exit(1);
    }
}

// 주어진 inode 번호의 inode 구조체 읽기
void read_inode(uint32_t ino, struct ext2_inode *inode) {
    uint32_t inodes_per_group = sb.s_inodes_per_group;
    // 그룹 내 인덱스 계산
    uint32_t index = (ino - 1) % inodes_per_group;
    off_t itable_block = gd.bg_inode_table;
    off_t offset = itable_block * block_size + index * inode_size;
    if (read_at(img_fd, inode, sizeof(*inode), offset) != sizeof(*inode)) {
        perror("read inode");
        exit(1);
    }
}

// 디렉터리 엔트리를 순회하며 콜백 호출
// depth: 트리 깊이, extra: 사용자 데이터
typedef void (*dirent_cb)(const char *name, uint32_t ino, void *extra, int depth, char *prefix, int is_last);

void iterate_dir(struct ext2_inode *inode, dirent_cb cb, void *extra, int depth, char *prefix, int is_last) {
    // 블록 버퍼 할당
    uint8_t *block = malloc(block_size);
    for (int i = 0; i < 12; i++) {
        uint32_t bnum = inode->i_block[i];
        if (bnum == 0) continue;
        off_t off = (off_t)bnum * block_size;

        // 디렉터리 블록 읽기
        if (read_at(img_fd, block, block_size, off) != (ssize_t)block_size) {
            perror("read dir block");
            exit(1);
        }

        int pos = 0;
		int count = 0;
		int entry_count = 0;
        while (pos < block_size) {
            struct ext2_dir_entry *de = (void*)(block + pos);
            if (de->inode) entry_count++;
			pos += de->rec_len;
		}

		pos = 0;
		int seen = 0;
		while(pos < block_size) {
			struct ext2_dir_entry *de = (void*)(block + pos);
            if (de->inode) {
                // 이름 복사 및 널 종료
                char name[EXT2_NAME_LEN+1];
                memcpy(name, de->name, de->name_len);
                name[de->name_len] = '\0';
				seen++;
                // 콜백 호출
                cb(name, de->inode, extra, depth, prefix, seen == entry_count);
            }
            pos += de->rec_len;
        }
    }
    free(block);
}

// tree 명령 콜백: 디렉터리 구조를 트리 형태로 출력
void tree_cb(const char *name, uint32_t ino, void *extra, int depth, char *prefix, int is_last) {
    // '.' 및 '..' 스킵
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(name, "lost+found") == 0) return;

	int *opts = (int*)extra;
	int opt_r = opts[0], opt_s = opts[1], opt_p = opts[2];

    // 자식 inode 읽기
    struct ext2_inode child;
    read_inode(ino, &child);

	
	// 디렉토리/파일 카운트
    if ((child.i_mode & 0xF000) == 0x4000)
        total_dirs++;
    else
        total_files++;

	
	printf("%s%s ", prefix, is_last ? "┗" : "┣");

	if (opt_p) {
        char perms[11];
		get_permissions(child.i_mode, perms);
		printf("[%s", perms);
		if (opt_s) {
        printf(" %u", child.i_size);
    	}
		printf("] ");
	}
    else if (opt_s) {
        printf("[%u] ", child.i_size);
    }

	printf("%s\n", name);

    // 디렉터리이면 재귀 호출
    if ((opt_r & OPT_R) && (child.i_mode & 0xF000) == 0x4000) {
		char new_prefix[512];
		snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "   " : "┃  ");

        iterate_dir(&child, tree_cb, extra, depth + 1, new_prefix, 1);
    }
}

// tree 명령 실행 함수
void cmd_tree(char *argline) {
    struct ext2_inode cur_inode;
    uint32_t cur_ino = EXT2_ROOT_INO;
    // 루트 inode 읽기
    read_inode(cur_ino, &cur_inode);

	// count dir and file
	total_dirs = 1;
    total_files = 0;

	//option
	int argc = 0;
	char *argv[16];
	char *token = strtok(argline, " ");
	while (token && argc <16)
		argv[argc++] = token, token = strtok(NULL, " ");
	
	//option fishing 
	int opt_r = 0, opt_s = 0, opt_p = 0;
	optind = 0;
	int opt;
	while (( opt = getopt(argc, argv, "rsp")) != -1){
		switch(opt) {
			case 'r': opt_r |= OPT_R; break;
			case 's': opt_s |= OPT_S; break;
			case 'p': opt_p |= OPT_P; break;
			default:
					  help(CMD_TREE);
					  return;
		}
	}
	char *path = argv[0];
	if(!path) path = ".";

    // 경로가 '.' 또는 '/'가 아니면 토큰 처리
    if (strcmp(path, ".") != 0 && strcmp(path, "/") != 0) {
        char *token, *saveptr;
        token = strtok_r(path, "/", &saveptr);
        while (token) {
            struct ext2_inode parent = cur_inode;
            int found = 0;
            uint8_t *block = malloc(block_size);

            // direct 블록(0~11) 검색
            for (int i = 0; i < 12 && !found; i++) {
                if (parent.i_block[i] == 0) continue;
                read_at(img_fd, block, block_size,
                        (off_t)parent.i_block[i] * block_size);

                int pos = 0;
                while (pos < block_size) {
                    struct ext2_dir_entry *de = (void*)(block + pos);
                    if (de->inode && de->name_len == strlen(token) &&
                        strncmp(de->name, token, de->name_len) == 0) {
                        // 매칭된 토큰의 inode로 이동
                        cur_ino = de->inode;
                        read_inode(cur_ino, &cur_inode);
                        found = 1;
                        break;
                    }
                    pos += de->rec_len;
                }
            }
            free(block);

            if (!found) {
                // 경로가 존재하지 않으면 종료
				fprintf(stderr, "Error: path not found: %s\n", token);
				help(CMD_TREE);
                return;
            }
			//dirtectory check
			if ((cur_inode.i_mode & 0xF000) != 0x4000) {
			    fprintf(stderr, "Error: '%s' is not directory\n", path);
			    return;
			}
            token = strtok_r(NULL, "/", &saveptr);
        }
    }

    // 최종 경로 출력 및 디렉터리 순회 시작
	int opts[3] = {opt_r, opt_s, opt_p};
	char prefix[512] = "";

	if (opt_p & OPT_P) {
        char perms[11];
		get_permissions(cur_inode.i_mode, perms);
		printf("[%s", perms);
        if (opt_s & OPT_S) printf(" %u", cur_inode.i_size);
        printf("] ");
    }
    else if (opt_s == OPT_S) {
        printf("[%u] ", cur_inode.i_size);
    }

	printf("%s\n", path);

    iterate_dir(&cur_inode, tree_cb, opts, 0, prefix, 1);

	printf("\n%d directories, %d files\n", total_dirs, total_files);
}

// print 명령 실행 함수: 파일 내용 출력
void cmd_print(char *argline) {

	char *argv[16];
    int argc = 0;
    char *token = strtok(argline, " ");
    while (token && argc < 16)
        argv[argc++] = token, token = strtok(NULL, " ");

    // 옵션 변수
    int line_limit = -1;

    optind = 0;
    int opt;
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                line_limit = atoi(optarg);
                if (line_limit <= 0) {
                    fprintf(stderr, "print: option requires positive integer -- 'n'\n");
                    return;
                }
                break;
            case '?':
            default:
                return;
        }
    }


    struct ext2_inode cur_inode;
    uint32_t cur_ino = EXT2_ROOT_INO;
    // 루트 inode 읽기
    read_inode(cur_ino, &cur_inode);

    // 경로 토큰화 및 검색
    char *strtok, *saveptr;
    strtok = strtok_r(argline, "/", &saveptr);
    while (strtok) {
        struct ext2_inode parent = cur_inode;
        int found = 0;
        uint8_t *block = malloc(block_size);

        for (int i = 0; i < 12 && !found; i++) {
            if (parent.i_block[i] == 0) continue;
            read_at(img_fd, block, block_size,
                    (off_t)parent.i_block[i] * block_size);
            int pos = 0;
            while (pos < block_size) {
                struct ext2_dir_entry *de = (void*)(block + pos);
                if (de->inode && de->name_len == strlen(strtok) &&
                    strncmp(de->name, strtok, de->name_len) == 0) {
                    // 해당 토큰에 매칭되는 inode로 이동
                    cur_ino = de->inode;
                    read_inode(cur_ino, &cur_inode);
                    found = 1;
                    break;
                }
                pos += de->rec_len;
            }
        }
        free(block);

        if (!found) {
            fprintf(stderr, "print: path not found: %s\n", strtok);
			help(CMD_PRINT);
            return;
        }
        strtok = strtok_r(NULL, "/", &saveptr);
    }

    // 디렉터리인지 검사
    if ((cur_inode.i_mode & 0xF000) == 0x4000) {
        fprintf(stderr, "Error: %s is not file\n", argline);
        return;
    }

    // 파일 블록을 차례로 읽어 stdout으로 출력
    char buf[block_size];
    uint32_t to_read = cur_inode.i_size;
	int line_count = 0;
    for (int i = 0; i < 12 && to_read > 0; i++) {
        if (cur_inode.i_block[i] == 0) break;
        off_t off = (off_t)cur_inode.i_block[i] * block_size;
        ssize_t n = read_at(img_fd, buf, block_size, off);
        if (n < 0) { perror("read file"); return; }

    for (int j = 0; j < n && to_read > 0; j++) {
            putchar(buf[j]);
            if (buf[j] == '\n') {
                line_count++;
                if (line_limit > 0 && line_count >= line_limit) return;
            }
            to_read--;
        }
	}
    // TODO: single indirect 블록 처리 추가 가능
    printf("\n");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <EXT2_IMAGE>\n", argv[0]);
        exit(1);
    }
    img_fd = open(argv[1], O_RDONLY);
    if (img_fd < 0) {
        perror("open image");
        exit(1);
    }
    // 슈퍼블록 및 그룹 디스크립터 초기화
    load_super_and_group();

    char line[1024];
    while (1) {
        printf("%d> ", STUDENT_NUM);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "exit", 4) == 0) break;

        char *cmd = strtok(line, " ");
        if (!cmd) continue;
        if (strcmp(cmd, "tree") == 0){
            char *argline = strtok(NULL, "");
			if (!argline) argline = ".";
            cmd_tree(argline);
        }
        else if (strcmp(cmd, "print") == 0) {
            char *argline = strtok(NULL, "");
            if (!argline) {
                fprintf(stderr, "print: missing path\n");
                continue;
            }
            cmd_print(argline);
        }
		else if (strcmp(cmd, "help") == 0) {
 		    char *subcmd = strtok(NULL, " ");
	
		    if (!subcmd) {
	        // help만 입력한 경우: 전체 명령어 출력
		        help(0);
		    }
		    else if (strcmp(subcmd, "tree") == 0) {
		        help(CMD_TREE);
		    }
		    else if (strcmp(subcmd, "print") == 0) {
		        help(CMD_PRINT);
		    }
		    else if (strcmp(subcmd, "help") == 0) {
		        help(CMD_HELP);
		    }
   			else if (strcmp(subcmd, "exit") == 0) {
		        help(CMD_EXIT);
		    }
			else {
				fprintf(stderr, "Invalid command: %s\n", subcmd);
				help(0);
			}

	    }
		else {
            fprintf(stderr, "Unknown command: %s\n", cmd);
			help(0);
        }
	}


    close(img_fd);
    return 0;
}

