// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_block_ram.ko (/dev/nv_ram)
 *
 *   sudo insmod ../nv_block_ram.ko
 *   ./bin/nv_test_ram
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEV_PATH	"/dev/nv_ram"
#define SECTOR_SIZE	512
#define TEST_SECTORS	8

int main(void)
{
	unsigned char wbuf[SECTOR_SIZE * TEST_SECTORS];
	unsigned char rbuf[SECTOR_SIZE * TEST_SECTORS];
	int fd, i;
	ssize_t n;

	for (i = 0; i < (int)sizeof(wbuf); i++)
		wbuf[i] = (unsigned char)(i & 0xff);

	fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s\n", DEV_PATH, strerror(errno));
		return 1;
	}

	n = write(fd, wbuf, sizeof(wbuf));
	if (n != (ssize_t)sizeof(wbuf)) {
		fprintf(stderr, "write failed: %zd (%s)\n", n, strerror(errno));
		close(fd);
		return 1;
	}

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		close(fd);
		return 1;
	}

	memset(rbuf, 0, sizeof(rbuf));
	n = read(fd, rbuf, sizeof(rbuf));
	close(fd);

	if (n != (ssize_t)sizeof(rbuf)) {
		fprintf(stderr, "read failed: %zd\n", n);
		return 1;
	}

	if (memcmp(wbuf, rbuf, sizeof(wbuf)) != 0) {
		fprintf(stderr, "FAIL: data mismatch\n");
		return 1;
	}

	printf("PASS: wrote/read %zd bytes on %s\n", n, DEV_PATH);
	return 0;
}
