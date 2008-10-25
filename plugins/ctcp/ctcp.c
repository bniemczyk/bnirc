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

#include "plugin.h"

static int	hook	( const char *who, const char *victim, const char *cdata, const char *server )
{
	if(!cdata)
		return 1;

	if(!who)
		return 1;

	if(!victim)
		return 1;

	char 	**argv;
	int	argc;
	char	*buf;
	channel_t *ch = NULL;

	make_argv(&argc, &argv, cdata);

	if(argc < 1)
		return 1;

	argv[0] = string_lc(argv[0]);

	if(!strcmp(argv[0], "version")) {

		if(argc > 1) {
			cio_out("%s %s\n", who, cdata);
			return 0;
		}

		cio_out("recieved VERSION request from %s\n", who);
		if(!string_option("version_string"))
			irc_out("NOTICE %s :%cVERSION %s%c\n", who, 1, PACKAGE "-" VERSION, 1);
		else
			irc_out("NOTICE %s :%cVERSION %s%c\n", who, 1, string_option("version_string"), 1);

	} else if(!strcmp(argv[0], "action")) {

		if(victim[0] != '#')
			return 1;

		ch = find_channel(victim, server);
		if(!ch)
			return 1;

		buf = strstr_nc(cdata, "action");

		while(buf[0] != ' ' && buf[0] != '\0')
			buf++;
		while(buf[0] == ' ')
			buf++;

		cwio_out(ch->win, "%s ", timestamp());
		if(ch->is_active(ch))
			wio_colored_out(ch->win, USER_WHITE, "%s ", who);
		else
			wio_colored_out(ch->win, USER_WHITE, "%s/%s ", who, ch->name);
		cwio_out(ch->win, "%s\n", buf);

	} else if(!strcmp(argv[0], "time")) {

		if(argc > 1)
			cio_out("%s %s\n", who, cdata);
		else {
			irc_out("NOTICE %s :%cTIME %s%c\n", who, 1, timestamp(), 1);
		}

	} else {
		buf = malloc(strlen(who) + strlen("ctcp()") + 1);
		sprintf(buf, "ctcp(%s)", who);
		user_t *u = find_user(buf, server);
		if(u != NULL)
			return -1;
		u->privmsg_print(u, victim, cdata);
		/* channel_msg(buf, victim, server, (char *)cdata); */
		free(buf);
	}

	return 0;
}

static plugin_t	plugin	= {
	.name		= "ctcp",
	.version	= "0.0.1",
	.description	= "handles all ctcp requests from other users",
	.ctcp_in_hook	= hook
};

REGISTER_PLUGIN(plugin, ctcp);
