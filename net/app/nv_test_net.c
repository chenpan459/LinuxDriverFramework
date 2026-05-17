// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_net_dummy.ko (interface nv0)
 *
 *   sudo insmod ../nv_net_dummy.ko
 *   ./bin/nv_test_net
 */

#include <errno.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define IFNAME	"nv0"

int main(void)
{
	struct ifreq ifr;
	int sock, ret = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, IFNAME, IFNAMSIZ - 1);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		fprintf(stderr, "ioctl SIOCGIFFLAGS on %s: %s\n",
			IFNAME, strerror(errno));
		fprintf(stderr, "load nv_net_dummy.ko first\n");
		close(sock);
		return 1;
	}

	printf("PASS: interface %s exists (flags=0x%x)\n",
	       IFNAME, (unsigned)ifr.ifr_flags);

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
		unsigned char *m = (unsigned char *)ifr.ifr_hwaddr.sa_data;

		printf("       MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		       m[0], m[1], m[2], m[3], m[4], m[5]);
	}

	close(sock);
	return ret;
}
