#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.c"
#include "time.h"

int
main(void)
{
	int fd_parent_to_child[2];
	int fd_child_to_parent[2];

	if (pipe(fd_parent_to_child) < 0) {
		perror("Error en File Descriptors para el pipe1");
		return 1;
	}
	if (pipe(fd_child_to_parent) < 0) {
		perror("Error en File Descriptors para el pipe2");
		return 1;
	}

	printf("Hola, soy PID %d:\n", getpid());
	printf("\t - primer pipe me devuelve: [%d, %d]\n",
	       fd_parent_to_child[READ],
	       fd_parent_to_child[WRITE]);
	printf("\t - segundo pipe me devuelve: [%d, %d]\n\n",
	       fd_child_to_parent[READ],
	       fd_child_to_parent[WRITE]);

	pid_t child_id = fork();

	if (child_id < 0) {
		fprintf(stderr, "Error en fork.\n");
		return 1;
	}

	if (process_is_parent(child_id)) {
		int wstatus;
		int value_from_child;
		srandom(time(NULL));
		long int rnumber = random();

		close(fd_parent_to_child[READ]);
		close(fd_child_to_parent[WRITE]);

		printf("Donde fork me devuelve %d:\n", child_id);
		printf("\t - getpid me devuelve: %d\n", getpid());
		printf("\t - getppid me devuelve: %d\n", getppid());
		printf("\t - random me devuelve: %ld\n", rnumber);
		printf("\t - envío valor %ld a través de fd=%d\n\n",
		       rnumber,
		       fd_parent_to_child[WRITE]);

		int write_result = write(fd_parent_to_child[WRITE],
		                         &rnumber,
		                         sizeof(rnumber));
		if (write_result < 0) {
			perror("Error de escritura.\n");
			return 1;
		}
		close(fd_parent_to_child[WRITE]);
		wait(&wstatus);

		int read_result = read(fd_child_to_parent[READ],
		                       &value_from_child,
		                       sizeof(value_from_child));
		if (read_result < 0) {
			perror("Error de lectura.\n");
			return 1;
		}
		close(fd_child_to_parent[READ]);

		printf("Hola, de nuevo PID %d\n", getpid());
		printf("\t - recibí valor %d via fd=%d\n",
		       value_from_child,
		       fd_child_to_parent[READ]);

	} else if (process_is_child(child_id)) {
		close(fd_parent_to_child[WRITE]);
		close(fd_child_to_parent[READ]);

		int value_from_parent;

		if (read(fd_parent_to_child[READ],
		         &value_from_parent,
		         sizeof(value_from_parent)) < 0) {
			perror("Error de lectura.\n");
		}
		close(fd_parent_to_child[READ]);

		printf("Donde fork me devuelve %d:\n", child_id);
		printf("\t - getpid me devuelve: %d\n", getpid());
		printf("\t - getppid me devuelve: %d\n", getppid());
		printf("\t - recibo valor %d vía fd=%d\n",
		       value_from_parent,
		       fd_parent_to_child[READ]);
		printf("\t - reenvío valor en fd=%d y termino\n\n",
		       fd_child_to_parent[WRITE]);

		if (write(fd_child_to_parent[WRITE],
		          &value_from_parent,
		          sizeof(value_from_parent)) < 0) {
			printf("Error de escritura.\n");
		}
		close(fd_child_to_parent[WRITE]);
	}

	return 0;
}