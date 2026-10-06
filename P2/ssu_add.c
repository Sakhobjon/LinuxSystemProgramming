#include "ssu_header.h"

FILE *global_log_fp = NULL;

void handle_sigusr1(int sig){
	exit(0);
}

void setup_config_file(command_parameter *parameter, const char *monitor_path, const char *output_path) {
    config_path = (char*)malloc(sizeof(char)*PATH_MAX);
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char start_time_str[64];
    char time_buf[64];

    sprintf(config_path, "%s/ssu_cleanupd.config", monitor_path);
    config_fp = fopen(config_path, "w");
    if (config_fp == NULL) {
        perror("fopen config");
        exit(1);
    }
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(config_fp,
        "monitoring_path : %s\n"
        "pid : %d\n"
        "start_time : %s\n"
        "output_path : %s\n"
        "time_interval : %d\n"
        "max_log_lines : %s\n"
        "exclude_path : %s\n"
        "extension : %s\n"
        "mode : %d\n",
        monitor_path,
        getpid(),
        start_time_str,
        output_path,
        parameter->time,
        (parameter->commandopt & OPT_L) ? (snprintf(time_buf, sizeof(time_buf), "%d", parameter->log_limit), time_buf) : "none",
        (parameter->commandopt & OPT_X && parameter->excludes) ? parameter->excludes : "none",
        (parameter->commandopt & OPT_E && parameter->extensions) ? parameter->extensions : "all",
        parameter->mode
    );
    fclose(config_fp);
}

void setup_log_file(const char *monitor_path) {
    log_path = (char*)malloc(sizeof(char)*PATH_MAX);
    sprintf(log_path, "%s/ssu_cleanupd.log", monitor_path);
    global_log_fp = fopen(log_path, "a+");
    if (global_log_fp == NULL) {
        perror("fopen log");
        exit(1);
    }
}

void register_daemon_in_list(const char *monitor_path) {
    FILE *list_fp = fopen(listPATH, "a");
    if (list_fp == NULL) {
        perror("fopen current_daemon_list");
        exit(1);
    }
    fprintf(list_fp, "%s\n", monitor_path);
    fclose(list_fp);
}

void copy_file_with_log(const char *src_path, const char *dest_path, command_parameter *parameter) {
    FILE *src_fp = fopen(src_path, "r");
    FILE *dst_fp = fopen(dest_path, "w");
    if (!src_fp || !dst_fp) {
        fprintf(stderr, "ERROR opening file: %s or %s\n", src_path, dest_path);
        if (src_fp) fclose(src_fp);
        if (dst_fp) fclose(dst_fp);
        return;
    }

    char buffer[8192];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src_fp)) > 0)
        fwrite(buffer, 1, n, dst_fp);
    fclose(src_fp);
    fclose(dst_fp);

    if (global_log_fp) {
        // log_limit 체크 추가
        int line_count = 0;
        fseek(global_log_fp, 0, SEEK_SET);
        char ch;
        while ((ch = fgetc(global_log_fp)) != EOF) {
            if (ch == '\n') line_count++;
        }
        fseek(global_log_fp, 0, SEEK_END);

        if ((parameter->commandopt & OPT_L) && parameter->log_limit > 0 && line_count >= parameter->log_limit) {
            return;
        }

        char time_buf[64];
        time_t now = time(NULL);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime(&now));
        fprintf(global_log_fp, "[%s][%d][%s][%s]\n", time_buf, getpid(), src_path, dest_path);
        fflush(global_log_fp);
    }
}

void daemon_main_loop(command_parameter *parameter, const char *monitor_path, const char *output_path) {
    setsid();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    chdir(output_path);
    close(0); close(1); close(2);
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

	//Register signal handler for SIGUSR1
    signal(SIGUSR1, handle_sigusr1);


    while (1) {
        DIR *dp;
        struct dirent *entry;
        struct stat st;
        src_path = (char*)malloc(sizeof(char)*PATH_MAX);
        dest_dir = (char*)malloc(sizeof(char)*PATH_MAX);
        dest_path = (char*)malloc(sizeof(char)*PATH_MAX);

        if ((dp = opendir(monitor_path)) != NULL) {
            while ((entry = readdir(dp)) != NULL) {
                if (entry->d_type == DT_REG &&
                    strcmp(entry->d_name, "ssu_cleanupd.log") != 0 &&
                    strcmp(entry->d_name, "ssu_cleanupd.config") != 0 &&
                    strstr(entry->d_name, "_arranged") == NULL) {

                    sprintf(src_path, "%s/%s", monitor_path, entry->d_name);
                    if (stat(src_path, &st) == 0) {
                        char *ext = strrchr(entry->d_name, '.');
                        if (ext != NULL && ext != entry->d_name) {
                            ext++;
                            sprintf(dest_dir, "%s/%s", output_path, ext);
                            mkdir(dest_dir, 0755);
                            sprintf(dest_path, "%s/%s", dest_dir, entry->d_name);

                            if (access(dest_path, F_OK) == 0) {
                                struct stat exist_st;
                                stat(dest_path, &exist_st);
                                if ((parameter->mode == 1 && st.st_mtime <= exist_st.st_mtime) ||
                                    (parameter->mode == 2 && st.st_mtime >= exist_st.st_mtime) ||
                                    parameter->mode == 3) {
                                    continue;
                                }
                            }

                            copy_file_with_log(src_path, dest_path, parameter);
                        }
                    }
                }
            }
            closedir(dp);
        }
        sleep(parameter->time);
    }
   fclose(global_log_fp);
}

