#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "log_debug.h"
#include "list.h"

#define STRLEN	255
// A static variable for holding the line.
char *line_read = (char *)NULL;


struct node_cmd {
	char** arg;		// the arguments list of the command
	int ntokens;
	char out[255];		// the output : "stdout" "next" "filename"
	char in[255];		// the input :  "stdin"  "prev" "filename"
	struct list_head list;
};


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

/* lexer()
   @params: line_read -- the string to be parsed
   _ntokens -- (address) the number of tokens, note that may be some tokens would be NULL
   @return: arg -- store the tokens, arg[i] respents the ith token
 */
char** lexer(char* line_read, int* _ntokens)
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
	// tokens = #space + 1; and the additional one is set as NULL to label the end
	int ntokens = nspace + 2;
	char **arg = (char**)malloc(ntokens*sizeof(char*));
	//log_debug("arg : %p\n", arg);
	for (i = 0; i < ntokens; i++)
	{
		arg[i] = (char*)malloc(STRLEN*sizeof(char));
		strcpy(arg[i], "\0");
	}
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
		//log_debug("%d %d\n", offset, pos);
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
	//log_debug("token count:%d\n", count);
	for (i = count; i < ntokens; i++)
	{
		free(arg[i]);
		arg[i] = NULL;
	}
	*_ntokens = ntokens;
	//log_debug("The end of lexer\n");
	return arg;
}

