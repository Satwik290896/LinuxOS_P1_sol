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
#include <sys/mman.h>

#include "shell2.h"

#define DELIMS " "
#define EXECV_ERROR 66

/* Pointer to the buffer which contains input string */
char *input_line;
size_t len;

static void process_line(char *line, char add_to_history);

static void release_all_resources(void)
{
	if (munmap(input_line, len) < 0)
		exit(1);
}

ssize_t output(int fildes, char *s, int strerr)
{
	ssize_t r;
	char tbuf[100], *e, *p, *n = "\n";

	p = s;
	memset(tbuf, 0, 100);
	if (strerr) {
		p = tbuf;
		strncpy(tbuf, s, strlen(s));
		e = strerror(errno);
		strncat(tbuf, e, strlen(e));
		strncat(tbuf, n, strlen(n));
	}
	r = write(fildes, p, strlen(p));
	if (r < 0) {
		if (strerr != 2)
			release_all_resources();
		exit(1);
	}

	return r;
}

static char *mmap_realloc(char *n, char **s, size_t l, size_t dl)
{

	char *nmp;

	if (munmap(n, dl) < 0) {
		output(STDERR, "munmap failed: ", 2);
		exit(1);
	}
	nmp = mmap(NULL, l + dl, PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE, -1, 0);
	if (((long) nmp) < 0) {
		output(STDERR, "mmap failed: ", 1);
		release_all_resources();
		exit(1);
	}
	strncpy(nmp, *s, l);
	if (munmap(*s, l) < 0) {
		output(STDERR, "munmap failed: ", 2);
		exit(1);
	}
	return nmp;
}

static ssize_t input(char **s, size_t *l, int fildes)
{
	ssize_t r;
	size_t dl = 4096, rl;
	char *nmp; /* pointer to current position in buffer */

	if (!*s) {
		*l = dl;
		*s = mmap(NULL, *l, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
		if (((long) *s) < 0) {
			output(STDERR, "mmap failed: ", 1);
			release_all_resources();
			exit(1);
		}
	}
	nmp = *s;
	rl = *l;
	memset(*s, 0, *l);
	/* check if not end of input, then resize buf and continue reading */
	;
	while ((r = read(fildes, nmp, rl)) == rl &&
		nmp[rl - 1] != '\0' && nmp[rl - 1] != '\n') {
		nmp = mmap(*s + *l, dl, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
		if (((long) nmp) < 0) {
			output(STDERR, "mmap failed: ", 1);
			release_all_resources();
			exit(1);
		}
		/*
		 * realloc if new mmaped memory is not
		 * contiguous with the previous buffer
		 */
		if (nmp != *s + *l) {
			*s = mmap_realloc(nmp, s, *l, dl);
			nmp = *s + *l;
		}
		*l += dl;
		memset(nmp, 0, dl);
		rl = dl;
	}
	if (r < 0) {
		output(STDERR, "read failed: ", 1);
		return -1;
	}

	return strlen(*s);
}

static int handle_cd_cmd(int argc, char **argv)
{
	if (argc != 2) {
		output(STDERR, "error: expecting one argument\n", 0);
		return -1;
	}

	if (chdir(argv[1]) < 0) {
		output(STDERR, "error: ", 1);
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

	line_dup_org = line_dup; /* For munmap(), keep original pointer */
	if (!line_dup_org) {
		release_all_resources();
		exit(1);
	}

	token = strtok(line_dup, DELIMS);
	if (!token)
		goto cleanup;

	cmd = check_builtin_cmd(token);
	if (cmd)
		*cmd_ = cmd;

cleanup:
	if (munmap(line_dup_org, strlen(line_dup_org) + 1) < 0) {
		output(STDERR, "munmap failed: ", 2);
		exit(1);
	}

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
		output(STDERR, "error: child: ", 1);

		release_all_resources();
		/* to differentiate from the error of the command itself */
		exit(EXECV_ERROR);
	}

	return 0;

error:
	output(STDERR, "error: ", 1);
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
			output(STDERR, "error: ", 1);
			//all_valid = 0;
		}
	}
}

static inline void print_prompt(void)
{
	output(STDERR, "$", 0);
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

	len = 0;

	print_prompt();

	signal(SIGINT, handle_control_c);

	while ((n = input(&input_line, &len, STDIN)) > 0) {
		if (n > 1) {
			/* Remove newline character */
			input_line[n - 1] = '\0';
			process_line(input_line, 1);
		}
		print_prompt();
	}

	release_all_resources();

	return 0;
}
