// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_adc_demo via IIO sysfs
 *
 *   ./bin/nv_test_adc [iio_device_dir]
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSFS_IIO	"/sys/bus/iio/devices"
#define DEV_NAME	"nv_adc_demo"
#define NUM_CH		4

static int read_file(const char *path, char *buf, size_t len)
{
	FILE *f = fopen(path, "r");

	if (!f)
		return -1;
	if (!fgets(buf, len, f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static int write_file(const char *path, const char *val)
{
	FILE *f = fopen(path, "w");

	if (!f) {
		perror(path);
		return -1;
	}
	if (fprintf(f, "%s", val) < 0) {
		perror(path);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static int find_adc_device(char *out, size_t outlen)
{
	DIR *dir;
	struct dirent *ent;
	char path[512];
	char name[64];

	dir = opendir(SYSFS_IIO);
	if (!dir)
		return -1;

	while ((ent = readdir(dir)) != NULL) {
		if (strncmp(ent->d_name, "iio:device", 10) != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s/name", SYSFS_IIO, ent->d_name);
		if (read_file(path, name, sizeof(name)) < 0)
			continue;

		if (strncmp(name, DEV_NAME, strlen(DEV_NAME)) == 0) {
			snprintf(out, outlen, "%s/%s", SYSFS_IIO, ent->d_name);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);
	return -1;
}

static int test_device(const char *devdir)
{
	char path[520];
	char buf[32];
	int ch, val, expected;

	printf("Testing %s\n", devdir);

	for (ch = 0; ch < NUM_CH; ch++) {
		snprintf(path, sizeof(path), "%s/in_voltage%d_raw", devdir, ch);
		if (read_file(path, buf, sizeof(buf)) < 0) {
			fprintf(stderr, "read %s failed\n", path);
			return -1;
		}
		val = atoi(buf);
		expected = (ch + 1) * 1000;
		if (val != expected) {
			fprintf(stderr, "FAIL: ch%d expected %d got %d\n",
				ch, expected, val);
			return -1;
		}
		printf("  in_voltage%d_raw = %d\n", ch, val);
	}

	snprintf(path, sizeof(path), "%s/in_voltage0_raw", devdir);
	if (write_file(path, "5555") < 0)
		return -1;
	if (read_file(path, buf, sizeof(buf)) < 0)
		return -1;
	if (atoi(buf) != 5555) {
		fprintf(stderr, "FAIL: write ch0 expected 5555 got %s\n", buf);
		return -1;
	}
	printf("  write/read ch0 = 5555 OK\n");

	printf("PASS: ADC channels ok\n");
	return 0;
}

int main(int argc, char **argv)
{
	char devdir[512];

	if (argc > 1) {
		strncpy(devdir, argv[1], sizeof(devdir) - 1);
		devdir[sizeof(devdir) - 1] = '\0';
	} else if (find_adc_device(devdir, sizeof(devdir)) < 0) {
		fprintf(stderr,
			"No %s in %s\n"
			"  sudo insmod adc/driver/nv_adc_demo.ko\n",
			DEV_NAME, SYSFS_IIO);
		return 1;
	}

	return test_device(devdir) ? 1 : 0;
}
