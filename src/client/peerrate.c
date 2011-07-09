#include "peerrate.h"

long get_peer_rate() {
	struct sysinfo sinfo;
	if (sysinfo(&sinfo) < 0) {
		return -1;
	}
	printf("=========== %d =============\n", PRECISION);
	printf("UPTIME = %ld\nTOTALRAM(B) = %lu\nFREERAM(B) = %lu\nMEM-UNIT = %d\n", sinfo.uptime, 
			sinfo.totalram * sinfo.mem_unit, sinfo.freeram * sinfo.mem_unit, sinfo.mem_unit);
	return sinfo.uptime + (sinfo.totalram * sinfo.mem_unit / PRECISION);

}
