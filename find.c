#define _GNU_SOURCE

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

void buscar_coincidecias(DIR *directorio_actual,
                         char *subcadena,
                         char path[PATH_MAX]);

bool es_directorio_valido(struct dirent *entry);
int chequear_error(int resultado, char mensaje[]);
DIR *abrir_subdirectorio(DIR *directorio_padre, char *nombre_subdir);
char *(*ptr_subcadena_coincidente)(const char *, const char *);


int
main(int argc, char *argv[])
{
	if ((argc < 2) || (argc > 3) || ((argc == 3) && strcmp(argv[1], "-i"))) {
		printf("ERROR: Cantidad de argumentos erronea\n");
		return -1;
	}

	char *subcadena;

	if (argc == 3) {
		ptr_subcadena_coincidente = strcasestr;
		subcadena = argv[2];

	} else {
		ptr_subcadena_coincidente = strstr;
		subcadena = argv[1];
	}

	DIR *directorio = opendir(".");
	if (directorio == NULL) {
		perror("error con opendir");
		exit(-1);
	}

	buscar_coincidecias(directorio, subcadena, "");
	closedir(directorio);

	return 0;
}


//---------- FUNCIONES ----------

int
chequear_error(int resultado, char mensaje[])
{
	if (resultado == -1) {
		perror(mensaje);
		return (-1);
	}
	return 0;
}

void
buscar_coincidecias(DIR *directorio_actual, char *subcadena, char path[PATH_MAX])
{
	struct dirent *entry;
	entry = readdir(directorio_actual);
	if (!entry)
		return;

	char path_aux[PATH_MAX];
	strcpy(path_aux, path);

	if (es_directorio_valido(entry)) {
		DIR *dir_nuevo =
		        abrir_subdirectorio(directorio_actual, entry->d_name);
		if (!dir_nuevo)
			return;

		strcat(path_aux, entry->d_name);

		if (ptr_subcadena_coincidente(entry->d_name, subcadena)) {
			printf("%s\n", path_aux);
		}
		strcat(path_aux, "/");
		buscar_coincidecias(dir_nuevo, subcadena, path_aux);
		closedir(dir_nuevo);

	} else if (entry->d_type == DT_REG) {
		if (ptr_subcadena_coincidente(entry->d_name, subcadena)) {
			printf("%s\n", strcat(path_aux, entry->d_name));
		}
	}

	buscar_coincidecias(directorio_actual, subcadena, path);
}

DIR *
abrir_subdirectorio(DIR *directorio_padre, char *nombre_subdir)
{
	int fd = dirfd(directorio_padre);
	if (chequear_error(fd, "error en dirfd") == -1)
		return NULL;

	int fd_subdir = openat(fd, nombre_subdir, O_DIRECTORY);
	if (chequear_error(fd_subdir, "error en openat") == -1)
		return NULL;

	DIR *dir_nuevo = fdopendir(fd_subdir);
	if (dir_nuevo == NULL)
		perror("error con opendir");

	return dir_nuevo;
}

bool
es_directorio_valido(struct dirent *entry)
{
	return (entry->d_type == DT_DIR) && strcmp(entry->d_name, "..") &&
	       strcmp(entry->d_name, ".");
}