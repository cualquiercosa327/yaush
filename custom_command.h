#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CUST_CMD 32
#define CUST_CMD_NAME_LEN 32

// function declaration
void yaush_cd(char* cmd, char** arg);
void yaush_exit(char* cmd, char** arg);
void yaush_about(char* cmd, char** arg);

// variables
char cmd_list[MAX_CUST_CMD][CUST_CMD_NAME_LEN] = {"cd", "exit", "about"};
void (*f[MAX_CUST_CMD])(char*, char**) = {yaush_cd, yaush_exit, yaush_about};

 
/* yaush_cd(): change the current working directory
 * @params: cmd -- the command name, actually useless in this function, it's 
                   just used to uniform the arguments of all custom functions
	    arg -- the argumnets of the command, noted that arg[0] is the command name
 * @return: 0 is successful, or -1 if errors occur
 */
void yaush_cd(char* cmd, char** arg)
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
}

void yaush_exit(char* cmd, char** arg)
{
	exit(0);
}

void yaush_about(char* cmd, char** arg)
{
	printf("--------------------------Yaush--------------------\n");
	printf("-----------------Yet Another Unix Shell------------\n");
	printf("--author: Xinghao Chen\n");
	printf("--email : chenxh09thu@gmail.com\n");
	printf("--github: https://github.com/FromHJ/yaush.git\n");
	printf("--2014-06\n");
	printf("---------------------------------------------------\n");
}

int execute_cust_cmd(char* cmd, char** arg)
{
	int i;
	int flag = 0;
	for (i = 0; i < MAX_CUST_CMD; i++)
	{
		if (strlen(cmd_list[i]) > 0 && strcmp(cmd, cmd_list[i]) == 0)
		{
			(*f[i])(cmd, arg);
			flag = 1;
			break;
		}
	}
	return flag;
}
