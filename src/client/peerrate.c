#include "peerrate.h"

unsigned long get_peer_rate() {
	struct sysinfo sinfo;
	unsigned long cpu_freq;
	unsigned long ram;
	unsigned long uptime;

	if (sysinfo(&sinfo) < 0) {
		return 0;
	}
	if(hw_rate == 0){
		cpu_freq = get_cpu_freq();
		ram = (sinfo.totalram * sinfo.mem_unit / PRECISION);
		if (ram > RAM_LIMIT) {
			ram = RAM_LIMIT;
		}
		if (cpu_freq > CPU_LIMIT) {
			cpu_freq = CPU_LIMIT;
		}
			
		hw_rate = ram + cpu_freq;
		printf("UPTIME = %ld\nTOTALRAM(B) = %lu\nFREERAM(B) = %lu\nMEM-UNIT = %d\nCPU-FREQ = %ld\n", sinfo.uptime-start_time, sinfo.totalram * sinfo.mem_unit, sinfo.freeram * sinfo.mem_unit, sinfo.mem_unit, cpu_freq);
		printf("RAM: %ld\n", ram);
		printf("CPU: %ld\n", cpu_freq);
	
	}
	if(start_time == 0){
		printf("START TIME RATE\n");
		start_time = sinfo.uptime;	
	}

	if ((sinfo.uptime - start_time) / 60 > 4800) {
		uptime = 4800;
	} else {
		uptime = (sinfo.uptime - start_time) / 60;
	}
	
	return uptime + hw_rate;

}

unsigned long get_cpu_freq() {
	int fd;
	int n, i;
	char buf[MAXLINE];
	double tmp, freq;

	if ((fd = open("/proc/cpuinfo", O_RDONLY)) < 0) {
		perror("get_cpu_freq error - open failed");
		return 0;
	}

	freq = 0;
	while ((n = readline(fd, buf, MAXLINE)) > 0) {
		if (strncmp(CPUFREQ_FLAG, buf, strlen(CPUFREQ_FLAG))) {
			continue;
		}

		buf[n - 1] = 0;
	//	printf("%s\n", buf);

		for (i = 0; i < n; i ++) {
			if (buf[i] == ':') {
				break;
			}
		}
		i ++;

		errno = 0;
		tmp = strtod(buf + i, NULL);

		if (errno == ERANGE) {
			perror("get_cpu_freq error - strtod failed");
			if (close(fd) < 0) {
				perror("get_cpu_freq error - close failed");
			}
			return 0;
		}

		freq += tmp;
	}

	if (close(fd) < 0) {
		perror("get_cpu_freq error - close failed");
	}

	return (unsigned long)freq;
}

