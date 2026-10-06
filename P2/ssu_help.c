#include "ssu_header.h"

//Pass cmd_bit and print accordingly to the bit
void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }
  if(!cmd_bit || cmd_bit & CMD_SHOW) {
    printf("%s show \n    <none> : show monitoring deamon process info \n ", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_ADD) {
    printf("%s add <DIR_PATH> [OPTION]...\n    <none> : add deamon process monitoring the <DIR_PATH> directory\n    -d  <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n    -i  <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n    -l  <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record\n    -x  <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n    -e  <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n    -m  <M> : Specify the value for the <M> option.\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_MODIFY) {
    printf("%s modify <DIR_PATH> [OPTION]...\n    <none> : modify deamon process config monitoring the <DIR_PATH> directory\n    -d  <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n    -i  <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n    -l  <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record\n    -x  <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n    -e  <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n    -m  <M> : Specify the value for the <M> option.\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_REMOVE) {
    printf("%s remove <DIR_PATH> \n    <none> : remove deamon process monitoring the <DIR_PATH> directory\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help \n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_EXIT) {
    printf("%s exit : exit program\n", (cmd_bit?"Usage:":"  >"));
  }
}

//Function that is called from ssu_cleanup when command is help
int help_process(command_parameter *parameter) {
  //If no second param for help, print all helps. Else print specified help command.
  if(parameter->filename == NULL) {
    help(0);
  } else if(!strcmp(parameter->filename, "show")) {
    help(CMD_SHOW);
  } else if(!strcmp(parameter->filename, "add")) {
    help(CMD_ADD);
  } else if(!strcmp(parameter->filename, "modify")) {
    help(CMD_MODIFY);
  } else if(!strcmp(parameter->filename, "remove")) {
    help(CMD_REMOVE);
  } else if(!strcmp(parameter->filename, "help")) {
    help(CMD_HELP);
  } else if(!strcmp(parameter->filename, "exit")) {
    help(CMD_EXIT);
  } else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", parameter->filename);
    return -1;
  }

  return 0;
}

