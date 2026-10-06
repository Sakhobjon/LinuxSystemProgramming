#include "ssu_header.h"

char *commanddata[10] = {
    "show",
    "add",
    "modify",
    "remove",
    "help"
};

// Initialize environment (called at the start of main)
void Init() {
    getcwd(exePATH, PATHMAX);
    sprintf(homePATH, "%s", "/home");
    strcat(cleanupdPATH, homePATH);
    strcat(cleanupdPATH, "/ssu_cleanupd");

    sprintf(pwd_path, "%s", exePATH);
    

   
    // Setup .ssu_cleanupd directory

    struct stat st;
    if (stat(cleanupdPATH, &st) == -1) {
        if (mkdir(cleanupdPATH, 0755) == -1) {
            perror("mkdir ~/.ssu_cleanupd");
            exit(1);
        }
    }
    strcat(listPATH, cleanupdPATH);
    strcat(listPATH, "/current_daemon_list");

    // Setup current_daemon_list file
    FILE *file = fopen(listPATH, "a");
    if (file) fclose(file);
    else {
        perror("fopen current_daemon_list");
        exit(1);
    }
}

// Function to process parameters and check if values and paths are okay
int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter) {
    int option = 0;
    struct stat buf;

    switch (command) {
        case CMD_ADD: {
            optind = 0;

            if (parameter->filename == NULL || !strcmp(parameter->filename, "")) {
                help(CMD_ADD);
                return -1;
            }
            if (strlen(parameter->filename) > FILE_MAX) {
                fprintf(stderr, "ERROR: Path too long: %s\n", parameter->filename);
                return -1;
            }

            free(full_path);
            full_path = NULL;
            if ((full_path = realpath2(parameter->filename)) == NULL) {
                fprintf(stderr, "ERROR: Invalid path: %s\n", parameter->filename);
                return -1;
            }

            if (access(full_path, F_OK) == -1 || Path_Type(full_path) == -1) {
                fprintf(stderr, "ERROR: Not a valid directory: %s\n", full_path);
                return -1;
            }

            if (lstat(full_path, &buf) < 0 || strlen(full_path) > PATHMAX) {
                fprintf(stderr, "ERROR: Cannot stat or path too long: %s\n", full_path);
                return -1;
            }

            while ((option = getopt(argcnt, arglist, "d:i:l:x:e:m:")) != -1) {
                switch(option) {
                    case 'd':
                        parameter->commandopt |= OPT_D;
                        parameter->outdir = strdup(optarg);
                        if (Path_Type(parameter->outdir) == -1 || check_path_access(parameter->outdir, parameter->commandopt) == -1) {
                            fprintf(stderr, "ERROR: Invalid output directory\n");
                            return -1;
                        }
                        break;
                    case 'i':
                        parameter->commandopt |= OPT_I;
                        for (int i = 0; optarg[i] != '\0'; i++) {
                            if (!isdigit(optarg[i])) {
                                fprintf(stderr, "ERROR: '-i' requires a natural number\n");
                                return -1;
                            }
                        }
                        parameter->time = atoi(optarg);
                        if (parameter->time <= 0) {
                            fprintf(stderr, "ERROR: '-i' requires a positive integer\n");
                            return -1;
                        }
                        break;
                    case 'l':
                        parameter->commandopt |= OPT_L;
                        parameter->log_limit = atoi(optarg);
                        break;
                    case 'x':
                        parameter->commandopt |= OPT_X;
                        parameter->excludes = strdup(optarg);
                        break;
                    case 'e':
                        parameter->commandopt |= OPT_E;
                        parameter->extensions = strdup(optarg);
                        break;
                    case 'm':
                        parameter->commandopt |= OPT_M;
                        parameter->mode = atoi(optarg);
                        if (parameter->mode < 1 || parameter->mode > 3) {
                            fprintf(stderr, "ERROR: '-m' must be between 1 and 3\n");
                            return -1;
                        }
                        break;
                    default:
                        help(CMD_ADD);
                        return -1;
                }
            }

            if (check_path_access(full_path, parameter->commandopt) == -1) {
                return -1;
            }
            break;
        }
		case CMD_MODIFY: {
            optind = 0;

            if (parameter->filename == NULL || !strcmp(parameter->filename, "")) {
                help(command);
                return -1;
            }
            if (strlen(parameter->filename) > FILE_MAX) {
                fprintf(stderr, "ERROR: Path too long: %s\n", parameter->filename);
                return -1;
            }

            free(full_path);
            full_path = NULL;
            if ((full_path = realpath2(parameter->filename)) == NULL) {
                fprintf(stderr, "ERROR: Invalid path: %s\n", parameter->filename);
                return -1;
            }

            if (access(full_path, F_OK) == -1 || Path_Type(full_path) == -1) {
                fprintf(stderr, "ERROR: Not a valid directory: %s\n", full_path);
                return -1;
            }

            if (lstat(full_path, &buf) < 0 || strlen(full_path) > PATHMAX) {
                fprintf(stderr, "ERROR: Cannot stat or path too long: %s\n", full_path);
                return -1;
            }

            while ((option = getopt(argcnt, arglist, "d:i:l:x:e:m:")) != -1) {
                switch(option) {
                    case 'd':
                        parameter->commandopt |= OPT_D;
                        parameter->outdir = strdup(optarg);
                        if (Path_Type(parameter->outdir) == -1 || check_path_access(parameter->outdir, parameter->commandopt) == -1) {
                            fprintf(stderr, "ERROR: Invalid output directory\n");
                            return -1;
                        }
                        break;
                    case 'i':
                        parameter->commandopt |= OPT_I;
                        for (int i = 0; optarg[i] != '\0'; i++) {
                            if (!isdigit(optarg[i])) {
                                fprintf(stderr, "ERROR: '-i' requires a natural number\n");
                                return -1;
                            }
                        }
                        parameter->time = atoi(optarg);
                        if (parameter->time <= 0) {
                            fprintf(stderr, "ERROR: '-i' requires a positive integer\n");
                            return -1;
                        }
                        break;
                    case 'l':
                        parameter->commandopt |= OPT_L;
                        parameter->log_limit = atoi(optarg);
                        break;
                    case 'x':
                        parameter->commandopt |= OPT_X;
                        parameter->excludes = strdup(optarg);
                        break;
                    case 'e':
                        parameter->commandopt |= OPT_E;
                        parameter->extensions = strdup(optarg);
                        break;
                    case 'm':
                        parameter->commandopt |= OPT_M;
                        parameter->mode = atoi(optarg);
                        if (parameter->mode < 1 || parameter->mode > 3) {
                            fprintf(stderr, "ERROR: '-m' must be between 1 and 3\n");
                            return -1;
                        }
                        break;
                    default:
                        help(command);
                        return -1;
                }
            }

            if (check_path_access(full_path, parameter->commandopt) == -1) {
                return -1;
            }
            break;
        }
		case CMD_REMOVE: {
			if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
				fprintf(stderr, "ERROR: directory path not included\n");
				help(CMD_REMOVE);
				return -1;
			}
			break;
		}
    }
    return 1;
}

