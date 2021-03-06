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
//#include "list.h"
#include "shortcut_signal.h"
#include "custom_command.h"

#define STRLEN	255
// A static variable for holding the line.
char *line_read = (char *)NULL;
//struct list_head* pid_list;

struct node_cmd 
{
	char** arg;		// the arguments list of the command
	int ntokens;
	char out[255];		// the output : "stdout" "next" "filename"
	char in[255];		// the input :  "stdin"  "prev" "filename"
	struct list_head list;
	int background;		// run in background (1) or not (0)
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
		{
			if (i > 0 && line_read[i-1] == '\\')
				continue;	
			nspace++;
		}
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
		if (offset == 0 && pos == 0)
			pstr = strchr(line_read + offset, delim);
		else
			pstr = strchr(line_read + pos + 1, delim);
		if (pstr == NULL)
			pos = strlen(line_read);
		else
		{
			pos = strlen(line_read) - strlen(pstr);
			//log_debug("%s\n", pstr);
		}
		// "\ " means a whitespace in the filename
		if (pos > 0 && line_read[pos-1] == '\\')
		{	
			//offset = pos+1;
			continue;
		}

		//log_debug("%d %d\n", offset, pos);
		if (pos > offset)
		{
			strncpy(arg[count], line_read + offset, pos - offset);
			arg[count][pos-offset] = '\0';
			// elimate the "\"
			char* ptmp = strchr(arg[count], '\\');
			if (ptmp != NULL)
			{
				int postmp = strlen(arg[count]) - strlen(ptmp);
				int i;
				for(i = postmp; i < strlen(arg[count]); i++)
					arg[count][i] = arg[count][i+1]; 	
			}
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

/* free_string()
 * @params: arg -- the pointer of the string list
 ntokens -- the number of string in the list
 @return: void
 */
void free_string(char** arg, int ntokens)
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

/* free_list()
 * @params: head -- the pointer of the list
 * @return: void
 */
void free_list(struct list_head *head)
{
	struct list_head *plist, *n;	
	list_for_each_safe(plist, n, head)
	{
		struct node_cmd *node = list_entry(plist, struct node_cmd, list);
		free_string(node->arg,node->ntokens);
		list_del(&node->list);
		node = NULL;
	}
	head = NULL;
}

/* parser()
 * @params: arg -- arguments list
 ntokens -- the ntokens of the list
 @return: the head of the list, every node of the list represents one command
 */
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
		if (arg[pos] == NULL || strcmp(arg[pos], "|") == 0 || strcmp(arg[pos], "&") == 0)
		{
			// if this is the last token
			//if (pos == ntokens - 1)
			//	pos += 1;
			// malloc for node
			struct node_cmd *node = (struct node_cmd*)malloc(sizeof(struct node_cmd));
			node->background = 0;
			// output redirection
			strcpy(node->out, "next");
			if (arg[pos] == NULL)
				strcpy(node->out, "stdout");
			else if (pos+1 < ntokens &&  strcmp(arg[pos], "&") == 0 && arg[pos+1] == NULL)
				strcpy(node->out, "stdout");
			// input redirection
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
			// are these commands running in background or not
			// only if the '&' appear in the end of the command, it takes effect.
			if (pos+1 < ntokens && arg[pos] != NULL && strcmp(arg[pos], "&") == 0 )// )// )// )// )// )// )//  && arg[pos+1] == NULL)
				node->background = 1;
			// add this node to the list
			list_add(&node->list, head);
			if (arg[pos] == NULL)
				break;
			if (pos+1 < ntokens && arg[pos] != NULL && strcmp(arg[pos], "&") == 0 && arg[pos+1] == NULL)
				break;
		}
		pos++;
	}
	free_string(arg, ntokens);
	return head;
}


/* exec_singlecmd()
   @params: path -- the path of the command 
   arg -- the arguments of the command
   ntokens -- the number of the tokens in arg (actually length(arg) == ntokens+1,
   because the last token is always set to NULL)
   @return: void
 */
