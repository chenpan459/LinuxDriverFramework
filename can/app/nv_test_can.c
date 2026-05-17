// SPDX-License-Identifier: GPL-2.0
/*
 * User-space SocketCAN test for nv_can_demo (nvcan0 loopback)
 */

#include <errno.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define IFNAME		"nvcan0"
#define TEST_ID		0x123
#define TEST_DATA	"hello"

int main(int argc, char **argv)
{
	const char *ifname = (argc > 1) ? argv[1] : IFNAME;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame tx, rx;
	int s, ifindex, ret;
	socklen_t len = sizeof(rx);

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0) {
		perror("socket");
		return 1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		fprintf(stderr, "ioctl SIOCGIFINDEX on %s: %s\n",
			ifname, strerror(errno));
		fprintf(stderr, "  sudo insmod driver; sudo ip link set %s up type can bitrate 1000000\n",
			ifname);
		close(s);
		return 1;
	}

	ifindex = ifr.ifr_ifindex;
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(s);
		return 1;
	}

	memset(&tx, 0, sizeof(tx));
	tx.can_id = TEST_ID;
	tx.len = strlen(TEST_DATA);
	memcpy(tx.data, TEST_DATA, tx.len);

	ret = write(s, &tx, sizeof(tx));
	if (ret != (int)sizeof(tx)) {
		perror("write");
		close(s);
		return 1;
	}

	memset(&rx, 0, sizeof(rx));
	ret = recvfrom(s, &rx, sizeof(rx), 0, NULL, &len);
	if (ret < 0) {
		perror("read");
		close(s);
		return 1;
	}

	close(s);

	if (rx.can_id != TEST_ID || rx.len != tx.len ||
	    memcmp(rx.data, tx.data, tx.len) != 0) {
		fprintf(stderr, "FAIL: id=0x%x len=%u data mismatch\n",
			rx.can_id, rx.len);
		return 1;
	}

	printf("PASS: CAN loopback id=0x%x len=%u \"%.*s\"\n",
	       rx.can_id, rx.len, rx.len, rx.data);
	return 0;
}
