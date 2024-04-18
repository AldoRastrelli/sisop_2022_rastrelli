#include "exec.h"
#include "parsing.h"
#include "utils.h"
#define _OVERWRITE 1

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	size_t i = 0;
	int index;
	char key[BUFLEN], value[BUFLEN];

	while (i < eargc) {
		if ((index = block_contains(eargv[i], '=')) > 0) {
			get_environ_key(eargv[i], key);
			get_environ_value(eargv[i], value, index);
		}

		if (setenv(key, value, _OVERWRITE) != 0) {
			perror("Error");
			extern status;
			status = 1;
		}
		i++;
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	bool o_creat_is_used =
	        flags & O_CREAT;  // Bitwise operation. Checks if O_CREATED is used
	mode_t permissions = o_creat_is_used ? S_IWUSR | S_IRUSR : 0644;

	return open(file, flags, permissions);
}

void
redirect(char *file, int fd, int flags)
{
	int open_fd = open_redir_fd(file, flags);
	if (open_fd < 0) {
		perror("Error");
		_exit(1);
	}

	int dup_result = dup2(open_fd, fd);
	if (dup_result < 0) {
		close(open_fd);
		perror("Error");
		_exit(1);
	}
	close(open_fd);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);
		// spawns a command

		int result = execvp(e->argv[0], e->argv);
		if (result < 0) {
			if (errno == ENOENT) {
				perror("Error");
			}
			_exit(1);
		}
		break;
	}

	case BACK: {
		// runs a command in background

		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//

		r = (struct execcmd *) cmd;
		set_environ_vars(r->eargv, r->eargc);

		int in_fd = 0;
		int out_fd = 1;
		int error_fd = 2;

		// Begin name's length verification
		bool has_in_file = strlen(r->in_file) > 0;
		bool has_out_file = strlen(r->out_file) > 0;
		bool has_err_file = strlen(r->err_file) > 0;

		if (has_in_file) {
			redirect(r->in_file, in_fd, O_CLOEXEC | O_RDONLY);
		}

		if (has_out_file) {
			redirect(r->out_file,
			         out_fd,
			         O_CLOEXEC | O_TRUNC | O_CREAT | O_RDWR);
		}

		if (has_err_file) {
			// 2>&1: Out and Err
			bool combine_out_err = strcmp(r->err_file, "&1") == 0;

			// 2>&1: Out and Err
			if (combine_out_err) {
				int dup_result = dup2(out_fd, error_fd);
				if (dup_result < 0) {
					perror("Error");
					_exit(1);
				}
				// Standard Error to File
			} else {
				redirect(r->err_file,
				         error_fd,
				         O_CREAT | O_RDWR | O_CLOEXEC);
			}
		}

		if (!has_in_file && !has_out_file && !has_err_file) {
			perror("Error");
			_exit(1);
		}

		// End verifications

		if (execvp(*(r->argv), r->argv) < 0) {
			_exit(1);
		}
		break;
	}

	case PIPE: {
		// pipes two commands
		//

		// TODO: fails test 12. Fix.
		p = (struct pipecmd *) cmd;

		int fds[2];
		int pid1;
		int pid2;

		if (pipe(fds) < 0) {
			perror("Error");
		}

		pid1 = fork();
		if (pid1 < 0) {
			perror("Error");
		}

		if (process_is_child(pid1)) {
			// Left Command
			close(fds[READ]);

			int dup_result = dup2(fds[WRITE], WRITE);
			if (dup_result < 0) {
				close(fds[WRITE]);
				perror("Error");
				_exit(1);
			}
			close(fds[WRITE]);
			exec_cmd(p->leftcmd);
			_exit(0);

		} else if (process_is_parent(pid1)) {
			// Right Command(s)
			close(fds[WRITE]);

			// Fork for possible right command(s)
			pid2 = fork();
			if (pid2 < 0) {
				perror("Error");
				_exit(-1);
			}

			if (process_is_child(pid2)) {
				int dup_result = dup2(fds[READ], READ), fds[READ];
				if (dup_result < 0) {
					close(fds[READ]);
					perror("Error");
					_exit(1);
				}

				close(fds[READ]);
				close(fds[WRITE]);
				struct cmd *right_cmd =
				        parse_line(p->rightcmd->scmd);
				exec_cmd(right_cmd);
				_exit(1);

			} else {
				wait(NULL);
				wait(NULL);
			}
		} else {
			perror("Error");
			_exit(1);
		}

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		_exit(0);
		break;
	}
	}
}