#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


#define STUDENT_NUM 20213379

#define CMD_TREE      0b00001
#define CMD_PRINT   0b00010
#define CMD_HELP     0b01000
#define CMD_EXIT     0b10000
#define NOT_CMD	     0b00000

#define OPT_S		0b001
#define OPT_P		0b010
#define OPT_R		0b100
#define OPT_N		0b101

#define STRMAX 4096
#define FILE_MAX 255
#define PATHMAX 4096

#define SIZE_STR 64
#define PERM_STR 11

// ext2 슈퍼블록이 디스크의 1024바이트 오프셋에 위치
#define EXT2_SUPERBLOCK_OFFSET 1024
// ext2 파일시스템의 매직 넘버
#define EXT2_SUPER_MAGIC       0xEF53
// 루트 디렉터리의 inode 번호
#define EXT2_ROOT_INO          2
// 디렉터리 엔트리명 최대 길이
#define EXT2_NAME_LEN          255
// 기본 inode 구조체 크기
#define INODE_SIZE             128

// on-disk superblock
struct ext2_super_block {
    uint32_t   s_inodes_count;
    uint32_t   s_blocks_count;
    uint32_t   s_r_blocks_count;
    uint32_t   s_free_blocks_count;
    uint32_t   s_free_inodes_count;
    uint32_t   s_first_data_block;
    uint32_t   s_log_block_size;
    uint32_t   s_log_frag_size;
    uint32_t   s_blocks_per_group;
    uint32_t   s_frags_per_group;
    uint32_t   s_inodes_per_group;
    uint32_t   s_mtime;
    uint32_t   s_wtime;
    uint16_t   s_mnt_count;
    uint16_t   s_max_mnt_count;
    uint16_t   s_magic;
    uint16_t   s_state;
    uint16_t   s_errors;
    uint16_t   s_minor_rev_level;
    uint32_t   s_lastcheck;
    uint32_t   s_checkinterval;
    uint32_t   s_creator_os;
    uint32_t   s_rev_level;
    uint16_t   s_def_resuid;
    uint16_t   s_def_resgid;

    // EXT2_DYNAMIC_REV부터 추가된 필드
    uint32_t   s_first_ino;       // First non-reserved inode
    uint16_t   s_inode_size;      // Size of inode structure
    uint16_t   s_block_group_nr;  // Block group number of this superblock
    uint32_t   s_feature_compat;
    uint32_t   s_feature_incompat;
    uint32_t   s_feature_ro_compat;
    uint8_t    s_uuid[16];
    char       s_volume_name[16];
    char       s_last_mounted[64];
    uint32_t   s_algorithm_usage_bitmap;

    // 이후 필드는 필요에 따라 생략 가능
};

// on-disk group descriptor
struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
     // 나머지 필드는 사용하지 않음
};

// on-disk inode
struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
     // 나머지 필드는 사용하지 않음
};

// on-disk directory entry
struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[EXT2_NAME_LEN];
};

static int img_fd;            // 이미지 파일 디스크립터
static uint32_t block_size;   // 계산된 블록 크기
static uint16_t inode_size;   // 계산된 inode 크기
static struct ext2_super_block sb; // 슈퍼블록 정보 저장
static struct ext2_group_desc gd;  // 그룹 디스크립터 저장



//ssu_help.c
void help(int cmd_bit);


