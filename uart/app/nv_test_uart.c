// SPDX-License-Identifier: GPL-2.0
/*
 * User-space loopback test for nv_uart_demo (/dev/ttyNV0)
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define DEV_PATH	"/dev/ttyNV0"
#define TEST_MSG	"hello nv_uart\n"

static int setup_tty(int fd)
{
	struct termios tio;

	if (tcgetattr(fd, &tio) < 0)
		return -1;

	cfmakeraw(&tio);
	tio.c_cflag |= (CLOCAL | CREAD);
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;
	tio.c_cflag &= ~PARENB;
	tio.c_cflag &= ~CSTOPB;
	cfsetispeed(&tio, B115200);
	cfsetospeed(&tio, B115200);
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 50; /* 5 s */

	if (tcsetattr(fd, TCSANOW, &tio) < 0)
		return -1;

	tcflush(fd, TCIOFLUSH);
	return 0;
}

int main(int argc, char **argv)
{
	const char *dev = (argc > 1) ? argv[1] : DEV_PATH;
	char buf[256];
	ssize_t wn, rn;
	int fd;

	(void)argc;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "open %s: %s (insmod nv_uart_demo.ko first)\n",
			dev, strerror(errno));
		return 1;
	}

	if (setup_tty(fd) < 0) {
		perror("tcsetattr");
		close(fd);
		return 1;
	}

	/* switch to blocking for read */
	fcntl(fd, F_SETFL, 0);

	wn = write(fd, TEST_MSG, strlen(TEST_MSG));
	if (wn < 0) {
		perror("write");
		close(fd);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	rn = read(fd, buf, sizeof(buf) - 1);
	if (rn < 0) {
		perror("read");
		close(fd);
		return 1;
	}

	close(fd);

	if (rn != wn || memcmp(buf, TEST_MSG, wn) != 0) {
		fprintf(stderr, "FAIL: wrote %zd, read %zd\n", wn, rn);
		fprintf(stderr, "got: %.*s\n", (int)rn, buf);
		return 1;
	}

	printf("PASS: loopback \"%s\"\n", buf);
	return 0;
}
