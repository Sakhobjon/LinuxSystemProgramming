#include "ssu_header.h"

//Pass cmd_bit and print accordingly to the bit
void help(int cmd_bit) {
  if(!cmd_bit) {
    printf("Usage: \n");
  }

  if(!cmd_bit || cmd_bit & CMD_TREE) {
    printf("%s tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory\n    -r : display the directory structure recursively if <PATH> is a directory\n    -s : display the directory structure if <PATH> is a directory, including the size of each file\n    -p : display the directory structure if <PATH> is a directory, including the permissions of each directory and file\n", (cmd_bit?"Usage:":"  >"));
  }

  if(!cmd_bit || cmd_bit & CMD_PRINT) {
    printf("%s print <PATH> [OPTION]... : print the contents on the standard output if <PATH> is file\n    -n <line_number> : print only the first <line_number> lines of its contents on the standard output if <PATH> is file\n", (cmd_bit?"Usage:":"  >"));
  }
  if(!cmd_bit || cmd_bit & CMD_HELP) {
    printf("%s help [COMMAND] : show commands for program\n", (cmd_bit?"Usage:":"  >"));
  }
  
  if(!cmd_bit || cmd_bit & CMD_EXIT) {
    printf("%s exit : exit program\n", (cmd_bit?"Usage:":"  >"));
  }
}

