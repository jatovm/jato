#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#define NR_DIRS 2

static const char *install_dirs[NR_DIRS] = {
	"/usr",
	"/usr/local",
};

static bool is_file(const char *path)
{
	struct stat st;
	int err;

	err = stat(path, &st);
	if (err)
		return false;

	return S_ISREG(st.st_mode);
}

static bool is_dir(const char *path)
{
	struct stat st;
	int err;

	err = stat(path, &st);
	if (err)
		return false;

	return S_ISDIR(st.st_mode);
}

int main(int argc, char *argv[])
{
	int i;

	for (i = 0; i < NR_DIRS; i++) {
		char glibj[PATH_MAX];
		char lib[PATH_MAX];

		strcpy(glibj, install_dirs[i]);
		strcat(glibj, "/share/classpath/glibj.zip");

		strcpy(lib, install_dirs[i]);
		strcat(lib, "/lib/classpath");

		if (is_file(glibj) || is_dir(lib)) {
			printf("%s\n", install_dirs[i]);
			break;
		}
	}
	return 0;
}
