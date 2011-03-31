#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

static const char *install_dirs[] = {
	"/usr",
	"/usr/local/classpath",
	"/usr/gnu-classpath-0.98",	/* Gentoo */
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

	for (i = 0; i < sizeof(install_dirs) / sizeof(*install_dirs); i++) {
		char glibj[PATH_MAX];
		char lib[PATH_MAX];

		strcpy(glibj, install_dirs[i]);
		strcat(glibj, "/share/classpath/glibj.zip");

		strcpy(lib, install_dirs[i]);
		strcat(lib, "/lib/classpath");

		if (is_file(glibj) && is_dir(lib)) {
			printf("%s\n", install_dirs[i]);
			break;
		}
	}
	return 0;
}
