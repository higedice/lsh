#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "builtin_commands.h"
#include "prompt.h"


int hish_launch(char **args)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(args[0], args) == -1) {
			perror("hish");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("hish");
	} else {
		// Parent process
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}


int hish_execute(char **args)
{
	int i;
	int status;

	if (args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	if ((status = exec_if_builtin_cmd(args[0], args)) >= 0) {
		return status;
	}

	return hish_launch(args);
}


#define HISH_RL_BUFSIZE 1024

char *hish_read_line(void)
{
	int bufsize = HISH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "hish: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		c = getchar();

		if (c == EOF) {
			exit(EXIT_SUCCESS);
		} else if (c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		} 
		position++;

		if (position >= bufsize) {
			bufsize += HISH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "hish: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}


#define HISH_TOK_BUFSIZE 64
#define HISH_TOK_DELIM " \t\r\n\a"

char **hish_split_line(char *line)
{
	int bufsize = HISH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token, **tokens_backup;

	if (!tokens) {
		fprintf(stderr, "hish: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, HISH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += HISH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				free(tokens_backup);
				fprintf(stderr, "hish: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, HISH_TOK_DELIM);
	}

	tokens[position] = NULL;

	return tokens;
}


void hish_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		print_prompt();
		line = hish_read_line();
		args = hish_split_line(line);
		status = hish_execute(args);

		free(line);
		free(args);
	} while (status);
}


int main(int argc, char **argv)
{
	// load config files, if any.

	// Run command loop.
	hish_loop();

	// Perform any shutdown/cleanup.

	return EXIT_SUCCESS;
}







