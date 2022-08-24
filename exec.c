#include "exec.h"

void redir_fd(int fd_old, int fd_new);


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
	// Your code here
	for (int i = 0; i < eargc; i++) {
		char key[BUFLEN];
		char value[BUFLEN];

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, block_contains(eargv[i], '='));
		if (setenv(key, value, 1) < 0) {
			perror("Error en setenv");
		}
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
	// Your code here
	int fd = open(file, flags, S_IWUSR | S_IRUSR);
	if (fd < 0) {
		perror("Error en open");
		exit(-1);
	}

	return fd;
}

void
redir_fd(int fd_old, int fd_new)
{
	if (dup2(fd_old, fd_new) < 0) {
		perror("Error en dup");
		close(fd_old);
		exit(-1);
	}
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
	case EXEC:
		// spawns a command
		//
		// Your code here
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) < 0) {
			perror("Error en exec");
			_exit(-1);
		}
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
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
		// Your code here
		r = (struct execcmd *) cmd;

		if (strlen(r->in_file) > 0) {
			int fd = open_redir_fd(r->in_file, O_CLOEXEC);
			redir_fd(fd, 0);
		}

		if (strlen(r->out_file) > 0) {
			int fd = open_redir_fd(r->out_file,
			                       O_CLOEXEC | O_CREAT | O_WRONLY |
			                               O_TRUNC);
			redir_fd(fd, 1);
		}

		if (strlen(r->err_file) > 0) {
			if (!strcmp(r->err_file, "&1"))
				redir_fd(1, 2);
			else {
				int fd = open_redir_fd(r->err_file,
				                       O_CLOEXEC | O_CREAT |
				                               O_WRONLY | O_TRUNC);
				redir_fd(fd, 2);
			}
		}

		cmd->type = EXEC;
		exec_cmd(cmd);

		break;
	}

	case PIPE: {
		// pipes two commands
		//
		// Your code here
		p = (struct pipecmd *) cmd;
		int status;

		int fd[2];
		if (pipe(fd) < 0) {
			perror("Error en pipe");
			exit(-1);
		}

		int pid_l = fork();
		if (pid_l < 0) {
			perror("Error en fork");
			close(fd[WRITE]);
			close(fd[READ]);
			exit(-1);
		}

		if (pid_l == 0) {
			free_command(p->rightcmd);
			free((struct pipecmd *) cmd);
			close(fd[READ]);
			dup2(fd[WRITE], STDOUT_FILENO);
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		}

		int pid_r = fork();
		if (pid_r < 0) {
			perror("Error en fork");
			close(fd[WRITE]);
			close(fd[READ]);
			exit(-1);
		}

		if (pid_r == 0) {
			free_command(p->leftcmd);
			free((struct pipecmd *) cmd);
			close(fd[WRITE]);
			dup2(fd[READ], STDIN_FILENO);
			close(fd[READ]);
			exec_cmd(p->rightcmd);
		}

		close(fd[WRITE]);
		close(fd[READ]);
		waitpid(pid_l, NULL, 0);
		waitpid(pid_r, &status, 0);

		// free the memory allocated
		// for the pipe tree structure
		free_command(cmd);
		exit(WEXITSTATUS(status));
		break;
	}
	}
}
