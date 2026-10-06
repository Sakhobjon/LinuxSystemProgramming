#include "ssu_header.h"

//Pass cmd_bit and print accordingly to the bit
void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }

  if(!cmd_bit || cmd_bit & CMD_TREE) {
    printf("%s tree <DIR_PATH> [OPTION]...\n    <none> : Display the directory structure recursively if <DIR_PATH> is a directory\n    -s : Display the directory structure recursively if <DIR_PATH> is a directory, including the size of each file\n    -p : Display the directory structure recursively if <DIR_PATH> is a directory, including the permissions of each directory and file\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_ARRANGE) {
    printf("%s arrange <DIR_PATH> [OPTION]...\n    <none> : Arrange the directory if <DIR_PATH> is a directory\n    -d <output_path> : Specify the output directory <output_path> where <DIR_PATH> will be arranged if <DIR_PATH is a directory\n    -t <seconds> : Only arrnage files that were modified more than <seconds> seconds ago\n    -x <exlude_path1, exclude_path2, ...> : Arrange the directory if <DIR_PATH> is a directory except for the files inside <exclude_path> directory\n    -e <extension1, extention2, ...> : Arrange the directory with the specified extension <extension1, extension2 ... >\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help [COMMAND] \n", (cmd_bit?"Usage:":"  >"));
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
  } else if(!strcmp(parameter->filename, "tree")) {
    help(CMD_TREE);
  } else if(!strcmp(parameter->filename, "arrange")) {
    help(CMD_ARRANGE);
  }  else if(!strcmp(parameter->filename, "help")) {
    help(CMD_HELP);
  } else if(!strcmp(parameter->filename, "exit")) {
    help(CMD_EXIT);
  } else {
    fprintf(stderr, "ERROR: invalid command -- '%s'\n", parameter->filename);
    return -1;
  }

  return 0;
}

