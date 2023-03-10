#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"

#define DELIMS " "
#define EXECV_ERROR 66

/* Pointer to the buffer which contains input string */
char *input_line;

static void process_line(char *line, char add_to_history);

static void release_all_resources(void)
{
	free(input_line);
}

static int handle_cd_cmd(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "error: expecting one argument\n");
		return -1;
	}

	if (chdir(argv[1]) < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int handle_exit_cmd(int argc, char **argv)
{
	release_all_resources();
	exit(0);

	return -1; /* Unreachable */
}

static const struct command commands[] = {
	{ "cd", handle_cd_cmd },
	{ "exit", handle_exit_cmd },
	{ NULL, NULL }
};

const struct command *check_builtin_cmd(char *input)
{
	const struct command *cmd = NULL;

	for (cmd = commands; cmd->name != NULL; ++cmd) {
		if (!strcmp(input, cmd->name))
			return cmd;
	}

	return NULL;
}

/* Check syntax */
static int preprocess_line(char *line, const struct command **cmd_)
{
	char *line_dup, *line_dup_org, *token;
	const struct command *cmd;

	line_dup = logged_strdup(line);

	line_dup_org = line_dup; /* For free(), keep original pointer */
	if (!line_dup_org)
		return -1;

	token = strtok(line_dup, DELIMS);
	if (!token)
		goto cleanup;

	cmd = check_builtin_cmd(token);
	if (cmd)
		*cmd_ = cmd;

cleanup:
	free(line_dup_org);

	return 0;
}

/*
 * Attempts to kick off the command specified by line as a child process. The
 * child process will accept input from the i/o resource specified by
 * read_end_fd and write output to the i/o resource specified by write_end_fd.
 * If read_end_fd == -1, the child process will read from stdin. If
 * write_end_fd == -1, the child process will write to stdout.
 */
int handle_executable(char *line)
{
	char *argv[_POSIX_ARG_MAX];
	pid_t pid;

	/* Command was invalid, i.e. too many args */
	if (tokenize(line, argv, DELIMS, _POSIX_ARG_MAX) < 0)
		return -1;

	pid = fork();
	if (pid == -1)
		goto error;

	if (!pid) {
		execv(argv[0], argv);

		/* Should not be reached. */
		fprintf(stderr, "error: child: %s.", strerror(errno));

		fprintf(stderr, "\n");

		release_all_resources();
		/* to differentiate from the error of the command itself */
		exit(EXECV_ERROR);
	}

	return 0;

error:
	fprintf(stderr, "error: %s\n", strerror(errno));
	return -1;
}

/*
 * Tokenizes the line into a series of commands that form a pipeline. Runs the
 * commands in the pipeline, chaining them together with pipes.
 */
static void process_line(char *line, char add_to_history)
{
	int exit_status = 0,
	    line_len = strlen(line);
	char line_dup[line_len + 1];
	char *argv[_POSIX_ARG_MAX];
	const struct command *cmd = NULL;

	strncpy(line_dup, line, line_len);
	line_dup[line_len] = '\0';

	if (preprocess_line(line_dup, &cmd) < 0)
		goto cleanup;
	/*
	 * This loop executes each command in the pipeline, linking it to the
	 * next command with a pipe.
	 */
	char *cur_cmd = line_dup;

	if (cmd != NULL) {
		int argc = tokenize(cur_cmd, argv, DELIMS,
				_POSIX_ARG_MAX);

		/*
		 * Use function pointer to execute the handler
		 * implementation for each built-in command.
		 *
		 * You will see a similar pattern often in the
		 * Linux kernel. =]
		 */
		if (cmd->handle_cmd(argc, argv))
			goto cleanup;

		cmd = NULL;
	} else if (handle_executable(cur_cmd) < 0) {
		goto cleanup;
	}

cleanup:
	/* Wait for all of the children to exit. */
	for (;;) {
		if (wait(&exit_status) == -1) {
			/*
			 * ECHILD indicates that there are no children left to
			 * wait for. Any other value of errno is an error.
			 */
			if (errno == ECHILD)
				break;
			fprintf(stderr, "error: %s\n", strerror(errno));
			//all_valid = 0;
		}
	}
}

static inline void print_prompt(void)
{
	fprintf(stderr, "$");
	fflush(stderr);
}

/*
 * Release all resources upon detecting SIGINT (control-c).
 */
static void handle_control_c(int unused)
{
	release_all_resources();
	exit(0);
}

int main(int argc, char **argv)
{
	ssize_t n = 0;
	size_t len = 0;

	print_prompt();

	signal(SIGINT, handle_control_c);

	while ((n = getline(&input_line, &len, stdin)) > 0) {
		if (n > 1) {
			/* Remove newline character */
			input_line[n - 1] = '\0';
			process_line(input_line, 1);
		}
		print_prompt();
	}

	if (n < 0 && !feof(stdin))
		fprintf(stderr, "error: %s\n", strerror(errno));

	release_all_resources();

	return 0;
}
