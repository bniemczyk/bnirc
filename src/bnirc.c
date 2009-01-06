/*
 * Copyright (C) 2004 Brandon Niemczyk
 * 
 * DESCRIPTION:
 *
 * CHANGELOG:
 * 
 * LICENSE: GPL Version 2
 *
 * TODO:
 *
 */

#ifndef NDEBUG
#ifndef __cplusplus
extern int printf(const char *, ...);
#define DPRINT printf
#else
extern "C" int printf(const char *, ...);
#define DPRINT ::printf
#endif
#else
#define DPRINT(x,...)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#include <locale.h>
#include "irc.h"

static char *profile = STARTUPDIR "default";
static char *startup = ".bnirc";
static int default_startup = 1;

/**********************************************
 * valid options are:
 *  -p <profile name>
 *  -s <startup file>
 *  -h
 **********************************************/
static int do_options(int argc, char *argv[])
{
	int c;
	while (1) {
		c = getopt(argc, argv, "p:s:h");

		if (c == -1)
			break;

		switch (c) {
		case 'p':
			profile =
			    malloc(strlen(STARTUPDIR) + strlen(optarg) + 1);
			sprintf(profile, STARTUPDIR "%s", optarg);
			break;
		case 's':
			startup = malloc(strlen(optarg) + 1);
			strcpy(startup, optarg);
			default_startup = 0;
			break;
		case 'h':
			printf
			    ("usage: %s [-p <profile>] [-s <startup file>] [-h]\n",
			     argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	return 0;
}

#ifdef HAVE_BACKTRACE
#ifdef backtrace
#undef backtrace
#endif
/************************************************************************************************
 * i check for the backtrace_symbols function in my configure.ac with:
 *    AC_CHECK_FUNC(backtrace_symbols, [AC_DEFINE(HAVE_BACKTRACE, 1, [backtrace_symbols check])])
 ************************************************************************************************/
static void print_backtrace(int fd)
{
	const char start[] = "BACKTRACE ------------\n";
	const char end[] = "----------------------\n";

	void *bt[128];
	int bt_size;
	char **bt_syms;
	int i;

	bt_size = backtrace(bt, 128);
	bt_syms = backtrace_symbols(bt, bt_size);
	/* stderr is file descriptor 2 */
	write(fd, start, strlen(start));
	for (i = 1; i < bt_size; i++) {
		write(fd, bt_syms[i], strlen(bt_syms[i]));
		write(fd, "\n", 1);
	}
	write(fd, end, strlen(end));
}
#else
#define print_backtrace(x)
#endif

/**********************************************
 * SIGSEGV handler
 **********************************************/

static void stdin_poll_func(void *ignoreme)
{
	char *buf = io_get_input();
	if(buf) {
		plugin_input_hook(buf);
		parse_input_string(buf);
	}
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

	char *ts;
	irc_poll_t stdin_poll = {
		.callback = stdin_poll_func,
		.fd = fileno(stdin),
		.in = stdin
	};

	register_poll(&stdin_poll);
	// signal(SIGCHLD, SIG_IGN);
	do_options(argc, argv);


	use_comments = 1;

	cio_out(PACKAGE "-" VERSION " - smack my BitchX up\n");
	cio_out("email bug reports to: bniemczyk@gmail.com\n");
	ts = timestamp();
	cio_out("starting at %s\n", ts);
	free(ts);

	/**********************************************
	 * begin the polling thread
	 **********************************************/
	// start_polling();

	char *buf = NULL;
	FILE *init_f = NULL;
	char *home = getenv("HOME");
	char *init_fname = NULL;
	size_t tmp;

	/**********************************************
	 * this is need to pre-initialize /set
	 **********************************************/
	if (!bool_option("verbose")) {
		cio_out("setting up /set command\n");
	}

	/**********************************************
	 * parse the profile and ~/.bnirc if it exists
	 **********************************************/
	if (home) {

		cio_out("HOME: %s\n", home);
		tmp = strlen(home);

		init_fname = malloc(strlen(profile) + 1);
		sprintf(init_fname, profile);
		init_f = bnirc_fopen(init_fname, "r");
		if (init_f) {
			tmp = 0;
			while (bnirc_getline(&buf, &tmp, init_f) != -1) {
				plugin_input_hook(buf);
				parse_input_string(buf);
			}
			fclose(init_f);
			cio_out("%s parsed\n", init_fname);
		} else {
			io_colored_out(USER_RED,
				       "%s cannot be opened or doesn't exist\n",
				       init_fname);
		}

		if (default_startup) {
			init_fname = malloc(strlen(home) + strlen(startup) + 1);
			sprintf(init_fname, "%s/%s", home, startup);
		} else {
			init_fname = startup;
		}
		init_f = bnirc_fopen(init_fname, "r");
		if (init_f) {
			tmp = 0;
			while (bnirc_getline(&buf, &tmp, init_f) != -1) {
				plugin_input_hook(buf);
				parse_input_string(buf);
			}
			fclose(init_f);
			cio_out("%s parsed\n", init_fname);
		} else {
			io_colored_out(USER_RED,
				       "%s cannot be opened or doesn't exist\n",
				       init_fname);
		}

	} else {
		io_colored_out(USER_RED, "no home directory found!\n");
	}

	/**********************************************
	 * main loop
	 **********************************************/
	use_comments = 0;
	while (!irc_shutdown) {
		/***************************************************************
		 * run loops calls the polling func, which sleeps,
		 * so we don't have to do that in here, we are nice
		 * anyways ;)
		 ***************************************************************/
		run_loop_hooks();
	}

	/**********************************************
	 * do some cleaning up
	 **********************************************/
	unload_all_plugins();

	return EXIT_SUCCESS;
}
