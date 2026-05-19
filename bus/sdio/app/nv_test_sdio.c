// SPDX-License-Identifier: GPL-2.0
/*
 * User-space test for nv_sdio_demo
 *
 * Without a matching SDIO function: verifies driver sysfs registration.
 * With a bound device: exercises .../reg sysfs via SDIO I/O.
 *
 *   ./bin/nv_test_sdio [sdio_func_sysfs_dir]
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSFS_SDIO	"/sys/bus/sdio/devices"
#define SYSFS_DRV	"/sys/bus/sdio/drivers/nv_sdio_demo"
#define DRV_NAME	"nv_sdio_demo"

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

static int find_bound_device(char *out, size_t outlen)
{
	DIR *dir;
	struct dirent *ent;
	char path[512];
	ssize_t n;

	dir = opendir(SYSFS_SDIO);
	if (!dir)
		return -1;

	while ((ent = readdir(dir)) != NULL) {
		char driver_path[512];
		char target[256];

		if (ent->d_name[0] == '.')
			continue;

		snprintf(path, sizeof(path), "%s/%s", SYSFS_SDIO, ent->d_name);
		snprintf(driver_path, sizeof(driver_path), "%s/driver", path);
		n = readlink(driver_path, target, sizeof(target) - 1);
		if (n < 0)
			continue;
		target[n] = '\0';

		if (strstr(target, DRV_NAME)) {
			snprintf(out, outlen, "%s", path);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);
	return -1;
}

static int test_reg_sysfs(const char *devdir)
{
	char path[520];
	char buf[32];

	snprintf(path, sizeof(path), "%s/reg", devdir);
	if (access(path, W_OK) != 0) {
		fprintf(stderr, "no writable %s\n", path);
		return -1;
	}

	if (write_file(path, "0x5a") < 0)
		return -1;
	if (read_file(path, buf, sizeof(buf)) < 0)
		return -1;

	if (strstr(buf, "5a") == NULL && strstr(buf, "5A") == NULL) {
		fprintf(stderr, "FAIL: reg expected 0x5a got %s", buf);
		return -1;
	}

	printf("  reg read/write OK (%s)\n", buf);
	return 0;
}

static int test_driver_only(void)
{
	char name[64];

	if (access(SYSFS_DRV, F_OK) != 0) {
		fprintf(stderr, "FAIL: %s missing\n", SYSFS_DRV);
		return -1;
	}

	snprintf(name, sizeof(name), "%s/%s", SYSFS_DRV, "bind");
	printf("Driver registered: %s\n", SYSFS_DRV);
	printf("No SDIO function matched vendor/device 0x1234:0x5678 — probe skipped.\n");
	printf("PASS: driver load ok (bind when hardware present)\n");
	return 0;
}

int main(int argc, char **argv)
{
	char devdir[512];

	if (argc > 1) {
		strncpy(devdir, argv[1], sizeof(devdir) - 1);
		devdir[sizeof(devdir) - 1] = '\0';
		printf("Testing %s\n", devdir);
		return test_reg_sysfs(devdir) ? 1 : 0;
	}

	if (find_bound_device(devdir, sizeof(devdir)) == 0) {
		printf("Bound device: %s\n", devdir);
		if (test_reg_sysfs(devdir) == 0) {
			printf("PASS: SDIO reg sysfs ok\n");
			return 0;
		}
		return 1;
	}

	return test_driver_only() ? 1 : 0;
}
