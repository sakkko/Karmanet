#ifndef _CHECKER_H
#define _CHECKER_H

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "thread_util.h"

#define TIME_CHECK_FLAG 12

struct sp_checker_info {
	struct th_info thinfo;
	int sp_checker_pipe;
	int sp_flag;
};

int sp_checker_run(struct sp_checker_info *chinfo);

int sp_checker_stop(struct sp_checker_info *chinfo);

void sp_checker_func(void *args);

int update_sp_flag(struct sp_checker_info *chinfo);

#endif
