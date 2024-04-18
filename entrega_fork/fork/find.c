#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define EXTRA_ARG "-i"

/* Auxiliar Funcs */
static bool
are_equal(char *string1, char *string2)
{
	// Strcmp return 0 if both strings are equal. This must be negated for bool return value to be correct.
	return !strcmp(string1, string2);
}

static bool
is_actual_or_parent(char *dir_name)
{
	return are_equal(dir_name, ".") || are_equal(dir_name, "..");
}

static bool
includes_string_cs(char *name, char *str)
{
	return strstr(name, str) != NULL;
}

static bool
includes_string_notcs(char *name, char *str)
{
	return strcasestr(name, str) != NULL;
}

static bool
is_case_sensitive(char *case_insensitive)
{
	return are_equal(case_insensitive, EXTRA_ARG);
}

static bool
extra_arg_is_correct(char *case_insensitive)
{
	return (case_insensitive == NULL) || is_case_sensitive(case_insensitive);
}

static void
print_command_error()
{
	printf("Error de comando.\nLas opciones son:\n\t 'find <str>' for case "
	       "sensitive\n\t'find -i <str>' for case insensitive.\n");
}

/* Primary Funcs */
static void
find(DIR *directory, char *path, char *str, bool (*find_func)(char *, char *))
{
	int file_descr = dirfd(directory);
	struct dirent *entry;

	while ((entry = readdir(directory)) != NULL) {
		if (entry->d_type == DT_DIR) {
			if (!is_actual_or_parent(entry->d_name)) {
				int sub_file_descr = openat(file_descr,
				                            entry->d_name,
				                            O_DIRECTORY);
				DIR *subdirectory = fdopendir(sub_file_descr);
				char sub_path[PATH_MAX];
				strcpy(sub_path, path);
				strcat(sub_path, entry->d_name);
				strcat(sub_path, "/");

				find(subdirectory, sub_path, str, find_func);
			}
		}

		if (find_func(entry->d_name, str)) {
			printf("%s%s\n", path, entry->d_name);
		}
	}
	closedir(directory);
}

int
main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		print_command_error();
		exit(1);
	}

	char *str = (argc > 2) ? argv[2] : argv[1];
	char *extra_arg = (argc > 2) ? argv[1] : NULL;

	bool case_insensitive;

	if (extra_arg != NULL) {
		if (extra_arg_is_correct(extra_arg)) {
			case_insensitive = true;
		} else {
			print_command_error();
			exit(1);
		}
	} else {
		case_insensitive = false;
	}

	DIR *directory = opendir(".");
	if (directory == NULL) {
		perror("error con opendir\n");
		exit(-1);
	}

	bool (*find_func)(char *, char *) =
	        case_insensitive ? &includes_string_notcs : &includes_string_cs;

	find(directory, "", str, find_func);
}
