#include <errno.h>
#include <sys/types.h>
#include "utils.c"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

static void
filter_non_primes(int primes)
{
	int wstatus;

	int prime;
	int read_result = read(primes, &prime, sizeof(prime));
	if (read_result < 0) {
		perror("Error de lectura.\n");
		exit(1);
	}
	// Empty pipe
	if (read_result == 0) {
		exit(0);
	}

	int fd[2];
	if (pipe(fd) < 0) {
		perror("Error en File Descriptors para el pipe");
		exit(1);
	}

	pid_t child_id = fork();

	if (child_id < 0) {
		fprintf(stderr, "Error en fork.\n");
		exit(1);
	}

	if (process_is_parent(child_id)) {
		close(fd[READ]);
		printf("primo %d\n", prime);

		int last_num;
		while ((read(primes, &last_num, sizeof(last_num)) > 0)) {
			if (last_num % prime != 0) {
				if (write(fd[WRITE], &last_num, sizeof(last_num)) <
				    0) {
					printf("ERROR. Falló write: %m\n");
					exit(1);
				}
			}
		}

		close(primes);
		close(fd[WRITE]);
		wait(&wstatus);

	} else if (process_is_child(child_id)) {
		close(primes);
		close(fd[WRITE]);
		filter_non_primes(fd[READ]);
		close(fd[READ]);
	}
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		perror("Error de argumentos.\n");
		exit(1);
	}

	int wstatus;
	int min_num = 0;
	int max_num = atoi(argv[1]);

	int fd[2];
	if (pipe(fd) < 0) {
		perror("Error en File Descriptors para el pipe");
		exit(1);
	}

	pid_t child_id = fork();

	if (child_id < 0) {
		fprintf(stderr, "Error en fork.\n");
		exit(1);
	}

	if (process_is_parent(child_id)) {
		close(fd[READ]);
		for (int i = min_num + 2; i <= max_num; i++) {
			if (write(fd[WRITE], &i, sizeof(i)) < 0) {
				perror("Error de escritura.\n");
				exit(1);
			}
		}
		close(fd[WRITE]);
		wait(&wstatus);

	} else if (process_is_child(child_id)) {
		close(fd[WRITE]);
		filter_non_primes(fd[READ]);
		close(fd[READ]);
	}
}
