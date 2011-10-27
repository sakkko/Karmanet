#ifndef _SHARED_H
#define _SHARED_H

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <openssl/md5.h>

#include "ioutil.h"
#include "md5_util.h"

#define FILE_SHARE "../.karma.share"
#define FILE_DIFF "../.karma.diff"
#define FILE_TMP "../.karma.share.tmp"


int selall(const struct dirent *dirname);

int selfile(const struct dirent *dirname);

int seldir(const struct dirent *dirname);

int write_dir(int fd, int modtime, const char *dir);

int write_file(int fd, int modtime, const unsigned char *md5, const char *filename);

int write_diff(int fd, char flag, const unsigned char *md5, const char *filename);

int add_shared_file(int fd_share, int fd_diff, const struct dirent *file, const char *share_dir);

int add_shared_files(int fd_share, int fd_diff, struct dirent **flist, int start_indx, int nfile, const char *dir);

int add_dir(int fdnew, int fddiff, const char *share_dir);

int update_file(int fdnew, int fddiff, time_t modtime, const char *fpath, const char *fname, const unsigned char *old_md5);

int remove_dir(int fd_share, int fd_diff);

int remove_all_files(int fd_share, int fd_diff);

int update_dir(int fd_share, int fdnew, int fddiff, const char *share_dir);

int copy_dir(int fd_share, int fdnew);

int diff(int fd_share, int fdnew, int fddiff, const char *share_dir, int flag);

int create_diff(const char *share_dir);

int get_filepath(char *filepath, char *filename, const unsigned char *md5);

#endif