void exec_singlecmd(char** arg, int ntokens)
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
		free_string(arg, ntokens);
		exit(0);
	}
	// wait until the child process return
	wait(NULL);
}

/* exec_multicmd()
   @params: head -- the head of the list, every node in the list represent a command
   @return: void
 */
void exec_multicmd(struct list_head *head)
{
	int forkstatus = 1;
	int i;
	int pnum = 0;
	int background = 0;
	struct list_head *plist;
	// count the number of the command
	list_for_each(plist, head)
	{
		struct node_cmd *node = list_entry(plist, struct node_cmd, list);	
		if (pnum == 0 && node->background == 1)
		{
			background = 1;
		}
		pnum++;
	}
	log_debug("background:%d\n", background);
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
	int cust = 0;
	int ret;
	// loop for every command
	list_for_each(plist, head)
	{
		struct node_cmd *node = list_entry(plist, struct node_cmd, list);	
		//log_debug("tokens:%d\n", node->ntokens);
		for (i = 0 ; i < node->ntokens; i++)
		{
			log_debug("%p %s %s %s %d\n", node, node->arg[i], node->in, node->out, node->background);
		}

		cust = execute_cust_cmd(node->arg[0], node->arg);
		log_debug("execute_cust_cmd return %d\n", cust);
		if (cust == 0)
			continue;
		else if (cust > 0)
		{
			// fg
			if (strcmp(node->arg[0],"fg") == 0)
			{
				// add it to the pid_list
				struct node_process *node = (struct node_process*)malloc(sizeof(struct node_process));
				node->pid = cust;
				list_add(&node->list, pid_list);
				// and remove from the jobs_list;
				struct list_head *plist, *n;
				list_for_each_safe(plist, n, jobs_list)
				{
					struct node_process *node = list_entry(plist, struct node_process, list);
					if (node->pid == cust)
					{
						list_del(&node->list);
						break;
					}
				}
				break;
			}
			// bg
			else
			{
				struct list_head *plist, *n;
				list_for_each_safe(plist, n, jobs_list)
				{
					struct node_process *node = list_entry(plist, struct node_process, list);
					if (node->pid == cust)
					{
						node->pstatus = Running;
						kill(node->pid, SIGCONT);
						break;
					}
				}
				background = 1;
				break;
			}
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
			}
			else if(strcmp(node->in, "stdin") != 0)
			{
				int fp = open(node->in, O_RDONLY);
				dup2(fp, STDIN_FILENO);
			}
			//log_debug("%p input:%d\n", node, STDIN_FILENO);

			// output
			if (strcmp(node->out, "next") == 0)
			{
				close(STDOUT_FILENO);
				dup2(fd[idx-1][1], STDOUT_FILENO);
			}
			else if(strcmp(node->out, "stdout") != 0)
			{
				int fp = open(node->out, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				dup2(fp, STDOUT_FILENO);
			}


			for (i = 0; i < pnum-1; i++)
			{
				close(fd[i][1]);
				close(fd[i][0]);
			}
			log_debug("%s pid: %d\n", node->arg[0], getpid());
			//execvp() search $PATH to locate the bin file
			ret = execvp(node->arg[0], node->arg);

			// print the error message
			if (ret < 0)
			{
				//printf("error:%s\n", strerror(errno));
				fprintf(stderr, "%s:%s\n", node->arg[0], strerror(errno));
				exit(-1);
			}
		}
		// in parent process: put every pid of non-background process into the list
		else if (background == 0)
		{
			struct node_process *node = (struct node_process*)malloc(sizeof(struct node_process));
			node->pid = forkstatus;
			node->pstatus = Running;
			log_debug("pid %d inqueue\n", forkstatus);
			list_add(&node->list, pid_list);
		}
		// in parent process: add background process into the jobs list
		else
		{
			struct node_process *node = (struct node_process*)malloc(sizeof(struct node_process));
			node->pid = forkstatus;
			node->pstatus = Running;
			list_add(&node->list, jobs_list);
		}
		idx++;
	}

	// wait until the child process return
	if (forkstatus > 0 && cust != 0)
	{
		// close the pipe in the parent process
		for (i = 0; i < pnum-1; i++)
		{
			close(fd[i][0]);
			close(fd[i][1]);
		}
		// wait until all the process return
		if (background == 0)
		{
			// wait the process in the pid_list
			while (!list_empty(pid_list))
			{
				int ret = wait(NULL);
				struct list_head *plist, *n;
				// search the pid:ret
				list_for_each_safe(plist, n, pid_list)
				{
					struct node_process *node = list_entry(plist, struct node_process, list);
					if (node->pid == ret)
					{
						log_debug("wait %d return %d %d\n", i, ret, node->pid);
						// delete this node
						list_del(&node->list);
						break;
					}
				}
				// update the jobs_list
				list_for_each(plist, jobs_list)
				{
					struct node_process *node = list_entry(plist, struct node_process, list);
					if (node->pid == ret)
					{
						node->pstatus = Done;
						break;
					}
				}
			}
		}

	}
}


