#ifndef _UTILS_H_
#define _UTILS_H_

#define PIPE_CALL_DEL "|"

/*
 * Attempts to copy a string and logs if strdup fails.
 */
char *logged_strdup(char *str);

/*
 * Tokenizes the string based on the delims. Stores pointers to each token
 * in the array argv. Returns the number of tokens.
 */
int tokenize(char *str, char **argv, char *delims, int max_tokens);

/*
 * Checks to see if given command is a valid pipeline by comparing
 * number of processes against number of pipes.
 */
int check_pipeline(char *input);

#endif
