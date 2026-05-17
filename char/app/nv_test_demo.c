// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_char_demo.ko (/dev/nv_demo, poll + ioctl)
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEV_PATH	"/dev/nv_demo"

#define NV_DEMO_IOC_MAGIC	'v'
#define NV_DEMO_IOC_GET_LEN	_IOR(NV_DEMO_IOC_MAGIC, 0x01, int)
#define NV_DEMO_IOC_CLEAR	_IO(NV_DEMO_IOC_MAGIC, 0x02)

int main(int argc, char **argv)
{
	const char *msg = (argc > 1) ? argv[1] : "hello nv_demo";
	char buf[256];
	int fd, len = -1;
	ssize_t rn;

	(void)argv;

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	if (write(fd, msg, strlen(msg)) < 0) {
		perror("write");
		close(fd);
		return 1;
	}

	if (ioctl(fd, NV_DEMO_IOC_GET_LEN, &len) < 0) {
		perror("ioctl GET_LEN");
		close(fd);
		return 1;
	}

	if (len != (int)strlen(msg)) {
		fprintf(stderr, "FAIL: ioctl len=%d, expected %zu\n",
			len, strlen(msg));
		close(fd);
		return 1;
	}

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		close(fd);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	rn = read(fd, buf, sizeof(buf) - 1);
	if (rn < 0 || (size_t)rn != strlen(msg)) {
		fprintf(stderr, "FAIL: read %zd bytes\n", rn);
		close(fd);
		return 1;
	}

	if (ioctl(fd, NV_DEMO_IOC_CLEAR) < 0) {
		perror("ioctl CLEAR");
		close(fd);
		return 1;
	}

	len = 99;
	if (ioctl(fd, NV_DEMO_IOC_GET_LEN, &len) == 0 && len != 0) {
		fprintf(stderr, "FAIL: len after clear=%d\n", len);
		close(fd);
		return 1;
	}

	close(fd);
	printf("PASS: demo read/write/ioctl ok (\"%s\")\n", buf);
	return 0;
}
