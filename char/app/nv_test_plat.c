// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_char_plat.ko (/dev/nv_plat)
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEV_PATH	"/dev/nv_plat"

int main(int argc, char **argv)
{
	const char *msg = (argc > 1) ? argv[1] : "hello nv_plat";
	char buf[4096];
	int fd;
	ssize_t rn;

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

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		close(fd);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	rn = read(fd, buf, sizeof(buf) - 1);
	close(fd);

	if (rn < 0 || memcmp(buf, msg, strlen(msg)) != 0) {
		fprintf(stderr, "FAIL: read mismatch\n");
		return 1;
	}

	printf("PASS: plat echo \"%s\"\n", buf);
	return 0;
}
