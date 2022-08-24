#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

int chequear_error(int resultado, char mensaje[]);
void imprimir_msj_prologo(int fd1[2], int fd2[2]);
void imprimir_msj_padre(int fork_pid, long random_number, int fd);
void imprimir_msj_hijo(int fork_pid, long received_number, int fd_r, int fd_w);
void imprimir_msj_epilogo(long received_number, int fd);

int
main(void)
{
	int fd1[2], fd2[2];
	srandom(time(NULL));

	int p1 = pipe(fd1);
	chequear_error(p1, "Error en primer pipe");
	int p2 = pipe(fd2);
	chequear_error(p2, "Error en segundo pipe");

	imprimir_msj_prologo(fd1, fd2);

	int pid = fork();
	chequear_error(pid, "Error en fork");

	if (pid != 0) {
		close(fd1[0]);
		close(fd2[1]);

		long int random_number = random();

		imprimir_msj_padre(pid, random_number, fd1[1]);

		int w = write(fd1[1], &random_number, sizeof(random_number));
		chequear_error(w, "[padre] Error en write");

		long int received_number;
		int r = read(fd2[0], &received_number, sizeof(received_number));
		chequear_error(r, "[padre] Error en read");

		imprimir_msj_epilogo(received_number, fd2[0]);

		close(fd1[1]);
		close(fd2[0]);
		if (wait(NULL) == -1)
			perror("Error en wait");

	} else {
		close(fd1[1]);
		close(fd2[0]);

		long int received_number;
		int r = read(fd1[0], &received_number, sizeof(received_number));
		chequear_error(r, "[hijo] Error en read");

		imprimir_msj_hijo(pid, received_number, fd1[0], fd2[1]);

		int w = write(fd2[1], &received_number, sizeof(received_number));
		chequear_error(w, "[hijo] Error en write");

		close(fd1[0]);
		close(fd2[1]);
	}

	return 0;
}

int
chequear_error(int resultado, char mensaje[])
{
	if (resultado < 0) {
		perror(mensaje);
		exit(-1);
	}
	return 0;
}

void
imprimir_msj_prologo(int fd1[2], int fd2[2])
{
	printf("Hola, soy PID %d:\n", getpid());
	printf("  - primer pipe me devuelve: [%d,%d]\n", fd1[0], fd1[1]);
	printf("  - segundo pipe me devuelve: [%d,%d]\n\n", fd2[0], fd2[1]);
}

void
imprimir_msj_padre(int fork_pid, long random_number, int fd)
{
	printf("Donde fork me devuelve %d:\n", fork_pid);
	printf("  - getpid me devuelve: %d\n", getpid());
	printf("  - getppid me devuelve: %d\n", getppid());
	printf("  - random me devuelve: %ld\n", random_number);
	printf("  - envío valor %ld a través de fd= %d\n\n", random_number, fd);
}

void
imprimir_msj_hijo(int fork_pid, long received_number, int fd_r, int fd_w)
{
	printf("Donde fork me devuelve %d:\n", fork_pid);
	printf("  - getpid me devuelve: %d\n", getpid());
	printf("  - getppid me devuelve: %d\n", getppid());
	printf("  - recibo valor %ld vía fd= %d\n", received_number, fd_r);
	printf("  - reenvío valor en fd=%d y termino\n\n", fd_w);
}

void
imprimir_msj_epilogo(long received_number, int fd)
{
	printf("Hola, de nuevo PID %d\n", getpid());
	printf("  - recibí valor %ld vía fd=%d\n", received_number, fd);
}
