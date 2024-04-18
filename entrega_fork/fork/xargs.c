#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "utils.c"

#ifndef NARGS
#define NARGS 4
#endif

static void
exec_command(char *command, char **exec_args)
{
	int pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Error en fork.\n");
		exit(1);
	}

	if (process_is_child(pid)) {
		if (execvp(command, exec_args) == -1) {
			perror("Error de execvp.\n");
			exit(1);
		}
	} else {
		// wait for child to end execvp
		wait(NULL);
	}
}

static void
end_exec_line(char **exec_args, char *exec_line, int lines_read)
{
	// https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
	char *token = NULL;
	/* get the first token */
	token = strtok(exec_line, "\n");

	exec_args[0] = "";

	int arg_num = 1;
	while (token != NULL) {
		/* walk through other tokens */
		exec_args[arg_num] = token;
		arg_num++;
		token = strtok(NULL, "\n");
	}
	exec_args[1 + lines_read] = NULL;
}

static int
limit_was_achieved(int lines_read)
{
	return lines_read == NARGS;
}

static void
empty_line(char *line)
{
	strncpy(line, "", sizeof(&line));
}

int
main(int argc, char *argv[])
{
	if (argc <= 1) {
		perror("Error de argumentos.\n");
		exit(1);
	}

	char *exec_args[NARGS + 2];
	// First arg is command
	char *command = argv[1];

	/* Assembled line_read */
	char exec_line[1900];
	empty_line(exec_line);
	int lines_read = 0;

	/* Line read from file */
	char *line_read = NULL;
	size_t line_size = 1024;

	while (getline(&line_read, &line_size, stdin) > 0) {
		// copy line_read at the end of exec_line
		strcat(exec_line, line_read);
		lines_read++;

		if (limit_was_achieved(lines_read)) {
			end_exec_line(exec_args, exec_line, lines_read);
			exec_command(command, exec_args);
			empty_line(exec_line);
			lines_read = 0;
		}
	}

	if (lines_read > 0) {
		end_exec_line(exec_args, exec_line, lines_read);
		exec_command(command, exec_args);
	}
}