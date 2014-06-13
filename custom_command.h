#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "bitmap.h"

#define MAX_CUST_CMD 32
#define CUST_CMD_NAME_LEN 32
#define WHY_STR_NUM 10
//#define DEFAULT_IMAGE_NAME "./images/lena64.bmp"
#define DEFAULT_IMAGE_NAME "./images/Jobs.bmp"

enum process_status
{
	Running = 0,
	Stopped,
	Done
};

char process_status_str[3][10] = {"Running", "Stopped", "Done"};

struct node_process
{
	int pid;
	enum process_status pstatus;
	struct list_head list;
};

struct list_head* jobs_list;	// all the background process
struct list_head* pid_list;	// all the foreground process


// function declaration
int yaush_cd(char* cmd, char** arg);
int yaush_exit(char* cmd, char** arg);
int yaush_about(char* cmd, char** arg);
int yaush_why(char* cmd, char** arg);
int yaush_jobs(char* cmd, char** arg);
int yaush_fg(char* cmd, char** arg);
int yaush_bg(char* cmd, char** arg);
int yaush_image(char* cmd, char** arg);


// variables
char cmd_list[MAX_CUST_CMD][CUST_CMD_NAME_LEN] = {"cd", "exit", "about", "why", "jobs", "fg", "bg", "image"};
int (*f[MAX_CUST_CMD])(char*, char**) = {yaush_cd, yaush_exit, yaush_about, yaush_why, yaush_jobs, yaush_fg, yaush_bg, yaush_image};


char why_str[WHY_STR_NUM][255] = {
	"I don't know why, my teacher o f C/UNIX course tell me to do so",
	"Don't ask!",
	"You know why.",
	"Because I'm a bad programmer.",
	"My TA and teacher assign too much homwork for me",
	"I'm crazy",
	"Oh no, too much howework for me!",
	"Go to the website https://github.com/FromHJ/yaush.git, and you will know why",
	"I'm just a stupid shell, why do you keep asking me why!",
	"type command: image, and there is a surprise"
};


/* yaush_cd(): change the current working directory
 * @params: cmd -- the command name, actually useless in this function, it's 
 just used to uniform the arguments of all custom functions
 arg -- the argumnets of the command, noted that arg[0] is the command name
 * @return: 0 if successful
 */
int yaush_cd(char* cmd, char** arg)
{
	int ret;
	// just the command 'cd' without any parameters
	// then change the current dir to home
	if (arg[1] == NULL)
	{
		char buf[255];
		sprintf(buf, "/home/%s", getenv("USER"));
		ret = chdir(buf);
	}
	else
		ret = chdir(arg[1]);
	if (ret < 0)
		perror("cd");
	return 0;
}

/* yaush_exit(): exit
 */
int yaush_exit(char* cmd, char** arg)
{
	exit(0);
	return 0;
}

/* yaush_about() : print the 'about' message
 */
int yaush_about(char* cmd, char** arg)
{
	printf("--------------------------Yaush--------------------\n");
	printf("-----------------Yet Another Unix Shell------------\n");
	printf("--author: Xinghao Chen\n");
	printf("--email : chenxh09thu@gmail.com\n");
	printf("--github: https://github.com/FromHJ/yaush.git\n");
	printf("--2014-06\n");
	printf("---------------------------------------------------\n");
	return 0;
}

/* yaush_why() : randomly print one message
 */
int yaush_why(char* cmd, char** arg)
{
	int idx = rand() % WHY_STR_NUM;
	printf("%s\n", why_str[idx]);	
	return 0;
}

/* yaush_jobs() : print the jobs list
 */
int yaush_jobs(char* cmd, char** arg)
{
	struct list_head *plist;
	int i = 0;
	list_for_each(plist, jobs_list)
	{
		struct node_process *node = list_entry(plist, struct node_process, list);
		printf("[%d]\tpid:%d\t%s\n", i, node->pid, process_status_str[node->pstatus]);
		i++;
	}
	return 0;
}

/* yaush_fg() : bring the specific task to the foreground
 * @return: the pid of the sprcific process
 */
int yaush_fg(char* cmd, char** arg)
{
	int idx = 0;
	int pid = 0;
	if (arg[1] != NULL)
	{
		idx = atoi(arg[1]);	
	}
	//printf("idx=%d\n",idx);
	if (idx <  -1)
		pid = -1;	
	struct list_head *plist;
	int i = 0;
	list_for_each(plist, jobs_list)
	{
		struct node_process *node = list_entry(plist, struct node_process, list);
		pid = node->pid;
		//printf("pid = %d\n",pid);
		if ( i >= idx)
			break;
		i++;
	}
	return pid;
}

/* yaush_bg() : bring the specific task to the background
 */
int yaush_bg(char* cmd, char** arg)
{
	int idx = 0;
	int pid = 0;
	if (arg[1] != NULL)
	{
		idx = atoi(arg[1]);	
	}
	//printf("idx=%d\n",idx);
	if (idx <  -1)
		pid = -1;	
	struct list_head *plist;
	int i = 0;
	list_for_each(plist, jobs_list)
	{
		struct node_process *node = list_entry(plist, struct node_process, list);
		pid = node->pid;
		//printf("pid = %d\n",pid);
		if ( i >= idx)
		{
			if (node->pstatus != Stopped)
				pid = 0;
			break;
		}
		i++;
	}
	return pid;
}

/* yaush_image() : show an image using ascii characters
 */
int yaush_image(char* cmd, char** arg)
{
	//int ret;
	// just the command 'cd' without any parameters
	// then change the current dir to home
	char path[255];
	if (arg[1] == NULL)
		strcpy(path, DEFAULT_IMAGE_NAME);
	else
		strcpy(path, arg[1]);
	U8 bitCountPerPix;  
	U32 width, height;  
	U8 *pdata = GetBmpData(&bitCountPerPix, &width, &height, path);
 	U8 BytePerPix = (bitCountPerPix) >> 3;
	U32 pitch = (width) * BytePerPix;
  
	if(pdata)  
	{
		int w, h;  
		for(h = 0; h < height;  h++)  
		{  
			for(w = 0; w < (width); w++)  
			{  
				int g = getGray(pdata[h*pitch + w*BytePerPix + 0], pdata[h*pitch + w*BytePerPix + 1], pdata[h*pitch + w*BytePerPix + 2]); 
				printf("%c ", toText(g)); 
			} 
			printf("\n");
		}  
	}
	//printf("%d %d %d\n", width, height, BytePerPix);	
	//if (ret < 0)
	//	perror("cd");
	return 0;
}

/* execute_cust_cmd(): execute an custom command 
   @params: cmd -- the command name
   arg -- the argumnets of the command, noted that arg[0] is the command name
 * @return: 0 is successful, or -1 if not found, or > 0 represents the pid
 */
int execute_cust_cmd(char* cmd, char** arg)
{
	int i;
	int flag = -1;
	for (i = 0; i < MAX_CUST_CMD; i++)
	{
		if (strlen(cmd_list[i]) > 0 && strcmp(cmd, cmd_list[i]) == 0)
		{
			flag = (*f[i])(cmd, arg);
			break;
		}
	}
	return flag;
}