void free_memory(char** arg, int ntokens)
{
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

struct list_head* parser(char** arg, int ntokens)
{
	int i;
	int pos = 0;
	int prevpos = 0;
	struct list_head *head = (struct list_head*)malloc(sizeof(struct list_head));
	int count = 0;
	INIT_LIST_HEAD(head);
	while (pos < ntokens)
	{
		// skip the null token
		//if (arg[pos] == NULL && pos < ntokens - 1)
		//{
		//	pos++;
		//	continue;	
		//}
		log_debug("%s\n", arg[pos]);
		// find the position of pipes - '|' 
		if (arg[pos] == NULL || strcmp(arg[pos], "|") == 0)
		{
			// if this is the last token
			//if (pos == ntokens - 1)
			//	pos += 1;
			// malloc for node
			struct node_cmd *node = (struct node_cmd*)malloc(sizeof(struct node_cmd));
			if (arg[pos] == NULL)
				strcpy(node->out, "stdout");
			else
				strcpy(node->out, "next");
			if (prevpos == 0)
				strcpy(node->in , "stdin");
			else
				strcpy(node->in, "prev");
			// malloc
			char **cmdarg = (char**)malloc((pos-prevpos+1)*sizeof(char*));
			node->arg = cmdarg;
			for (i = 0; i < pos - prevpos + 1; i++)
			{
				cmdarg[i] = (char*)malloc(STRLEN*sizeof(char));
				strcpy(cmdarg[i], "\0");
			}
			node->ntokens = pos - prevpos + 1;
			//log_debug("%d-%d\n", prevpos, pos);
			// prevpos ~ pos : one command
			i = prevpos;
			count = 0;
			while (i < pos)
			{
				//log_debug("%s\n", arg[i]);
				// output redirection
				if (strcmp(arg[i], ">") == 0)
				{
					if (i+1 < pos)
						strcpy(node->out, arg[++i]);
				}
				// input redirection
				else if (strcmp(arg[i], "<") == 0)
				{
					if (i+1 < pos)
						strcpy(node->in, arg[++i]);
				}
				else
				{
					strcpy(cmdarg[count], arg[i]);
					log_debug("cmdarg[%d] = %s\n", count, cmdarg[count]);
					count++;
				}
				i++;	
			}
			//log_debug("count:%d tokens:%d\n", count, node->ntokens);
			// set the remaining tokens to NULL
			for (i = count; i < node->ntokens; i++)
			{
				free(cmdarg[i]);
				cmdarg[i] = NULL;
			}
			prevpos = pos+1;
			// add this node to the list
			list_add(&node->list, head);
			if (arg[pos] == NULL)
				break;
		}
		pos++;
	}
	free_memory(arg, ntokens);
	return head;
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
	/*int i;
	  for (i = 0 ; i < ntokens; i++)
	  {
	  log_debug("%s\n", arg[i]);
	  }*/
	// fork to execute the command
	if (fork() == 0)
	{	
		// execvp() search $PATH to locate the bin file
		int ret = execvp(arg[0], arg);
		// print the error message
		if (ret < 0)
			//printf("error:%s\n", strerror(errno));
			perror("error");
		// free
		free_memory(arg, ntokens);
		exit(0);
	}
	// wait until the child process return
	wait(NULL);
}

void exec_cmd2(struct list_head *head)
{
	int forkstatus = 1;
	int i;
	int pnum = 0;
	struct list_head *plist;
	// count the number of the command
	list_for_each(plist, head)
		pnum++;
	// pipe descriptor
	int **fd = NULL;
	if (pnum > 1)
	{
		fd = (int**)malloc((pnum-1)*sizeof(int*));
		for (i = 0; i < pnum-1; i++)
		{
			fd[i] = (int*)malloc(2*sizeof(int)); 
			int flag= pipe(fd[i]);
			log_debug("pipe return:%d\n",flag);
			log_debug("fd[%d]:%d %d\n", i, fd[i][0], fd[i][1]);
			
		}
	}
	int idx = 0;
	// loop for every command
	list_for_each(plist, head)
	{
		struct node_cmd *node = list_entry(plist, struct node_cmd, list);	
		//log_debug("tokens:%d\n", node->ntokens);
	  	for (i = 0 ; i < node->ntokens; i++)
		{
			log_debug("%p %s %s %s\n", node, node->arg[i], node->in, node->out);
		}
		// fork to execute the command
		forkstatus = fork();
		if (forkstatus == 0)
		{
			// input
			if (strcmp(node->in, "prev") == 0)
			{
				close(STDIN_FILENO);
				dup2(fd[idx][0], STDIN_FILENO);
				close(fd[idx][1]);
				close(fd[idx][0]);
			}
			else if(strcmp(node->in, "stdin") != 0)
			{
				int fp = open(node->in, O_RDONLY);
				dup2(fp, STDIN_FILENO);
			}
			log_debug("%p input:%d\n", node, STDIN_FILENO);
			
			// output
			if (strcmp(node->out, "next") == 0)
			{
				close(STDOUT_FILENO);
				dup2(fd[idx-1][1], STDOUT_FILENO);
				close(fd[idx-1][0]);
				close(fd[idx-1][1]);
			}
			else if(strcmp(node->out, "stdout") != 0)
			{
				int fp = open(node->out, O_WRONLY);
				dup2(fp, STDOUT_FILENO);
			}
		        //execvp() search $PATH to locate the bin file
			int ret = execvp(node->arg[0], node->arg);
			
			// print the error message
			if (ret < 0)
			{
				//printf("error:%s\n", strerror(errno));
				perror("error");
				exit(-1);
			}
		}
		else
		{
			// free
			free_memory(node->arg,node->ntokens);
		}
		idx++;
	}
	
	// wait until the child process return
	if (forkstatus > 0)
	{
		// close the pipe in the parent process
		for (i = 0; i < pnum-1; i++)
		{
			close(fd[i][0]);
			close(fd[i][1]);
		}
		// wait until all the process return
		for (i = 0; i < pnum; i++)
			wait(NULL);	
	}
}


int main(int argc, char** argv)
{
	char** arg = NULL;
	int ntokens = 0;
	struct list_head *head;
	while(1)
	{
		arg = NULL;
		ntokens = 0;
		line_read = rl_gets();
		if (line_read && *line_read)
		{
			arg = lexer(line_read, &ntokens);
			//exec_cmd(arg, ntokens);
			//log_debug("arg : %p\n", arg);
			head = parser(arg, ntokens);
			exec_cmd2(head);
		}
	}
	return 0;
}
