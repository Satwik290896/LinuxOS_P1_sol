#ifndef _SHELL_H_
#define _SHELL_H_

#include "utils.h"

/* A built-in command and its corresponding function */
struct command {
	const char *name;
	int (*handle_cmd)(int argc, char **argv);
};

#endif
