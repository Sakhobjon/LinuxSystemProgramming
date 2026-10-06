#include "ssu_header.h"

char *commanddata[10]={
    "tree",
    "arrange",
    "help"
};

//Function to process parameters and check if values and paths are okay
int ParameterProcessing(int argcnt, char **arglist, int command, command_parameter *parameter) {
	int option = 0;
	
	switch(command) {
		case CMD_TREE: {
			struct stat buf;
			
			//If path is not included, error
			if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
				fprintf(stderr, "ERROR: <PATH> is not included\n");
				help(CMD_TREE);
				return -1;
			}

			//If path is too long, print error and return
            if(strlen(parameter->filename) > FILE_MAX){
                fprintf(stderr, "ERROR: Path is too long!\n");
                help(CMD_TREE);
                return -1;
            }
			  
			free(full_path);
			full_path = NULL;
			//Convert relative path to full path
			if((full_path = realpath2(parameter->filename)) == NULL) {
				fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
				return -1;
			}
			
			//Not normal directory
			if(Path_Type(full_path) == -1){
			    	fprintf(stderr, "ERROR: '%s' is wrong path\n", parameter->filename);
					help(CMD_TREE);
			    	return -1;
			}

			
			//Check lstat on path
			if (lstat(full_path, &buf) < 0) {
				fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
				return -1;
			}

			    
			// Path length is bigger than 4096 bytes -> error
			if(strlen(full_path) > PATHMAX){
			    	fprintf(stderr, "ERROR: <PATH> is too long\n");
					help(CMD_TREE);
				return -1;
			}
			    
			
			optind = 0;
			//Check if options are okay
			while((option = getopt(argcnt, arglist, "sp")) != -1) {
				if(option != 's' && option != 'p') {
					help(CMD_TREE);
					return -1;
				}
				switch(option){
					case 's':
						parameter->commandopt |= OPT_S;
						break;
					case 'p':
						parameter->commandopt |= OPT_P;
						break;
					default:
						help(CMD_TREE);
						return -1;
				}
			}
			//If no access to path exit (e.g. .repo or root)
			if(check_path_access(full_path, parameter->commandopt) == -1) {
				return -1;
			}
			
			break;
		} case CMD_ARRANGE: {


			struct stat buf;
			int optcnt = 0;

			//getopt에서 사용되는 변수들 초기화
		  	optind = 0;
			
			 //If path is not included, error
            if(parameter->filename == NULL || !strcmp(parameter->filename,"")) {
                help(CMD_ARRANGE);
                return -1;
            }
			//입력받은 경로가 255바이트를 초과하면 에러처리
			if(strlen(parameter->filename) > FILE_MAX){
				fprintf(stderr, "ERROR : %s is too Long\n", parameter->filename);				
				return -1;
			}

			free(full_path);
            full_path = NULL;
            //Convert relative path to full path
            if((full_path = realpath2(parameter->filename)) == NULL) {
                fprintf(stderr, " %s is wrong path\n", parameter->filename);
                return -1;
            }

			if (access(full_path, F_OK) == -1) {
				fprintf(stderr, "%s does not exist\n", parameter->filename);
				return -1;
			}

			   //Not normal directory
            if(Path_Type(full_path) == -1){
                    fprintf(stderr, "'%s' is not a directory\n", parameter->filename);
                    return -1;
            }


            //Check lstat on path
            if (lstat(full_path, &buf) < 0) {
                fprintf(stderr, "ERROR: lstat error for %s\n", parameter->filename);
                return -1;
            }
			  // Path length is bigger than 4096 bytes -> error
            if(strlen(full_path) > PATHMAX){
                    fprintf(stderr, "ERROR: <PATH> is too long\n");
                return -1;
            }
			while((option = getopt(argcnt, arglist, "d:t:x:e:")) != -1) {
				if(option != 'd' && option != 't' && option != 'x' && option != 'e') {
					help(CMD_ARRANGE);
					return -1;
				}
				switch(option){
					case 'd':

						 parameter->commandopt |= OPT_D;
						 parameter->outdir = strdup(optarg);
						break;
					case 'x': 
						parameter->commandopt |= OPT_X;
						parameter->excludes = strdup(optarg);
						break;
							  
					case 'e':
						parameter->commandopt |= OPT_E;
						parameter->extensions = strdup(optarg);
						break;
							 
					case 't':
							   parameter->commandopt |= OPT_T;
						//If there is a character that is not a digit, print error and return
						for(int i=0; optarg[i] != '\0'; i++){
							if(!isdigit(optarg[i])){
								fprintf(stderr, "Error: option for '-t' must be a natural number!\n");
								return -1;
							}
						}
						parameter->time = atoi(optarg);
		                if (parameter->time <= 0) {
							fprintf(stderr, "Error: option for '-t' must be a natural number!\n");
							return -1;
						}
						//If number is <= 0, print error and return
						break;
					default:
				        help(CMD_ARRANGE);
			            return -1;
    
				} 

			}
			//If no access to path exit (e.g. .repo or root)
            if(check_path_access(full_path, parameter->commandopt) == -1) {
                return -1;
            }
			break;
		}
		return 1; 
	}
	
}

