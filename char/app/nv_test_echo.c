// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_char_echo.ko (/dev/nv_echo)
 *
 *   sudo insmod ../nv_char_echo.ko
 *   ./bin/nv_test_echo "hello"
 *   sudo rmmod nv_char_echo
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEV_PATH	"/dev/nv_echo"

int main(int argc, char **argv)
{
	const char *msg = (argc > 1) ? argv[1] : "hello nv_echo";
	char buf[4096];
	ssize_t wn, rn;
	int fd;

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s (load nv_char_echo.ko first)\n",
			DEV_PATH, strerror(errno));
		return 1;
	}

	wn = write(fd, msg, strlen(msg));
	if (wn < 0) {
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

	if (rn < 0) {
		perror("read");
		return 1;
	}

	if ((size_t)rn != strlen(msg) || memcmp(buf, msg, rn) != 0) {
		fprintf(stderr, "FAIL: expected \"%s\", got \"%.*s\"\n",
			msg, (int)rn, buf);
		return 1;
	}

	printf("PASS: wrote %zd bytes, read back \"%s\"\n", wn, buf);
	return 0;
}
