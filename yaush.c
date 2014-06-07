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

/* parse()
   @params: line_read -- the string to be parsed
            arg -- store the tokens, arg[i] respents the ith token
   @return: the number of tokens, note that may be some tokens would be NULL
 */
char** parse(char* line_read, int* _ntokens)
{
	char delim = ' ';
	int nspace = 0;
	int i;
	// count the number of 'space'
	for (i = 0; i < strlen(line_read); i++)
	{
		//log_debug("%d%c" ,strlen(line_read), line_read[i]);
		if (line_read[i] == delim)
			nspace++;
	}
	//log_debug("%s %d\n", line_read, nspace);
	int ntokens = nspace + 1;
	char **arg = (char**)malloc((ntokens+1)*sizeof(char*));
	log_debug("arg : %p\n", arg);
	for (i = 0; i < ntokens; i++)
	{
		arg[i] = (char*)malloc(255*sizeof(char));
		strcpy(arg[i], "\0");
	}
	arg[ntokens] = NULL;
	char *pstr;
	int offset = 0;
	int pos = 0;
	// seperate the string using 'space' to get tokens
	//for (i = 0; i < ntokens; i++)
	int count = 0;
	while (offset < strlen(line_read))
	{
		pstr = strchr(line_read + offset, delim);
		if (pstr == NULL)
			pos = strlen(line_read);
		else
		{
			pos = strlen(line_read) - strlen(pstr);
			//log_debug("%s\n", pstr);
		}
		log_debug("%d %d\n", offset, pos);
		if (pos > offset)
		{
			strncpy(arg[count], line_read + offset, pos - offset);
			arg[count][pos-offset] = '\0';
		}
		else    // this is a 'space', so skip this string
		{
			offset = pos+1;
			continue;
		}
		offset = pos+1;
		log_debug("%s\n", arg[count]);
		count++;
	}
	log_debug("token count:%d\n", count);
	for (i = count; i < ntokens; i++)
	{
		free(arg[i]);
		arg[i] = NULL;
	}
	*_ntokens = ntokens;
	return arg;
}

/* exec_cmd()
   @params: path -- the path of the command 
            arg -- the arguments of the command
            ntokens -- the number of the tokens in arg (actually length(arg) == ntokens+1,
                       because the last token is always set to NULL)
   @return: void
 */
void exec_cmd(char** arg, int ntokens)
{
	int i;
	for (i = 0 ; i < ntokens; i++)
	{
		log_debug("%s\n", arg[i]);
	}
	// fork to execute the command
	if (fork() == 0)
	{	
		// execvp() search $PATH to locate the bin file
		int ret = execvp(arg[0], arg);
		// print the error message
		if (ret < 0)
			printf("error:%s\n", strerror(errno));
		// free
		int i; 
		for (i = 0; i < ntokens; i++)
		{
			if (arg[i] != NULL);
			{
				free(arg[i]);
				arg[i] = NULL;
			}
		}
		free(arg);
		arg = NULL;
	}
	// wait until the child process return
	wait(NULL);
}


int main(int argc, char** argv)
{
	char** arg = NULL;
	int ntokens = 0;
	while(1)
	{
		arg = NULL;
		ntokens = 0;
		line_read = rl_gets();
		arg = parse(line_read, &ntokens);
		log_debug("arg : %p\n", arg);
		exec_cmd(arg, ntokens);
	}
	return 0;
}
