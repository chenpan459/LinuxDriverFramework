// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_spi_demo via sysfs (reg attribute)
 *
 *   ./bin/nv_test_spi [sysfs_device_dir]
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSFS_SPI	"/sys/bus/spi/devices"
#define MODALIAS	"spi:nv_spi_demo"
#define TEST_VAL	0x5a

static int read_file(const char *path, char *buf, size_t len)
{
	FILE *f;

	f = fopen(path, "r");
	if (!f)
		return -1;
	if (!fgets(buf, len, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static int find_demo_device(char *out, size_t outlen)
{
	DIR *dir;
	struct dirent *ent;
	char path[512];
	char buf[128];

	dir = opendir(SYSFS_SPI);
	if (!dir)
		return -1;

	while ((ent = readdir(dir)) != NULL) {
		if (ent->d_name[0] == '.')
			continue;

		snprintf(path, sizeof(path), "%s/%s/modalias",
			 SYSFS_SPI, ent->d_name);
		if (read_file(path, buf, sizeof(buf)) < 0)
			continue;

		if (strncmp(buf, MODALIAS, strlen(MODALIAS)) == 0) {
			snprintf(out, outlen, "%s/%s", SYSFS_SPI, ent->d_name);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);
	return -1;
}

static int write_reg(const char *devdir, unsigned int val)
{
	char path[520];
	FILE *f;

	snprintf(path, sizeof(path), "%s/reg", devdir);
	f = fopen(path, "w");
	if (!f) {
		perror(path);
		return -1;
	}
	fprintf(f, "0x%x\n", val);
	fclose(f);
	return 0;
}

static int read_reg(const char *devdir, unsigned int *val)
{
	char path[520];
	char buf[32];
	unsigned int v;

	snprintf(path, sizeof(path), "%s/reg", devdir);
	if (read_file(path, buf, sizeof(buf)) < 0) {
		perror(path);
		return -1;
	}
	if (sscanf(buf, "%i", &v) != 1)
		return -1;
	*val = v;
	return 0;
}

int main(int argc, char **argv)
{
	char devdir[512];
	unsigned int val;

	if (argc > 1) {
		strncpy(devdir, argv[1], sizeof(devdir) - 1);
		devdir[sizeof(devdir) - 1] = '\0';
	} else if (find_demo_device(devdir, sizeof(devdir)) < 0) {
		fprintf(stderr,
			"No %s device under %s\n"
			"  insmod driver, bind DT node compatible \"nv,spi-demo\"\n"
			"  or use: sudo ./bin/nv_test_spi_sysfs\n",
			MODALIAS, SYSFS_SPI);
		return 1;
	}

	printf("Testing %s/reg\n", devdir);

	if (write_reg(devdir, TEST_VAL) < 0)
		return 1;

	if (read_reg(devdir, &val) < 0)
		return 1;

	if (val != TEST_VAL) {
		fprintf(stderr, "FAIL: wrote 0x%x, read 0x%x\n", TEST_VAL, val);
		return 1;
	}

	printf("PASS: sysfs reg 0x%02x\n", val);
	return 0;
}
