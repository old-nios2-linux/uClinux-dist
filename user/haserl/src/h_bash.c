/*-----------------------------------------------------------------
 * haserl functions specific to a bash/ash/dash shell
 * Copyright (c) 2003,2004    Nathan Angelacos (nangel@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 ------------------------------------------------------------------------- */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "common.h"
#include "h_error.h"
#include "h_bash.h"
#include "h_script.h"
#include "haserl.h"

/* Local subshell variables */
int subshell_pipe[4];
int subshell_pid;

void bash_sig_handler(int signo)
{
    switch (signo) {
    case SIGCHLD:
	exit(-1);
	break;
    default:
	break;
    }
}

void bash_open(char *shell)
{
    int retcode = 0;
    int count;
    argv_t *argv;
    char *av[20];

    if (shell == NULL)
	return;

    retcode = pipe(&subshell_pipe[PARENT_IN]);
    if (retcode == 0) {
	retcode = pipe(&subshell_pipe[PARENT_CTRLIN]);
    }
    if (retcode == 0) {
	signal(SIGCHLD, bash_sig_handler);	/* die if the child dies early */
	subshell_pid = fork();
	if (subshell_pid == -1) {
	    die_with_message(NULL, NULL, g_err_msg[E_SUBSHELL_FAIL]);
	}

	if (subshell_pid == 0) {
	    /* I'm the child */
	    signal(SIGCHLD, SIG_IGN);
	    dup2(subshell_pipe[PARENT_IN], STDIN_FILENO);
	    close(subshell_pipe[PARENT_IN]);
	    close(subshell_pipe[PARENT_OUT]);	/* we won't write on this pipe */
	    if (subshell_pipe[CHILD_CTRLOUT] != 5) {
		/* force the outbound control pipe to be fd 5 */
		dup2(subshell_pipe[CHILD_CTRLOUT], 5);
		close(subshell_pipe[CHILD_CTRLOUT]);
	    }
	    count = argc_argv(shell, &argv, "");
	    if (count > 19) {
		/* over 20 command line args, silently truncate */
		av[19] = "\0";
		count = 19;
	    }
	    while (count >= 0) {
		av[count] = argv[count].string;
		count--;
	    }
	    execv(argv[0].string, av);
	    free(argv);

	    /* if we get here, we had a failure, so close down all the pipes */
	    die_with_message(NULL, NULL, g_err_msg[E_SUBSHELL_FAIL]);
	} else {
	    /* I'm parent, move along please */
	}
    }

    /* control should get to this point only in the parent.
     */
}

void bash_destroy(void)
{
    int x;

    signal(SIGCHLD, SIG_IGN);
    /* closing the pipe should close the subshell if it is a bash-like shell */
    for (x = 0; x < 4; x++) {
	close(subshell_pipe[x]);
    }

    /* but kill it just to make sure */
    if (subshell_pid)
	kill(subshell_pid, 9);

}


void bash_exec(buffer_t * buf, char *str)
{
    buffer_add(buf, str, strlen(str));
    return;
}

/* Run the echo command in a subshell */
void bash_echo(buffer_t * buf, char *str, size_t len)
{
/* limits.h would tell us the ARG_MAX characters we COULD send to the echo command, but
 * we will take the (ancient) POSIX1 standard of 4K, subtract 1K from it and use that
 * as the maxmimum.    The Linux limit appears to be 128K, so 3K will fit. */

    const char echo_start[] = "echo -n '";
    const char echo_quote[] = "'\\''";
    const char echo_end[] = "'\n";
    const size_t maxlen = 3096;
    size_t pos;

    if (len == 0)
	return;
    pos = 0;

    buffer_add(buf, echo_start, strlen(echo_start));
    while (pos < len) {
	if (str[pos] == '\'')
	    buffer_add(buf, echo_quote, strlen(echo_quote));
	else
	    buffer_add(buf, str + pos, 1);
	pos++;
	if ((pos % maxlen) == 0) {
	    buffer_add(buf, echo_end, strlen(echo_end));
	    buffer_add(buf, echo_start, strlen(echo_start));
	}
    }
    buffer_add(buf, echo_end, strlen(echo_end));
}


/* do an evlaution in a subshell */
void bash_eval(buffer_t * buf, char *str, size_t len)
{
    const char echo_start[] = "echo -n ";
    const char echo_end[] = "\n";
    if (len == 0)
	return;

    buffer_add(buf, echo_start, strlen(echo_start));
    buffer_add(buf, str, len);
    buffer_add(buf, echo_end, strlen(echo_end));
}


void bash_setup(char *shell, list_t * env)
{
    list_t *next;

    /* populate the environment */
    while (env) {
	next = env->next;
	putenv(env->buf);
	env = next;
    }

    /* start the subshell */
    bash_open(shell);
}


void bash_doscript(buffer_t * script, char *name)
{
    const char postfix[] = "\necho $? >&5\n";
    char result[20];
    int count = 0;
    int handle = subshell_pipe[PARENT_OUT];


    /* dump the script to the subshell */
    write(handle, script->data, script->ptr - script->data);

    /* write the postfix */
    write(subshell_pipe[PARENT_OUT], postfix, strlen(postfix));

    /* wait for the result to come back */
    memset(result, 0, 20);
    while ((result[count] != '\n') && (count < 20)) {
	read(subshell_pipe[PARENT_CTRLIN], &result[count], 1);
	if (result[count] != '\n')
	    count++;
    }

}
