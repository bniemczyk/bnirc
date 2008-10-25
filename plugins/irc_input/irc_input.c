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

static int connect_hook(int argc, char *argv[])
{
	int buf;

	if (argc == 1) {
		cio_out("current server is: %s\n",
			server_name != NULL ? server_name : "(none)");
		return 0;
	}

	if (argc < 3) {
		irc_connect(6667, argv[1]);
	} else {
		buf = atoi(argv[2]);
		irc_connect(buf, argv[1]);
	}

	return 0;
}

static int nick_hook(int argc, char *argv[])
{
	irc_nick(argv[1]);
	return 0;
}

static int join_hook(int argc, char *argv[])
{
	assert(argc == 2);
	/**********************************************
	 * allow something like:
	 *  /join #channel1 #channel2
	 **********************************************/
	if (strstr(argv[1], " ")) {
		int i;
		char **tmp_argv;
		int tmp_argc;
		char *orig = argv[1];
		make_argv(&tmp_argc, &tmp_argv, argv[1]);
		for (i = 0; i < tmp_argc; i++) {
			argv[1] = tmp_argv[i];
			join_hook(argc, argv);
		}
		free_argv(tmp_argc, tmp_argv);
		argv[1] = orig;
		return 0;
	}

	channel_t *tmp;

	tmp = find_channel(argv[1], irc_pick_server());

	if (tmp == NULL)
		tmp = new_channel(argv[1], irc_pick_server());

	tmp->set_window(tmp, active_window <= window_max
			&& active_window > 0 ? active_window : 0);
	tmp->join(tmp);
	return 0;
}

static int msg_hook(int argc, char *argv[])
{
	irc_out("PRIVMSG %s :%s\n", argv[1], argv[2]);
	// channel_msg(irc_server_nick(irc_pick_server()), argv[1], irc_pick_server(), argv[2]);
	user_t *u =
	    find_user(irc_server_nick(irc_pick_server()), irc_pick_server());
	if (u == NULL)
		return -1;
	assert(u->flags & USER_IS_SELF);
	u->privmsg_print(u, argv[1], argv[2]);
	return 0;
}

static int me_hook(int argc, char *argv[])
{
	irc_action(argv[1]);
	return 0;
}

static int ctcp_hook(int argc, char *argv[])
{
	irc_ctcp(argv[1], argv[2]);
	return 0;
}

static int ignore_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	u->flags |= USER_IGNORE;
	return 0;
}

static int unignore_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	u->flags &= ~USER_IGNORE;
	return 0;
}

static int nignore_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	u->flags |= USER_IGNORE_NOTICES;
	return 0;
}

static int unnignore_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	u->flags &= ~USER_IGNORE_NOTICES;
	return 0;
}