// In the child process, excute given command to the process
void CommandFun(char **arglist) {
  int (*commandFun)(command_parameter * parameter);
  command_parameter parameter={arglist[0], arglist[1], atoi(arglist[2]), atoi(arglist[3]), arglist[4], arglist[5], arglist[6] };
  if(!strcmp(parameter.command, commanddata[0])) {
    commandFun = tree_process;
  } else if(!strcmp(parameter.command, commanddata[1])) {
    commandFun = arrange_process;
  } else if(!strcmp(parameter.command, commanddata[2])) {
    commandFun = help_process;
  }

  if(commandFun(&parameter) != 0) {
    exit(1);
  }

}

//Add "command" to parameter -> indicating that we will run a command on the child process
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
  parameter.argv[9] = (char *)0;

  //Create child process and execute command
  if((pid = fork()) < 0) {
    fprintf(stderr, "ERROR: fork error\n");
    exit(1);
  } else if(pid == 0) {
    execv(exeNAME, parameter.argv);
    exit(0);
  } else {
    pid = wait(NULL);
  }
}

// This is executed in the parent process
void Prompt() {
  char input[STRMAX*2];
  int argcnt = 0;
  char **arglist = NULL;
  int command;
  int option;
  command_parameter parameter={(char *)0, (char *)0, 0};

  //Continue to print prompt until exit
  while(true) {
    arglist = NULL;
    argcnt = 0;
    printf("%d> ", STUDENT_NUM);

    fgets(input, sizeof(input), stdin);

    input[strlen(input) - 1] = '\0';

    arglist = GetSubstring(input, &argcnt, " ");

    //If empty prompt, print the prompt again
    if(argcnt == 0){
      continue;
    }

    //If command is correct, set command. If command was incorrect, print help usage
    if (strcmp(arglist[0], "tree") == 0) {
    //check options using getopt
        command = CMD_TREE;
    } else if (strcmp(arglist[0], "arrange") == 0) {
        command = CMD_ARRANGE;
    } else if (strcmp(arglist[0], "help") == 0) {
        command = CMD_HELP;
    } else if(!strcmp(arglist[0], "exit")) {
		command = CMD_EXIT;
    	exit(0);
    } else {
	command = NOT_CMD;
    }
  

    //Run ComandExec if correct command
    if(command & (CMD_TREE | CMD_ARRANGE | CMD_HELP)) {
      ParameterInit(&parameter);
      parameter.command = arglist[0];
      parameter.filename = (argcnt > 1) ? arglist[1] : NULL;

      if(ParameterProcessing(argcnt, arglist, command, &parameter) != -1) {
       	CommandExec(parameter);
      }
    }
    //Run help if command was help or incorrect command
    else if(command == NOT_CMD) {
      help(0);
    }

    printf("\n");
  }
}


int main(int argc, char* argv[]) {
	
	getcwd(exePATH, PATHMAX);
	sprintf(homePATH, "%s", getenv("HOME"));
	sprintf(pwd_path, "%s", exePATH);
  //Executed in child process with command
	
  if(!strcmp(argv[0], "command")) {
    CommandFun(argv+2);
  }
  //Execute in parent process
  else {
    strcpy(exeNAME, argv[0]);

    Prompt();
  }

  exit(0);
}