// Executes in the child process
void CommandFun(char **arglist) {
    int (*commandFun)(command_parameter *parameter) = NULL;
    command_parameter parameter = {arglist[0], arglist[1], atoi(arglist[2]), atoi(arglist[3]), arglist[4], arglist[5], arglist[6], atoi(arglist[7]), atoi(arglist[8])};

    if (!strcmp(parameter.command, "show")) commandFun = show_process;
    else if (!strcmp(parameter.command, "add")) commandFun = add_process;
    else if (!strcmp(parameter.command, "help")) commandFun = help_process;
	else if (!strcmp(parameter.command, "modify")) commandFun = modify_process;
    else if (!strcmp(parameter.command, "remove")) commandFun = remove_process;

    if (commandFun && commandFun(&parameter) != 0) exit(1);
}

void CommandExec(command_parameter parameter) {
    pid_t pid;

    parameter.argv[0] = "command";
    parameter.argv[1] = (char *)malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[1], "%d", hash);
    parameter.argv[2] = parameter.command;
    parameter.argv[3] = parameter.filename;
    parameter.argv[4] = (char *)malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[4], "%d", parameter.commandopt);
    parameter.argv[5] = (char *)malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[5], "%d", parameter.time);
    parameter.argv[6] = parameter.outdir;
    parameter.argv[7] = parameter.excludes;
    parameter.argv[8] = parameter.extensions;
    parameter.argv[9] = (char *)malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[9], "%d", parameter.log_limit);
	parameter.argv[10] = (char *)malloc(sizeof(char *) * 32);
    sprintf(parameter.argv[10], "%d", parameter.mode);

    parameter.argv[11] = (char *)0;

    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        execv(exeNAME, parameter.argv);
        exit(0);
    } else {
        wait(NULL);
    }
}

void Prompt() {
    char input[STRMAX * 2];
    int argcnt = 0;
    char **arglist = NULL;
    int command;
    command_parameter parameter = {NULL, NULL, 0};

    while (true) {
        printf("%d> ", STUDENT_NUM);
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';

        arglist = GetSubstring(input, &argcnt, " ");
        if (argcnt == 0) continue;

        if (!strcmp(arglist[0], "show")) command = CMD_SHOW;
        else if (!strcmp(arglist[0], "add")) command = CMD_ADD;
        else if (!strcmp(arglist[0], "modify")) command = CMD_MODIFY;
        else if (!strcmp(arglist[0], "remove")) command = CMD_REMOVE;
        else if (!strcmp(arglist[0], "help")) command = CMD_HELP;
        else if (!strcmp(arglist[0], "exit")) break;
        else { help(0); continue; }

        ParameterInit(&parameter);
        parameter.command = arglist[0];
        parameter.filename = (argcnt > 1) ? arglist[1] : NULL;

        if (ParameterProcessing(argcnt, arglist, command, &parameter) != -1) {
            CommandExec(parameter);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    Init();

    if (!strcmp(argv[0], "command")) {
        CommandFun(argv + 2);
    } else {
        strcpy(exeNAME, argv[0]);
        hash = HASH_MD5;
        Prompt();
    }

    exit(0);
}