static int huser_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	switch (argc) {
	case 2:
		u->flags &= ~ALL_USER_COLORS;
		break;
	case 3:
		u->flags &= ~ALL_USER_COLORS;
		if (!strcasecmp(argv[2], "red"))
			u->flags |= USER_RED;
		else if (!strcasecmp(argv[2], "blue"))
			u->flags |= USER_BLUE;
		else if (!strcasecmp(argv[2], "green"))
			u->flags |= USER_GREEN;
		else if (!strcasecmp(argv[2], "white"))
			u->flags |= USER_WHITE;
		else
			cio_out("unknown color [%s]\n", argv[2]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int fgoogle_hook(int argc, char *argv[])
{
	fgoogle(argv[1]);
	return 0;
}

static int google_hook(int argc, char *argv[])
{
	google(argv[1]);
	return 0;
}

static int part_hook(int argc, char *argv[])
{
	channel_t *ch = find_channel(argv[1], irc_pick_server());
	if (!ch)
		return 1;
	ch->part(ch, argc > 2 ? argv[2] : NULL);

	return 0;
}

static int queue_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	switch (argc) {
	case 2:
		if (u->queue == NULL)
			cio_out("user %s has no queued message\n", u->name);
		else
			cio_out("[%s] is queued for user %s\n", u->queue,
				u->name);
		break;
	case 3:
		u->set_queue(u, argv[2]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int unqueue_hook(int argc, char *argv[])
{
	user_t *u = find_user(argv[1], irc_pick_server());
	u->set_queue(u, NULL);
	return 0;
}

static int query_hook(int argc, char *argv[])
{
	switch (argc) {
	case 1:
		do_unquery();
		break;
	case 2:
		do_query(argv[1]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int topic_hook(int argc, char *argv[])
{
	const char *channel = natural_channel_name();
	switch (argc) {
	case 1:
		irc_out("TOPIC %s\n", channel);
		break;
	case 2:
		irc_out("TOPIC %s\n", argv[1]);
		break;
	case 3:
		irc_out("TOPIC %s :%s\n", argv[1], argv[2]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int kick_hook(int argc, char *argv[])
{
	switch (argc) {
	case 3:
		irc_out("KICK %s %s\n", argv[1], argv[2]);
		break;
	case 4:
		irc_out("KICK %s %s :%s\n", argv[1], argv[2], argv[3]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int away_hook(int argc, char *argv[])
{
	away(1);
	return 0;
}

static int back_hook(int argc, char *argv[])
{
	away(0);
	return 0;
}

static int strict_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED,
		       "/strict is deprecated and will be removed soon\n");
	io_colored_out(USER_WHITE,
		       "please set strict_rfc instead [/help set]\n");
	return 0;
}

static int wall_hook(int argc, char *argv[])
{
	assert(argc == 2);

	wall(argv[1]);

	return 0;
}

static int names_hook(int argc, char *argv[])
{
	const char *channel = natural_channel_name();
	switch (argc) {
	case 1:
		if (!channel)
			break;
		irc_out("NAMES %s\n", channel);
		break;
	case 2:
		irc_out("NAMES %s\n", argv[1]);
		break;
	default:
		assert(!"code should never get here");
	}
	return 0;
}

static int send_argv(int argc, char *argv[])
{
	size_t i;

	for (i = 0; i < argc; i++) {
		irc_out("%s ", argv[i]);
	}

	irc_out("\n");
	return 0;
}
static command_t commands[] = {
	{"connect", "connect [server] [port]", 1, 3, connect_hook,
	 "connects you to a server or sets your active server"
	 ", if no arguments are given it shows you what server you are active on"},
	{"nick", "nick <nickname>", 2, 2, nick_hook, "sets your nickname"},
	{"join", "join <channel>", 2, 2, join_hook,
	 "join a channel, or switch to active channel"},
	{"msg", "msg <nick> <message>", 3, 3, msg_hook,
	 "send a private message"},
	{"me", "me <action>", 2, 2, me_hook,
	 "looks like you performed an action in the channel"},
	{"ctcp", "ctcp <nick|channel> <ctcp message>", 3, 3, ctcp_hook,
	 "send a ctcp request"},
	{"ignore", "ignore <nick>", 2, 2, ignore_hook, "ignore somebody"},
	{"unignore", "unignore <nick>", 2, 2, unignore_hook,
	 "unignore somebody"},
	{"nignore", "nignore <nick>", 2, 2, nignore_hook, "nignore notices from somebody"},
	{"unnignore", "unnignore <nick>", 2, 2, unnignore_hook,
	 "unnignore notices from somebody"},
	{"huser", "huser <nick> [color]", 2, 3, huser_hook,
	 "highlight everything someone says [color], or turn highling off for them if [color] is not present"},
	{"fgoogle", "fgoogle <search terms>", 2, 2, fgoogle_hook,
	 "display a url on fuckinggoogleit.com that will search for you search terms"},
	{"google", "google <search terms>", 2, 2, google_hook,
	 "display a google url that searches for your search terms"},
	{"part", "part <channel>", 2, 3, part_hook, "leave a channel"},
	{"queue", "queue <nick> [message]", 2, 3, queue_hook,
	 "queue a private message to be delivered to <nick> as soon as they say something, helpful when you gotta leave and they are still idle"},
	{"unqueue", "unqueue <nick>", 2, 2, unqueue_hook,
	 "destroy a queued message"},
	{"query", "query [nick or channel]", 1, 2, query_hook,
	 "everything you say will be sent to [nick or channel], if [nick or channel] is absent, this disables the current query"},
	{"topic", "topic [channel] [topic string]", 1, 3, topic_hook,
	 "look at or set the topic in a channel"},
	{"kick", "kick <channel> <nick> [reason]", 3, 4, kick_hook,
	 "kick <nick> from <channel> because of [reason]"},
	{"away", "away", 1, 1, away_hook, "set away"},
	{"back", "back", 1, 1, back_hook,
	 "unset away and view msgs that contained your name or were sent to you while you were away"},
	{"strict", "strict", 1, 1, strict_hook,
	 "changes the behaviour of queue to send a NOTICE instead of a PRIVMSG"},
//      { "mode", "mode <mode> [user or channel]", 2, 3, mode_hook, 
//              "change somebody's mode, like if you want to op l0ser in your current channel, do /mode +o l0ser" },
	{"names", "names [channel]", 1, 2, names_hook,
	 "display a list of nicks in [channel] (current channel if absent)"},
	{"who", "who [channel]", 1, 2, names_hook, "see /help names"},
	{"wall", "wall <message>", 2, 2, wall_hook,
	 "send a message to all channels your in"},
	{NULL, NULL, 0, 0, NULL}
};

static plugin_t plugin = {
	.name = "irc_input",
	.version = "0.0.1",
	.description = "provides basic irc commands",
	.command_list = commands
};

REGISTER_PLUGIN(plugin, irc_input);
