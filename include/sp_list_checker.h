#ifndef _SP_LIST_CHECKER_H
#define _SP_LIST_CHECKER_H

#include <sys/time.h>
#include <time.h>

#include "list.h"
#include "sp_list.h"
#include "thread_util.h"

#define SPL_CHECK_TIME 6

struct sp_list_checker_info {
	struct th_info thinfo;
	struct node **sp_list;
};

int sp_list_checker_run(struct sp_list_checker_info *splchinfo);

int sp_list_checker_stop(struct sp_list_checker_info *splchinfo);

void sp_list_checker_func(void *args);

int update_sp_flag(struct sp_list_checker_info *splchinfo, const struct sockaddr_in *sp_addr);

#endif
