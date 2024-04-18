#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return strncmp(cmd, "exit\0", 5) == 0 ||
	       strncmp(cmd, "exit\t", 5) == 0 || strncmp(cmd, "exit ", 5) == 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *path;
	bool includes_cd =
	        strncmp(cmd, "cd\0", 3) == 0 || strncmp(cmd, "cd ", 3) == 0;

	if (!includes_cd) {
		return false;
	}

	char *separator = " ";
	path = strtok(cmd, separator);
	path = strtok(NULL, separator);

	if (includes_cd && path == NULL) {
		path = getenv("HOME");
	}

	if (chdir(path) == 0) {
		snprintf(prompt, sizeof prompt, "(%s)", getcwd(path, PRMTLEN));
		return true;
	}
	extern status;
	status = 1;
	perror("Error");
	return false;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	bool cmd_is_pwd =
	        strncmp(cmd, "pwd\0", 4) == 0 || strncmp(cmd, "pwd ", 4) == 0;

	if (cmd_is_pwd) {
		char working_dir[PRMTLEN];
		bool success = getcwd(working_dir, sizeof(working_dir)) != NULL;
		if (success) {
			printf("%s\n", working_dir);
			return true;
		}
		extern status;
		status = 1;
		perror("Error");
	}
	return false;
}
