#include "ssu_header.h"

int show_process(command_parameter *parameter) {
    FILE *fp;
    char list_line[PATHMAX];
    char list_path[PATHMAX];
    char target_path[PATHMAX];
    int index = 1, choice = -1;
    char *daemon_paths[128];

    snprintf(list_path, sizeof(list_path), "%s", listPATH);
    fp = fopen(list_path, "r");
    if (!fp) {
        perror("fopen current_daemon_list");
        return -1;
    }

    printf("Current working daemon process list\n");
    while (fgets(list_line, sizeof(list_line), fp)) {
        list_line[strcspn(list_line, "\n")] = 0; // remove newline
        printf("%d. %s\n", index, list_line);
        daemon_paths[index] = strdup(list_line);
        index++;
    }
    fclose(fp);

    if (index == 1) {
        printf("No daemon process is running.\n");
        return 0;
    }

    printf("0. exit\nSelect one to see process info : ");
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input.\n");
        return 0;
    }
    getchar(); // remove leftover newline

    if (choice <= 0 || choice >= index) {
        printf("Please check your input is valid\n");
        return 0;
    }

    snprintf(target_path, PATHMAX, "%s/ssu_cleanupd.config", daemon_paths[choice]);
    FILE *config_fp = fopen(target_path, "r");
    if (!config_fp) {
        perror("fopen config");
        return -1;
    }

    printf("\n--- Config Information ---\n");
    char line[512];
    while (fgets(line, sizeof(line), config_fp)) {
        printf("%s", line);
    }
    fclose(config_fp);

    snprintf(target_path, PATHMAX, "%s/ssu_cleanupd.log", daemon_paths[choice]);
    FILE *log_fp = fopen(target_path, "r");
    if (!log_fp) {
        perror("fopen log");
        return -1;
    }

    printf("\n--- Log (Last 10 Entries) ---\n");
    char *lines[1000];
    int total_lines = 0;

    while (fgets(line, sizeof(line), log_fp)) {
        lines[total_lines++] = strdup(line);
        if (total_lines >= 1000) break;
    }
    fclose(log_fp);

    int start = total_lines > 10 ? total_lines - 10 : 0;
    for (int i = start; i < total_lines; i++) {
        printf("%s", lines[i]);
        free(lines[i]);
    }

    for (int i = 1; i < index; i++) {
        free(daemon_paths[i]);
    }

    fflush(stdout);
    return 0;
}

