#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


char *logged_strdup(char *str)
{
	char *line_dup = strdup(str);

	if (!line_dup)
		fprintf(stderr, "error: %s\n", strerror(errno));

	return line_dup;
}

int tokenize(char *str, char **argv, char *delims, int max_tokens)
{
	int argc = 0;

	for (argv[argc] = strtok(str, delims);
			argv[argc] != NULL;
			argv[argc] = strtok(NULL, delims)) {
		if (++argc >= max_tokens) {
			fprintf(stderr, "error: too many tokens\n");
			return -1;
		}
	}

	return argc;
}

static int is_empty_str(const char *str)
{
	while (*str) {
		if (!isspace(*str++))
			return 0;
	}

	return 1;
}

/*
 * based off
 * http://stackoverflow.com/questions/8705844/
 * need-to-know-when-no-data-appears-between-two-token-separators-using-strtok
 */
static char *single_del_strtok(char *str, char const *delimiter)
{
	static char *src = "";
	char *p = 0;
	char *ret = 0;

	if (str)
		src = str;

	if (!src)
		return NULL;

	p = strpbrk(src, delimiter);

	if (p) {
		*p  = 0;
		ret = src;
		src = ++p;
	} else if (*src) {
		ret = src;
		src = NULL;
	}

	return ret;
}

static int get_num_delim(const char *input, const char delim)
{
	int num_delim = 0;

	while (*input) {
		if (*input++ == delim)
			num_delim++;
	}

	return num_delim;
}

static int get_num_toks(char *line, const char *delim)
{
	int i = 0;

	char line_dup[strlen(line) + 1];
	char *arg;

	strncpy(line_dup, line, strlen(line));
	line_dup[strlen(line)] = '\0';

	arg = single_del_strtok(line_dup, delim);

	while (arg) {
		if (!is_empty_str(arg))
			i++;

		arg = single_del_strtok(NULL, delim);
	}

	return i;
}

int check_pipeline(char *input)
{
	int num_toks = get_num_toks(input, PIPE_CALL_DEL);
	int num_delim = get_num_delim(input, *PIPE_CALL_DEL);

	if (num_toks < 1 || num_toks != num_delim + 1) {
		fprintf(stderr, "error: invalid pipeline\n");
		return 0;
	}

	return 1;
}
