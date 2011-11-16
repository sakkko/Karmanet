#ifndef _PEERRATE_H_
#define _PEERRATE_H_

#include <sys/sysinfo.h>
#include <stdio.h>

#include "ioutil.h"

#define PRECISION 10010
#define CPUFREQ_FLAG "cpu MHz"

long start_time;

unsigned long get_peer_rate();

unsigned long get_cpu_freq();

#endif
