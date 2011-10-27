#ifndef _TRANSFER_H
#define _TRANSFER_H

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#include "inetutil.h"
#include "packet_util.h"
#include "transfer_list.h"
#include "thread_util.h"
#include "shared.h"

#define TIME_TO_SLEEP 1

#define CHUNK_SIZE 16384
#define DOWNLOAD_DIR "../download"
#define PART_DIR "../download/part"

#define INFO_STR_LEN MD5_DIGEST_LENGTH + sizeof(int) * 3

#endif
