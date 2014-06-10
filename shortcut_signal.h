#include <signal.h>
#include <unistd.h>
#include <readline/readline.h>
#include <setjmp.h>
#include "log_debug.h"

sigjmp_buf ctrlc_buf;
sigjmp_buf ctrlz_buf;

void handle_signals_ctrl_c(int signo)
{
	if (signo == SIGINT)
	{
		log_debug("You pressed Ctrl+C. pid:%d\n", getpid());
		printf("\n");
		siglongjmp(ctrlc_buf, 1);
	}
}

void handle_signals_ctrl_z(int signo)
{
	if (signo == SIGTSTP)
	{
		log_debug("You pressed Ctrl+Z. pid:%d\n", getpid());
		printf("\n");
		siglongjmp(ctrlz_buf, 1);
	}
}
