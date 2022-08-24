#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

int chequear_error(int resultado, char mensaje[]);
void inicializar_array(char *args_array[], char *cmd);
void limpiar_array(char *args_array[]);

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("ERROR: Cantidad de argumentos erronea\n");
		exit(-1);
	}

	char *args_array[NARGS + 2];
	inicializar_array(args_array, argv[1]);

	int args_leidos = 0;
	char *linea = NULL;
	ssize_t r;

	do {
		int pos = (args_leidos % NARGS) + 1;
		size_t len = 0;
		r = getline(&linea, &len, stdin);

		if (r > 0) {
			strtok(linea, "\n");
			args_array[pos] = linea;
			args_leidos++;
		}

		if ((args_leidos % NARGS == 0) || (r == -1)) {
			int pid = fork();
			chequear_error(pid, "[padre] error en fork");

			if (pid == 0) {
				int r = execvp(argv[1], args_array);
				chequear_error(r, "[hijo] error en exec");
			}

			if (wait(NULL) == -1)
				perror("Error en wait");
			limpiar_array(args_array);
		}
	} while (r > 0);

	free(linea);
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
inicializar_array(char *args_array[], char *cmd)
{
	for (int i = 0; i < NARGS + 2; i++) {
		args_array[i] = NULL;
	}
	args_array[0] = cmd;
}

void
limpiar_array(char *args_array[])
{
	for (int i = 1; i < NARGS + 1; i++) {
		if (args_array[i]) {
			free(args_array[i]);
			args_array[i] = NULL;
		}
	}
}