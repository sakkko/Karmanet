#ifndef _REQUEST_FIFO_CHECKER_H
#define _REQUEST_FIFO_CHECKER_H

#define RF_CHECK_TIME 6


#include "thread_util.h"
#include "fifo_request.h"

struct request_fifo_ch_info {
	struct th_info thinfo;
};

int request_fifo_checker_run(struct request_fifo_ch_info *rfchinfo);

int request_fifo_checker_stop(struct request_fifo_ch_info *rfchinfo);

void request_fifo_checker_func(void *args);


#endif
