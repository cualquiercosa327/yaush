#include <signal.h>
#include <readline/readline.h>
#include <setjmp.h>
#include "log_debug.h"

sigjmp_buf ctrlc_buf;

void handle_signals(int signo)
{
	if (signo == SIGINT)
	{
		log_debug("You pressed Ctrl+C\n");
		//rl_catch_signals = 1;
		//rl_set_signals();
		//rl_crlf();
		//rl_clear_message ();
		//rl_on_new_line_with_prompt ();
		//rl_cleanup_after_signal();
		printf("\n");
		//rl_reset_line_state(); 
		//rl_on_new_line();
		siglongjmp(ctrlc_buf, 1);
	}
}
