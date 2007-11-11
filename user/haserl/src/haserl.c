/* --------------------------------------------------------------------------
 * core of haserl.cgi - a poor-man's php for embedded/lightweight environments
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
 * -----
 * The x2c() and unescape_url() routines were taken from
 *  http://www.jmarshall.com/easy/cgi/getcgi.c.txt
 *
 * The comments in that text file state:
 *
 ***  Written in 1996 by James Marshall, james@jmarshall.com, except
 ***  that the x2c() and unescape_url() routines were lifted directly
 ***  from NCSA's sample program util.c, packaged with their HTTPD.
 ***     For the latest, see http://www.jmarshall.com/easy/cgi/
 * -----
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
#include "h_script.h"
#include "h_bash.h"

#ifdef USE_LUA
#include "h_lua.h"
#endif


#define SHELL_DECL
#include "haserl.h"


#ifndef TEMPDIR
#define TEMPDIR "/tmp"
#endif

#ifndef MAX_UPLOAD_KB
#define MAX_UPLOAD_KB 2048
#endif

/* Refuse to disable the subshell */
#ifndef SUBSHELL_CMD
#define SUBSHELL_CMD "/bin/sh"
#endif

haserl_t global;

/* global shell execution function pointers.   These point to the actual functions
   that do the job, based on the language */

/*
 * Command line / Config file directives When adding a long option, make sure
 * to update the short_options as well
 */

struct option ga_long_options[] = {
    {"version", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"debug", no_argument, 0, 'D'},
    {"upload-limit", required_argument, 0, 'u'},
    {"upload-dir", required_argument, 0, 'U'},
    {"accept-all", no_argument, 0, 'a'},
    {"accept-none", no_argument, 0, 'n'},
    {"shell", required_argument, 0, 's'},
    {"silent", no_argument, 0, 'S'},
    {0, 0, 0, 0}
};

const char *gs_short_options = "vhDu:U:ans:S";

/*
 * enum tag_t { HTML, RUN, IF, ELSE, FI, ABORT, INCLUDE,  NOOP };
 */

/*
 * Convert 2 char hex string into char it represents
 * (from http://www.jmarshall.com/easy/cgi)
 */
