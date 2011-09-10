#ifndef _LISTA_FILE_H
#define _LISTA_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h> 

#include "ioutil.h"

int write_share_file(int fd_share, const char *dirname);

void share_file(char *dir_path, char *file_log);

#endif