int main(int argc, char** argv)
{
	char** arg = NULL;
	int ntokens = 0;
	struct list_head *head;
	struct list_head *plist, *n;
	// init list head for jobs_list
	jobs_list = (struct list_head*)malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(jobs_list);
	// init list head for pid_list
	pid_list = (struct list_head*)malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(pid_list);
	// signal - Ctrl-C
	struct sigaction act;
	sigfillset(&(act.sa_mask));
	act.sa_flags = SA_SIGINFO;
	act.sa_handler = handle_signals_ctrl_c;
	if (sigaction(SIGINT, &act, NULL) < 0) 
	{
		printf("failed to register interrupts with kernel\n");
	}
	// signal - Ctrl-Z
	struct sigaction act_z;
	sigfillset(&(act_z.sa_mask));
	act_z.sa_flags = SA_SIGINFO;
	act_z.sa_handler = handle_signals_ctrl_z;
	if (sigaction(SIGTSTP, &act_z, NULL) < 0) 
	{
		printf("failed to register interrupts with kernel\n");
	}
	// sigsetjmp
	if ( sigsetjmp( ctrlc_buf, 1 ) != 0 )
	{
		;
	}
	if ( sigsetjmp( ctrlz_buf, 1 ) != 0 )
	{
		log_debug("ctrlz sigsetjmp\n");
		struct list_head *plist, *n;
		// move the list in pid_list to jobs_list
		list_for_each_safe(plist, n, pid_list)
		{
			struct node_process *node = list_entry(plist, struct node_process, list);
			log_debug("pid:%d pstatus:%d\n", node->pid, node->pstatus);
			struct node_process *newnode = (struct node_process*)malloc(sizeof(struct node_process));
			newnode->pstatus = Stopped;
			newnode->pid = node->pid;
			list_add(&newnode->list, jobs_list);
			list_del(&node->list);
		}
		log_debug("jobs_list empty:%d\n", list_empty(jobs_list));
	}
	// loop
	while(1)
	{
		arg = NULL;
		ntokens = 0;
		// readline 
		line_read = rl_gets();
		// print the jobs that were done
		int i = 0;
		int ret;
		list_for_each_safe(plist, n, jobs_list)
		{
			struct node_process *node = list_entry(plist, struct node_process, list);
			ret = waitpid(node->pid, NULL, WNOHANG);
			if (ret == -1)
				node->pstatus = Done;
			if (node->pstatus == Done)
			{
				printf("[%d]\tpid:%d\t%s\n", i, node->pid, process_status_str[node->pstatus]);
				// delete this node
				list_del(&node->list);
			}
			i++;
		}
		// handle the input 
		if (line_read && *line_read)
		{
			arg = lexer(line_read, &ntokens);
			//exec_singlecmd(arg, ntokens);
			//log_debug("arg : %p\n", arg);
			head = parser(arg, ntokens);
			exec_multicmd(head);
			free_list(head);
		}
	}
	//free(pid_list);
	//pid_list = NULL;
	return 0;
}
