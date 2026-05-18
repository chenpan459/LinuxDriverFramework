// SPDX-License-Identifier: GPL-2.0
/*
 * User-space PWM test via sysfs (/sys/class/pwm/pwmchipN/pwm0/)
 *
 *   ./bin/nv_test_pwm [pwmchip_dir]
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SYSFS_PWM	"/sys/class/pwm"
#define PERIOD_NS	1000000U
#define DUTY_NS		250000U

static int write_str(const char *path, const char *val)
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

static int read_u64(const char *path, unsigned long long *out)
{
	FILE *f = fopen(path, "r");
	unsigned long long v;

	if (!f) {
		perror(path);
		return -1;
	}
	if (fscanf(f, "%llu", &v) != 1) {
		fclose(f);
		return -1;
	}
	fclose(f);
	*out = v;
	return 0;
}

static int find_demo_chip(char *out, size_t outlen)
{
	DIR *dir;
	struct dirent *ent;
	char path[512];
	char driver[128];

	dir = opendir(SYSFS_PWM);
	if (!dir)
		return -1;

	while ((ent = readdir(dir)) != NULL) {
		if (strncmp(ent->d_name, "pwmchip", 7) != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s/device/driver",
			 SYSFS_PWM, ent->d_name);
		if (readlink(path, driver, sizeof(driver) - 1) < 0)
			continue;
		driver[sizeof(driver) - 1] = '\0';

		if (strstr(driver, "nv_pwm_demo")) {
			snprintf(out, outlen, "%s/%s", SYSFS_PWM, ent->d_name);
			closedir(dir);
			return 0;
		}
	}

	closedir(dir);
	return -1;
}

static int test_pwmchip(const char *chipdir)
{
	char path[512];
	unsigned long long period, duty;
	int en;

	snprintf(path, sizeof(path), "%s/export", chipdir);
	if (write_str(path, "0") < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/period", chipdir);
	if (write_str(path, "1000000") < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/duty_cycle", chipdir);
	if (write_str(path, "250000") < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/enable", chipdir);
	if (write_str(path, "1") < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/period", chipdir);
	if (read_u64(path, &period) < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/duty_cycle", chipdir);
	if (read_u64(path, &duty) < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/pwm0/enable", chipdir);
	{
		FILE *f = fopen(path, "r");

		if (!f)
			return -1;
		if (fscanf(f, "%d", &en) != 1) {
			fclose(f);
			return -1;
		}
		fclose(f);
	}

	if (period != PERIOD_NS || duty != DUTY_NS || en != 1) {
		fprintf(stderr, "FAIL: period=%llu duty=%llu enable=%d\n",
			period, duty, en);
		return -1;
	}

	printf("PASS: pwm0 period=%llu duty=%llu enabled=%d\n",
	       period, duty, en);

	snprintf(path, sizeof(path), "%s/unexport", chipdir);
	write_str(path, "0");
	return 0;
}

int main(int argc, char **argv)
{
	char chipdir[512];

	if (argc > 1) {
		strncpy(chipdir, argv[1], sizeof(chipdir) - 1);
		chipdir[sizeof(chipdir) - 1] = '\0';
	} else if (find_demo_chip(chipdir, sizeof(chipdir)) < 0) {
		fprintf(stderr,
			"No nv_pwm_demo pwmchip in %s\n"
			"  sudo insmod pwm/driver/nv_pwm_demo.ko\n",
			SYSFS_PWM);
		return 1;
	}

	printf("Testing %s/pwm0\n", chipdir);
	return test_pwmchip(chipdir) ? 1 : 0;
}
