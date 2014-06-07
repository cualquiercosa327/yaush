#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

#include "log_debug.h"

// A static variable for holding the line.
char *line_read = (char *)NULL;

// Read a string, and return a pointer to it.
// Returns NULL on EOF.
char* rl_gets ()
{
	char promptstr[255];
	char buf[255];
	// If the buffer has already been allocated,
	// return the memory to the freze pool.
	if (line_read)
	{
		free (line_read);
		line_read = (char *)NULL;
	}

	sprintf(promptstr, "%s:%s >> ", getenv("USER"), getcwd(buf, sizeof(buf)));
	// Get a line from the user.
	line_read = readline (promptstr);

	// If the line has any text in it,
	// save it on the history.
	if (line_read && *line_read)
		add_history (line_read);

	return (line_read);
}

char** parse(char* line_read)
{
	char delim = ' ';
	int nspace = 0;
	int i;
	// count the number of 'space'
	for (i = 0; i < strlen(line_read); i++)
	{
		if (line_read[i] == delim)
			nspace++;
	}
	//log_debug("%s %d\n", line_read, nspace);
	int ntokens = nspace + 1;
	char **arg = (char**)malloc((ntokens+1)*sizeof(char*));
	for (i = 0; i < ntokens+1; i++)
	{
		arg[i] = (char*)malloc(255*sizeof(char));
		strcpy(arg[i], "\0");
	}
	arg[ntokens] = NULL;
	char *pstr;
	int offset = 0;
	int pos = 0;
	for (i = 0; i < ntokens; i++)
	{
		pstr = strchr(line_read + offset, delim);
		if (pstr == NULL)
			pos = strlen(line_read);
		else
		{
			pos = strlen(line_read) - strlen(pstr);
			//log_debug("%s\n", pstr);
		}
		//log_debug("%d %d\n", offset, pos);
		if (pos > offset)
			strncpy(arg[i], line_read + offset, pos - offset);
		offset = pos+1;
		log_debug("%s\n", arg[i]);
	}
	return arg;
}


void exec_cmd(char* path, char** arg)
{
	if (fork() == 0)
	{	
		int ret = execvp(path, arg);
		if (ret < 0)
			printf("error:%s\n", strerror(errno));
	}
	wait(NULL);
}

int main(int argc, char** argv)
{
	char** arg;
	while(1)
	{
		line_read = rl_gets();
		arg = parse(line_read);
		exec_cmd(arg[0], arg);
	}
	return 0;
}
