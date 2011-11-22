#ifndef _PEERRATE_H_
#define _PEERRATE_H_

#include <sys/sysinfo.h>
#include <stdio.h>

#include "ioutil.h"

#define PRECISION (1024 * 1024) 
#define RAM_LIMIT (4 * 1024) 
#define CPU_LIMIT 4000
#define CPUFREQ_FLAG "cpu MHz"



long start_time;

long hw_rate;

unsigned long get_peer_rate();

unsigned long get_cpu_freq();

#endif