char x2c(char *what)
{
    char digit;

    digit =
	(what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit +=
	(what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
    return (digit);
}

/*
 * unsescape %xx to the characters they represent
 */
/* Modified by Juris Feb 2007 */
void unescape_url(char *url)
{
    int i, j;
     for (i = 0, j = 0; url[j]; ++i, ++j) {
       if ((url[i] = url[j]) != '%') continue;
        if (!url[j + 1] || !url[j + 2]) break;
            url[i] = x2c(&url[j + 1]);
            j += 2;
        }
     url[i] = '\0';
 }




/*
 * allocate memory or die, busybox style.
 */
void *xmalloc(size_t size)
{
    void *buf;
    if ((buf = malloc(size)) == NULL) {
	die_with_message(NULL, NULL, g_err_msg[E_MALLOC_FAIL]);
    }
    memset(buf, 0, size);
    return buf;
}


/*
 * realloc memory, or die xmalloc style.
 */
void *xrealloc(void *buf, size_t size)
{
    if ((buf = realloc(buf, size)) == NULL) {
	die_with_message(NULL, NULL, g_err_msg[E_MALLOC_FAIL]);
    }
    return buf;
}


/*
 *   adds or replaces the "key=value" value in the env_list chain
 *   prefix is appended to the key (e.g. FORM_key=value)
 */
list_t *myputenv(list_t * cur, char *str, char *prefix)
{
    list_t *prev = NULL;
    size_t keylen;
    char *entry = NULL;

    entry = xmalloc(strlen(str) + strlen(prefix) + 1);
    if (strlen(prefix)) {
	memcpy(entry, prefix, strlen(prefix));
    }
    memcpy((char *) (entry + strlen(prefix)), str, strlen(str));

    keylen = (size_t) (index(entry, '=') - entry);

    if (keylen <= 0) {
	free(entry);
	return (NULL);
    }

    /* does the value already exist? */
    while (cur != NULL) {
	if (memcmp(cur->buf, entry, keylen + 1) == 0) {
	    entry[keylen] = '\0';
	    // unsetenv (entry);
	    entry[keylen] = '=';
	    free(cur->buf);
	    if (prev != NULL)
		prev->next = cur->next;
	    free(cur);
	    cur = prev;
	}			/* end if found a matching key */
	prev = cur;
	cur = (list_t *) cur->next;
    }				/* end if matching key */

    /* add the value to the end of the chain  */
    cur = xmalloc(sizeof(list_t));
    cur->buf = entry;
    // putenv (cur->buf);
    if (prev != NULL)
	prev->next = cur;

    return (cur);
}

/* free list_t chain */
void free_list_chain(list_t * list)
{
    list_t *next;

    while (list) {
	next = list->next;
	free(list->buf);
	free(list);
	list = next;
    }
}



/* readenv
 * reads the current environment and popluates our environment chain
 */

void readenv(list_t * env)
{
    extern char **environ;
    int count = 0;

    while (environ[count] != NULL) {
	myputenv(env, environ[count], global.nul_prefix);
	count++;
    }
}


/* CookieVars ()
 * if HTTP_COOKIE is passed as an environment variable,
 * attempt to parse its values into environment variables
 */
void CookieVars(list_t * env)
{
    char *qs;
    char *token;

    if (getenv("HTTP_COOKIE") != NULL) {
	qs = strdup(getenv("HTTP_COOKIE"));
    } else {
	return;
    }

	/** split on; to extract name value pairs */
    token = strtok(qs, ";");
    while (token) {
	// skip leading spaces
	while (token[0] == ' ') {
	    token++;
	}
	myputenv(env, token, global.var_prefix);
	token = strtok(NULL, ";");
    }
    free(qs);
}


/* SessionID
 *  Makes a uniqe SESSIONID environment variable for this script
 */

void sessionid(list_t * env)
{
    char session[29];

    sprintf(session, "SESSIONID=%x%x", getpid(), (int) time(NULL));
    myputenv(env, session, global.nul_prefix);
}

list_t *wcversion(list_t * env)
{
    char version[200];
    sprintf(version, "HASERLVER=%s", PACKAGE_VERSION);
    return (myputenv(env, version, global.nul_prefix));
}


void haserlflags(list_t * env)
{
    char buf[200];

    snprintf(buf, 200, "HASERL_UPLOAD_DIR=%s", global.uploaddir);
    myputenv(env, buf, global.nul_prefix);

    snprintf(buf, 200, "HASERL_UPLOAD_LIMIT=%lu", global.uploadkb);
    myputenv(env, buf, global.nul_prefix);

    snprintf(buf, 200, "HASERL_ACCEPT_ALL=%d", global.acceptall);
    myputenv(env, buf, global.nul_prefix);

    snprintf(buf, 200, "HASERL_SHELL=%s", global.shell);
    myputenv(env, buf, global.nul_prefix);

}

/*
 * Read cgi variables from query string, and put in environment
 */

int ReadCGIQueryString(list_t * env)
{
    char *qs;
    char *token;
    int i;

    if (getenv("QUERY_STRING") != NULL) {
	qs = strdup(getenv("QUERY_STRING"));
    } else {
	return (0);
    }

    /* change plusses into spaces */
    for (i = 0; qs[i]; i++) {
	if (qs[i] == '+') {
	    qs[i] = ' ';
	}
    };

	/** split on & and ; to extract name value pairs */

    token = strtok(qs, "&;");
    while (token) {
	unescape_url(token);
	myputenv(env, token, global.var_prefix);
	token = strtok(NULL, "&;");
    }
    free(qs);
    return (0);
}


/*
 * Read cgi variables from stdin (for POST queries)
 * (oh... and if its mime-encoded file upload, we save the
 * file to /tmp; and return the name of the tmp file
 * the cgi script is responsible for disposing of the tmp file
 */

int ReadCGIPOSTValues(list_t * env)
{
    char *qs;
    int content_length = 0;
    int i;
    char *token;


    if (getenv("CONTENT_LENGTH") == NULL) {
	/* no content length?  Trash this request! */

	/* If it was a POST request, that's an error.  Otherwise,
	   silently fail */
	if (strcasecmp(getenv("REQUEST_METHOD"), "POST") == 0) {
	    die_with_message(NULL, NULL,
			     "HTTP POST request did not specify a Content Length.");
	} else {
	    return (0);
	}
    } else {
	content_length = atoi(getenv("CONTENT_LENGTH"));
    }

    /* Allow 2MB content, unless they have a global upload set */
    if (content_length >
	(((global.uploadkb == 0) ? 2048 : global.uploadkb) *1024)) {
	/* finish reading the content */
	while (fread(&i, sizeof(int), 1, stdin) == 1);
	die_with_message(NULL, NULL,
			 "Attempted to send content larger than allowed limits.");
    }

    if (!(qs = malloc(content_length + 1))) {
	die_with_message(NULL, NULL,
			 "Unable to Allocate memory for POST content.");
    }

    /* set the buffer to null, so that a browser messing with less
       data than content_length won't buffer underrun us */
    memset(qs, 0, content_length + 1);

    if ((!fread(qs, content_length, 1, stdin) && (content_length > 0)
	 && !feof(stdin))) {
	die_with_message(NULL, NULL, "Unable to read from stdin");
    }
    if (getenv("CONTENT_TYPE")) {
	if (strncasecmp(getenv("CONTENT_TYPE"), "multipart/form-data", 19)
	    == 0) {
	    /* This is a mime request, we need to go to the mime handler */
	    i = ReadMimeEncodedInput(env, qs);
	    free(qs);
	    return (i);
	}
    }

    /* change plusses into spaces */
    for (i = 0; qs[i]; i++) {
	if (qs[i] == '+') {
	    qs[i] = ' ';
	}
    };

	/** split on & and ; to extract name value pairs */

    token = strtok(qs, "&;");
    while (token) {
	unescape_url(token);
	myputenv(env, token, global.var_prefix);
	token = strtok(NULL, "&;");
    }
    free(qs);
    return (0);
}

/*
 *  LineToStr - Scans char and replaces the first "\n" with a "\0";
 *  If it finds a "\r", it does that to; (fix DOS brokeNnes) returns
 *  the length of the string;
 */
int LineToStr(char *string, size_t max)
{
    size_t offset = 0;

    while ((offset < max) && (string[offset] != '\n')
	   && (string[offset] != '\r')) {
	offset++;
    }
    if (string[offset] == '\r') {
	string[offset] = '\0';
	offset++;
    }
    if (string[offset] == '\n') {
	string[offset] = '\0';
	offset++;
    }
    return (offset);
}


/*
 * ReadMimeEncodedInput - handles things that are mime encoded
 * takes a pointer to the input; returns 0 on success
 */

int ReadMimeEncodedInput(list_t * env, char *qs)
{
    char *boundary;
    char *ct;
    int i;
    int datastart;
    size_t cl;
    size_t offset;
    char *envname;
    char *filename;
    char *ptr;
    int line;
    char *tmpname;
    int fd;
    token_t *curtoken;

    curtoken = global.uploadlist;

    /* we should only get here if the content type was set. Segfaults happen
       if Content_Type is null */

    if (getenv("CONTENT_LENGTH") == NULL) {
	/* No content length?! */
	die_with_message(NULL, NULL,
			 "No Content Length in HTTP Header from client");
    }

    cl = atoi(getenv("CONTENT_LENGTH"));

    /* we do this 'cause we can't mess with the real env. variable - it would
     * overwrite the environment - I tried.
     */
    i = strlen(getenv("CONTENT_TYPE")) + 1;
    ct = malloc(i);
    if (ct) {
	memcpy(ct, getenv("CONTENT_TYPE"), i);
    } else {
	return (-1);
    }

    i = (int) NULL;
    if (ct != NULL) {
	while (i < strlen(ct) && (strncmp("boundary=", &ct[i], 9) != 0)) {
	    i++;
	}
    }
    if (i == strlen(ct)) {
	/* no boundary information found */
	free(ct);
	die_with_message(NULL, NULL, "No Mime Boundary Information Found");
    }
    boundary = &ct[i + 7];
    /* add two leading -- to the boundary */
    boundary[0] = '-';
    boundary[1] = '-';

    /* begin the big loop.  Look for:
       --boundary
       Content-Disposition: form-data;  name="......."
       ....
       <blank line>
       content
       --boundary
       Content-Disposition: form-data; name="....." filename="....."
       ...
       <blank line>
       --boundary--
       eof
     */

    offset = 0;
    while (offset < cl) {
	/* first look for boundary */
	while ((offset < cl)
	       && (memcmp(&qs[offset], boundary, strlen(boundary)))) {
	    offset++;
	}
	/* if we got here and we ran off the end, its an error          */
	if (offset >= cl) {
	    free(ct);
	    die_with_message(NULL, NULL, "Malformed MIME Encoding");
	}
	/* if the two characters following the boundary are --,         */
	/* then we are at the end, exit                                 */
	if (memcmp(&qs[offset + strlen(boundary)], "--", 2) == 0) {
	    offset += 2;
	    break;
	}
	/* find where the offset should be */
	line = LineToStr(&qs[offset], cl - offset);
	offset += line;

	/* Now we're going to look for content-disposition              */
	line = LineToStr(&qs[offset], cl - offset);
	if (strncasecmp(&qs[offset], "Content-Disposition", 19) != 0) {
	    /* hmm... content disposition was not where we expected it  */
	    /* Be kind to Opera 9.10 and just ignore this block		*/
	    continue;
		/*	
	    free(ct);
	    die_with_message(NULL, NULL, "Content-Disposition Missing");
		*/
	}
	/* Found it, so let's go find "name="                           */
	if (!(envname = strstr(&qs[offset], "name="))) {
	    /* now name= is missing?!                               */
	    free(ct);
	    die_with_message(NULL, NULL,
			     "Content-Disposition missing name tag");
	} else {
	    envname += 6;
	}
	/* is there a filename tag?                                     */
	if ((filename = strstr(&qs[offset], "filename=")) != NULL) {
	    filename += 10;
	} else {
	    filename = NULL;
	}
	/* make envname and filename ASCIIZ                             */
	i = 0;
	while ((envname[i] != '"') && (envname[i] != '\0')) {
	    i++;
	}
	envname[i] = '\0';
	if (filename) {
	    i = 0;
	    while ((filename[i] != '"') && (filename[i] != '\0')) {
		i++;
	    }
	    filename[i] = '\0';
	}
	offset += line;
	/* Ok, by some miracle, we have the name; let's skip till we    */
	/* come to a blank line                                         */
	line = LineToStr(&qs[offset], cl - offset);
	while (strlen(&qs[offset]) > 1) {
	    offset += line;
	    line = LineToStr(&qs[offset], cl - offset);
	}
	offset += line;
	datastart = offset;
	/* And we go back to looking for a boundary */
	while ((offset < cl)
	       && (memcmp(&qs[offset], boundary, strlen(boundary)))) {
	    offset++;
	}
	/* strip [cr] lf */
	if ((qs[offset - 1] == '\n') && (qs[offset - 2] == '\r')) {
	    offset -= 2;
	} else {
	    offset -= 1;
	}
	qs[offset] = 0;
	/* ok, at this point, we know where the name is, and we know    */
	/* where the content is... we have to do one of two things      */
	/* based on whether its a file or not                           */
	if (filename == NULL) {	/* its not a file, so its easy            */
	    /* just jam the content after the name          */
	    memcpy(&envname[strlen(envname) + 1], &qs[datastart],
		   offset - datastart + 1);
	    envname[strlen(envname)] = '=';
	    myputenv(env, envname, global.var_prefix);
	} else {		/* handle the fileupload case           */
	    if (offset - datastart) {	/* only if they uploaded */
		if (global.uploadkb == 0) {
		    die_with_message(NULL, NULL,
				     "File uploads not allowed here.");
		}
		/*  stuff in the filename */
		ptr =
		    calloc(sizeof(char),
			   strlen(envname) + strlen(filename) + 2 + 5);
		sprintf(ptr, "%s_name=%s", envname, filename);
		myputenv(env, ptr, global.var_prefix);
		free(ptr);

		tmpname = xmalloc(strlen(global.uploaddir) + 8);
		strcpy(tmpname, global.uploaddir);
		strcat(tmpname, "/XXXXXX");
		fd = mkstemp(tmpname);
		if (fd == -1) {
		    die_with_message(NULL, NULL,
				     "Unable to open temp file");
		}
		write(fd, &qs[datastart], offset - datastart);
		close(fd);
		ptr =
		    calloc(sizeof(char),
			   strlen(envname) + strlen(tmpname) + 2);

		sprintf(ptr, "%s=%s", envname, tmpname);
		myputenv(env, ptr, global.var_prefix);

		/* add this filename to the list */
		curtoken =
		    push_token_on_list(curtoken, NULL, tmpname,
				       strlen(tmpname) + 1);
		printf("pushed %s on list\n", tmpname);
		if (global.uploadlist == NULL)
		    global.uploadlist = curtoken;

		free(ptr);
		/* don't free - we free in the unlink */
		/* free (tmpname); */
	    }
	}
    }
    free(ct);
    return (0);
}


int parseCommandLine(int argc, char *argv[])
{
    int c;
    int option_index = 0;

    /* set optopt and optind to 0 to reset getopt_long -
     * we may call it multiple times
     */
    optopt = 0;
    optind = 0;

    while ((c = getopt_long(argc, argv, gs_short_options,
			    ga_long_options, &option_index)) != -1) {
	switch (c) {
	case 'D':
	    global.debug = TRUE;
	    break;
	case 's':
	    global.shell = optarg;
	    break;
	case 'S':
	    global.silent = TRUE;
	    break;
	case 'u':
	    if (optarg) {
		global.uploadkb = atoi(optarg);
	    } else {
		global.uploadkb = MAX_UPLOAD_KB;
	    }
	    break;
	case 'a':
	    global.acceptall = TRUE;
	    break;
	case 'n':
	    global.acceptall = NONE;
	    break;
	case 'U':
	    global.uploaddir = optarg;
	    break;
	case 'v':
	case 'h':
	    printf("This is " PACKAGE_NAME " version " PACKAGE_VERSION ""
		   "(http://haserl.sourceforge.net)\n");
	    exit(0);
	    break;
	}
    }
    return (optind);
}

int BecomeUser(uid_t uid, gid_t gid)
{
    /* This silently fails if it doesn't work */
    setgid(gid);
    setuid(uid);
    return (0);
}

/*
 * Assign default values to the global structure
 */

void assignGlobalStartupValues()
{
    global.uploadkb = 0;	/* how big an upload do we allow (0 for none)   */
    global.shell = SUBSHELL_CMD;	/* The shell we use                             */
    global.silent = FALSE;	/* We do print errors if we find them           */
    global.uploaddir = TEMPDIR;	/* where to upload to                           */
    global.debug = FALSE;	/* Not int debug mode.                          */
    global.acceptall = FALSE;	/* don't allow POST data for GET method         */
    global.uploadlist = NULL;	/* we don't have any uploaded files             */
    global.var_prefix = HASERL_VAR_PREFIX;
    global.nul_prefix = "";

}


void unlink_uploadlist()
{
    token_t *me;
    me = global.uploadlist;
    while (me) {
	unlink(me->buf);
	free(me->buf);
	me = me->next;
    }

}



/*-------------------------------------------------------------------------
 *
 * Main
 *
 *------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{

    token_t *tokenchain;
    script_t *scriptchain;

    int retval = 0;
    char *filename = NULL;

    argv_t *av = NULL;
    char **av2;

    int command;
    int count;

    buffer_t script_text;
    list_t *env = NULL;

    assignGlobalStartupValues();
    buffer_init(&script_text);



    /* if more than argv[1] and argv[1] is not a file */
    switch (argc) {
    case 1:
	/* we were run, instead of called as a shell script */
	puts("This is " PACKAGE_NAME " version " PACKAGE_VERSION "\n"
	     "This program runs as a cgi interpeter, not interactively\n"
	     "Please see:  http://haserl.sourceforge.net");
	return (0);
	break;
    case 2:
	filename = argv[1];
	break;
    default:			/* more than two */
	command = argc_argv(argv[1], &av, "");

	/* and we need to add the original argv[0] command for optarg to work */
	av2 = xmalloc(sizeof(char *) * (command + 2));
	av2[0] = argv[0];
	for (count = 0; count < command; count++) {
	    av2[count + 1] = av[count].string;
	}
	parseCommandLine(command + 1, av2);
	argv[1] = av[1].string;
	free(av);
	free(av2);
	filename = argv[2];
	break;
	/* we silently ignore 3,4,etc. */
    }


    scriptchain = load_script(filename, NULL);
/* drop permissions */
    BecomeUser(scriptchain->uid, scriptchain->gid);

    /* populate the function pointers based on the shell selected */

    /* default to "Shell" */
    shell_exec = &bash_exec;
    shell_echo = &bash_echo;
    shell_eval = &bash_eval;
    shell_setup = &bash_setup;
    shell_doscript = &bash_doscript;
    shell_destroy = &bash_destroy;


/* lua is possible */
    if (strcmp(global.shell, "lua") == 0) {
#ifdef USE_LUA
	shell_exec = &lua_exec;
	shell_echo = &lua_echo;
	shell_eval = &lua_eval;
	shell_setup = &lua_setup;
	shell_doscript = &lua_doscript;
	shell_destroy = &lua_destroy;
	global.var_prefix = "FORM.";
	global.nul_prefix = "ENV.";
#else
	die_with_message(NULL, NULL, "Lua is not enabled.");
#endif
    }

/* Read the current environment into our chain */
    env = wcversion(env);
    readenv(env);
    sessionid(env);
    haserlflags(env);

    tokenchain = build_token_list(scriptchain, NULL);
    preprocess_token_list(tokenchain);


/* Read the request data */
    if (global.acceptall != NONE) {
	/* If we have a request method, and we were run as a #! style script */
	CookieVars(env);
	if (getenv("REQUEST_METHOD")) {
	    if (strcasecmp(getenv("REQUEST_METHOD"), "GET") == 0) {
		if (global.acceptall == TRUE)
		    ReadCGIPOSTValues(env);
		ReadCGIQueryString(env);
	    }

	    if (strcasecmp(getenv("REQUEST_METHOD"), "POST") == 0) {
		if (global.acceptall == TRUE)
		    retval = ReadCGIQueryString(env);
		retval = ReadCGIPOSTValues(env);
	    }
	}
    }

    /* build a copy of the script to send to the shell */
    process_token_list(&script_text, tokenchain);

    /* run the script */
    if (global.debug == TRUE) {
	if (getenv("REQUEST_METHOD"))
	    write(1, "Content-Type: text/plain\n\n", 26);
	write(1, script_text.data, script_text.ptr - script_text.data);
    } else {
	shell_setup(global.shell, env);
	shell_doscript(&script_text, scriptchain->name);
	shell_destroy();
    }

    /* destroy the script */
    buffer_destroy(&script_text);

    if (global.uploadlist) {
	unlink_uploadlist();
	free_token_list(global.uploadlist);
    }

    free_list_chain(env);
    free_token_list(tokenchain);
    free_script_list(scriptchain);
    return (0);
}
