#include "ssu_header.h"


//Init Function for parameter
void ParameterInit(command_parameter *parameter) {
  parameter->command = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->filename = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->commandopt = 0;
  parameter->time = 1;

  //for arrange options
  parameter->outdir = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->extensions = (char *)malloc(sizeof(char *) * PATH_MAX);
  parameter->excludes = (char *)malloc(sizeof(char *) * PATH_MAX);

}

//Function to return substring of a string
char *substr(char *str, int beg, int end) {
  char *ret = (char*)malloc(sizeof(char) * (end-beg+1));

  for(int i = beg; i < end && *(str+i) != '\0'; i++) {
    ret[i-beg] = str[i];
  }
  ret[end-beg] = '\0';
	
  return ret;
}

//Return substr
char *c_str(char *str) {
   return substr(str, 0, strlen(str));
}

//Function to convert a path to a realpath
char *realpath2(char* path) {
  pathNode *path_head;
  pathNode *curr_path;
  char *ptr;
  char *origin_path = (char *)malloc(sizeof(char *) * PATHMAX);
  char *ret_path = (char *)malloc(sizeof(char *) * PATHMAX);
  strcpy(ret_path, "");
  path_head = (pathNode*)malloc(sizeof(pathNode));
  path_head->depth = 0;
  path_head->tail_path = path_head;
  path_head->head_path = path_head;
  path_head->next_path = NULL;
  if(path[0] != '/') {
    strcat(origin_path, pwd_path);
    strcat(origin_path, "/");
    strcat(origin_path, path);
  } else {
    strcpy(origin_path, path);
  }
  if(path_list_init(path_head, origin_path) == -1) {
    return NULL;
  }
  curr_path = path_head->next_path;
  while(curr_path != NULL) {
    strcat(ret_path, curr_path->path_name);
    if(curr_path->next_path != NULL) {
      strcat(ret_path, "/");
    }
    curr_path = curr_path->next_path;
  }
  if(strlen(ret_path) == 0) {
    strcpy(ret_path, "/");
  }
  return c_str(ret_path);
}


char *QuoteCheck(char **str, char del) {
  char *tmp = *str+1;
  int i = 0;

  while(*tmp != '\0' && *tmp != del) {
    tmp++;
    i++;
  }
  if(*tmp == '\0') {
    *str = tmp;
    return NULL;
  }
  if(*tmp == del) {
    for(char *c = *str; *c != '\0'; c++) {
      *c = *(c+1);
    }
    *str += i;
    for(char *c = *str; *c != '\0'; c++) {
      *c = *(c+1);
    }
  }
}

//Return token of string
char *Tokenize(char *str, char *del) {
  int i = 0;
  int del_len = strlen(del);
  static char *tmp = NULL;
  char *tmp2 = NULL;

  if(str != NULL && tmp == NULL) {
    tmp = str;
  }

  if(str == NULL && tmp == NULL) {
    return NULL;
  }

  char *idx = tmp;

  while(i < del_len) {
    if(*idx == del[i]) {
      idx++;
      i = 0;
    } else {
      i++;
    }
  }
  if(*idx == '\0') {
    tmp = NULL;
    return tmp;
  }
  tmp = idx;

  while(*tmp != '\0') {
    if(*tmp == '\'' || *tmp == '\"') {
      QuoteCheck(&tmp, *tmp);
      continue;
    }
    for(i = 0; i < del_len; i++) {
      if(*tmp == del[i]) {
        *tmp = '\0';
        break;
      }
    }
    tmp++;
    if(i < del_len) {
      break;
    }
  }

  return idx;
}


//Get sub string
char **GetSubstring(char *str, int *cnt, char *del) {
  *cnt = 0;
  int i = 0;
  char *token = NULL;
  char *templist[100] = {NULL, };
  token = Tokenize(str, del);
  if(token == NULL) {
    return NULL;
  }

  while(token != NULL) {
    templist[*cnt] = token;
    *cnt += 1;
    token = Tokenize(NULL, del);
  }

	char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
	for (i = 0; i < *cnt; i++) {
		temp[i] = templist[i];
	}
	return temp;
}
// Used in cvt_path_2_realpath, init path list
int path_list_init(pathNode *curr_path, char *path) {
  pathNode *new_path = curr_path;
  char *ptr;
  char *next_path = "";

  if(!strcmp(path, "")) return 0;

  if(ptr = strchr(path, '/')) {
    next_path = ptr+1;
    ptr[0] = '\0';
  }

  if(!strcmp(path, "..")) {
    new_path = curr_path->prev_path;
    new_path->tail_path = new_path;
    new_path->next_path = NULL;
    new_path->head_path->tail_path = new_path;

    new_path->head_path->depth--;

    if(new_path->head_path->depth == 0) return -1;
  } else if(strcmp(path, ".")) {
    new_path = (pathNode*)malloc(sizeof(pathNode));
    strcpy(new_path->path_name, path);

    new_path->head_path = curr_path->head_path;
    new_path->tail_path = new_path;

    new_path->prev_path = curr_path;
    new_path->next_path = curr_path->next_path;

    curr_path->next_path = new_path;
    new_path->head_path->tail_path = new_path;

    new_path->head_path->depth++;
  }

  if(strcmp(next_path, "")) {
    return path_list_init(new_path, next_path);
  }

  return 0;
}

//Function to check path accessability
int check_path_access(char* path, int opt) {
  char *origin_path = (char*)malloc(sizeof(char)*(strlen(path)+1));
  char* ptr;
  int depth = 0;

  strcpy(origin_path, path);

  while(ptr = strchr(origin_path, '/')) {
    char *tmp_path = substr(origin_path, 0, strlen(origin_path) - strlen(ptr));

    depth++;
    origin_path = ptr+1;

    //While path is being cut down, if it never contains /home/, return -1
    if(depth == 2 && strcmp(substr(path, 0, strlen(path) - strlen(origin_path)), "/home/")) {
      fprintf(stderr, "ERROR: path must be in user directory\n");
      fprintf(stderr, " - '%s' is outside the home directory\n", path);
      return -1;
    }
  }

  if(!strcmp(path, "/home")){
      fprintf(stderr, "ERROR: path must be in user directory\n");
      return -1;
  }


  if(depth == 2 && (opt & OPT_S) && !strcmp(path, homePATH)){
  	return 0;
  }
  //If depth is too small (out of user dir) return -1
  else if(depth < 3) {
    fprintf(stderr, "ERROR: path must be in user directory\n");
    fprintf(stderr, " - '%s' is outside the home directory\n", path);
    return -1;
  }

  //Else return 0
  return 0;
}



//Check if path is  directory, or etc
int Path_Type(const char* path) {
	struct stat path_stat;
	//Wrong type or doesnt exist
	if(stat(path, &path_stat) != 0) return -1;


	//Directory
	if(S_ISDIR(path_stat.st_mode)) return 1;

	return -1;
}