int add_process(command_parameter *parameter) {
    char *monitor_path = realpath2(parameter->filename);
    if (monitor_path == NULL) {
        fprintf(stderr, "realpath2 failed\n");
        return -1;
    }

    char *output_path = NULL;
    if (parameter->commandopt & OPT_D) {
        output_path = realpath2(parameter->outdir);
    } else {
        output_path = malloc(PATHMAX);
        snprintf(output_path, PATHMAX, "%s_arranged", monitor_path);
        mkdir(output_path, 0755);
    }

    setup_config_file(parameter, monitor_path, output_path);
    setup_log_file(monitor_path);
    register_daemon_in_list(monitor_path);

    pid_t pid = fork();
    if (pid == 0) {
        daemon_main_loop(parameter, monitor_path, output_path);
    }

    printf("add success: monitoring %s\n", monitor_path);
    return 0;
}


int modify_process(command_parameter *parameter) {
    config_path=(char*)malloc(sizeof(char)*PATH_MAX);
    sprintf(config_path, "%s/ssu_cleanupd.config", parameter->filename);

    int config_fd = open(config_path, O_RDWR);
    if (config_fd < 0) {
        perror("open config");
        return -1;
    }

    struct flock lock = {0};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(config_fd, F_SETLKW, &lock) == -1) {
        perror("fcntl lock");
        close(config_fd);
        return -1;
    }

    config_fp = fdopen(config_fd, "r+");
    if (!config_fp) {
        perror("fdopen");
        close(config_fd);
        return -1;
    }

    // 기존 설정 읽기
    char line[512];
    char monitoring_path[PATHMAX] = "";
    char output_path[PATHMAX] = "";
    int time_interval = 10;
	char start_time[64] = "";
    char max_log_lines[64] = "none";
    char exclude_path[PATHMAX] = "none";
    char extension[64] = "all";
    int mode = 1;
	int o_pid = 0;
    while (fgets(line, sizeof(line), config_fp)) {
        if (strncmp(line, "monitoring_path :", 17) == 0)
            sscanf(line + 18, "%[^\n]", monitoring_path);
		else if (strncmp(line, "start_time :", 12) == 0)
            sscanf(line + 13, "%[^\n]", start_time);
		else if (strncmp(line, "pid :", 5) == 0)
			sscanf(line + 6, "%d", &o_pid);
        else if (strncmp(line, "output_path :", 13) == 0)
            sscanf(line + 14, "%[^\n]", output_path);
        else if (strncmp(line, "time_interval :", 15) == 0)
            sscanf(line + 16, "%d", &time_interval);
		else if (strncmp(line, "max_log_lines :", 15) == 0)
            sscanf(line + 16, "%[^\n]", max_log_lines);
        else if (strncmp(line, "exclude_path :", 14) == 0)
            sscanf(line + 15, "%[^\n]", exclude_path);
        else if (strncmp(line, "extension :", 11) == 0)
            sscanf(line + 12, "%[^\n]", extension);
        else if (strncmp(line, "mode :", 6) == 0)
            sscanf(line + 7, "%d", &mode);
    }

    // 수정할 옵션 반영
    if (parameter->commandopt & OPT_D && parameter->outdir)
        strcpy(output_path, parameter->outdir);
    if (parameter->commandopt & OPT_I)
        time_interval = parameter->time;
    if (parameter->commandopt & OPT_X && parameter->excludes)
        strcpy(exclude_path, parameter->excludes);
    if (parameter->commandopt & OPT_E && parameter->extensions)
        strcpy(extension, parameter->extensions);
    if (parameter->commandopt & OPT_M)
        mode = parameter->mode;

    // 파일 새로 쓰기
    rewind(config_fp);
    ftruncate(config_fd, 0);

    fprintf(config_fp,
        "monitoring_path : %s\n"
        "pid : %d\n"
        "start_time : %s\n"
        "output_path : %s\n"
        "time_interval : %d\n"
        "max_log_lines : %s\n"
        "exclude_path : %s\n"
        "extension : %s\n"
        "mode : %d\n",
        monitoring_path,
		o_pid,
		start_time,
        output_path,
        time_interval,
        max_log_lines,
        exclude_path,
        extension,
        mode
    );

    fflush(config_fp);
    fclose(config_fp);

    printf("modify success: updated config at %s\n", config_path);
    return 0;
}



int remove_process(command_parameter *parameter) {
    config_path = (char*)malloc(sizeof(char)*PATH_MAX);
    sprintf(config_path, "%s/ssu_cleanupd.config", parameter->filename);

    config_fp = fopen(config_path, "r");
    if (!config_fp) {
        perror("fopen config for remove");
        return -1;
    }

    char line[512];
    int target_pid = -1;

    while (fgets(line, sizeof(line), config_fp)) {
        if (strncmp(line, "pid :", 5) == 0) {
            sscanf(line + 6, "%d", &target_pid);
            break;
        }
    }
    fclose(config_fp);

    if (target_pid == -1) {
        fprintf(stderr, "Failed to find pid in config\n");
        return -1;
    }

    if (kill(target_pid, SIGUSR1) == -1) {
        perror("kill (SIGUSR1) to daemon");
        return -1;
    }

    printf("remove success: daemon process %d terminated\n", target_pid);
    return 0;
}

