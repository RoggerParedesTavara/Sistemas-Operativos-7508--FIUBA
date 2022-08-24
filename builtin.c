#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	// Your code here
	if (!strcmp(cmd, "exit"))
		return 1;

	return 0;
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
	// Your code here
	char *dir;

	if (!strcmp(cmd, "cd") || !strcmp(cmd, "cd "))
		dir = getenv("HOME");

	else if (!strncmp(cmd, "cd ", 3))
		dir = cmd + 3;
	else
		return 0;

	if (chdir(dir) < 0) {
		perror("Error en cd");
		status = 1;
	} else {
		char buf[PRMTLEN];
		snprintf(promt, sizeof promt, "(%s)", getcwd(buf, PRMTLEN));
		status = 0;
	}

	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	// Your code here
	if (strcmp(cmd, "pwd"))
		return 0;

	char cwd[BUFLEN];
	if (!getcwd(cwd, BUFLEN)) {
		perror("Error en getcwd");
		status = 1;

	} else {
		fprintf(stdout, "%s\n", cwd);
		status = 0;
	}

	return 1;
}
