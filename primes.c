#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int chequear_error(int resultado, char mensaje[]);
void enviar_numeros_a_primer_filtro(int fd[2], int n);
int aplicar_filtros(int fd_left[2]);
void enviar_numeros_filtrados(int fd_left[2],
                              int fd_right[2],
                              long num_primo_actual);


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("ERROR: Ingrese un sólo numero como argumento\n");
		exit(-1);
	}

	int fd[2];
	int p = pipe(fd);
	chequear_error(p, "[generador] error en pipe");

	int pid = fork();
	chequear_error(pid, "[generador] error en fork");

	if (pid != 0) {
		close(fd[0]);
		enviar_numeros_a_primer_filtro(fd, atoi(argv[1]));
		close(fd[1]);
		if (wait(NULL) == -1)
			perror("Error en wait");

	} else {
		aplicar_filtros(fd);
	}

	return 0;
}


//---------- FUNCIONES ----------

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
enviar_numeros_a_primer_filtro(int fd[2], int n)
{
	for (int i = 2; i <= n; i++) {
		int w = write(fd[1], &i, sizeof(i));
		chequear_error(w, "[generador] error en write");
	}
}


int
aplicar_filtros(int fd_left[2])
{
	close(fd_left[1]);

	int num_primo;
	int r = read(fd_left[0], &num_primo, sizeof(num_primo));
	chequear_error(r, "[filtro] error en read");

	// El ultimo proceso creado no tiene nada que leer, asi que cierra fd y termina.
	if (r == 0) {
		close(fd_left[0]);
		exit(0);
	}

	printf("primo %d\n", num_primo);

	int fd_right[2];
	int p = pipe(fd_right);
	chequear_error(p, "[filtro] error en pipe");

	int pid = fork();
	chequear_error(pid, "[filtro] error en fork");

	if (pid == 0) {
		close(fd_left[0]);
		close(fd_right[1]);
		return aplicar_filtros(fd_right);

	} else {
		close(fd_right[0]);

		enviar_numeros_filtrados(fd_left, fd_right, num_primo);

		close(fd_left[0]);
		close(fd_right[1]);
		if (wait(NULL) == -1)
			perror("Error en wait");
		exit(0);
	}
}

void
enviar_numeros_filtrados(int fd_left[2], int fd_right[2], long num_primo_actual)
{
	int n;
	int r;

	while ((r = read(fd_left[0], &n, sizeof(n)))) {
		chequear_error(r, "[filtro] error en read");

		if ((n % num_primo_actual) != 0) {
			int w = write(fd_right[1], &n, sizeof(n));
			chequear_error(w, "[filtro] Error en write");
		}
	}
}
