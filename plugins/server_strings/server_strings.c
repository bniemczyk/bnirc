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

static int nick_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);
	user_t *u = find_user(nick, irc_pick_server());
	u->recv_nick(u, argv[2]);
	free(nick);
	return 0;
}

static int display_cdata(int argc, char *argv[])
{
	cio_out("%s\n", argv[argc - 1]);

	return 0;
}

static int privmsg_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);

	if (!nick)
		return -1;
	user_t *u = find_user(nick, irc_pick_server());
	free(nick);
	u->privmsg_print(u, argv[2], argv[3]);
	/* channel_msg(u->name, argv[2], irc_pick_server(), argv[3]); */
	return 0;
}

static int notice_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);
	size_t i;

	user_t *u = find_user(nick, irc_pick_server());
	assert(u != NULL);
	if (u->flags & USER_IGNORE || u->flags & USER_IGNORE_NOTICES)
		return 0;

	for (i = 0; i < strlen(argv[3]); i++)
		if (argv[3][i] == 2)
			remove_char_from_str(argv[3], i);

	if (argv[3][0] == 1)
		plugin_ctcp_in_hook(nick, argv[2], argv[3], irc_pick_server());
	else if (argv[2][0] == '#') {
		channel_t *ch = find_channel(argv[2], irc_pick_server());
		if (!ch)
			return -1;
		wio_colored_out(ch->win,
				USER_GREEN,
				"*** NOTICE from %s: %s\n", nick, argv[3]);
	} else
		io_colored_out(USER_GREEN, "*** NOTICE from %s: %s\n", nick,
			       argv[3]);
	free(nick);
	return 0;
}

static int names_hook(int argc, char *argv[])
{
	char **my_argv;
	int my_argc;
	size_t i;
	user_t *u;

	make_argv(&my_argc, &my_argv, argv[5]);

	for (i = 0; i < my_argc; i++) {
		assert(my_argv[i] != NULL);
		if (my_argv[i][0] != '@' && my_argv[i][0] != '+') {
			u = find_user(my_argv[i], irc_pick_server());
			u->flags |= USER_IS_OP;
		} else {
			u = find_user(&my_argv[i][1], irc_pick_server());
		}
		assert(u);

		u->add_channel(u, argv[4]);
	}

	channel_t *ch = find_channel(argv[4], irc_pick_server());
	if (!ch)
		return -1;

	// wio_colored_out(ch->win, USER_RED, "[%s] ", argv[4]);
	// wio_colored_out(ch->win, USER_WHITE, "%s\n", argv[5]);
	return 0;
}

static int end_of_names_hook(int argc, char *argv[])
{
	const char **nicks = users_in_channel(argv[3]);
	channel_t *ch = find_channel(argv[3], irc_pick_server());
	int del_chn = 0;
	if(ch == NULL) {
		ch = new_channel(argv[3], irc_pick_server());
		del_chn = 1;
	}
	int j;

	/**************************************
	 * find the longest nick
	 **************************************/
	int longest = 0;
	for(j = 0; nicks[j] != NULL; j++) {
		if(strlen(nicks[j]) > longest)
			longest = strlen(nicks[j]);
	}

	int sizex = io_get_width();
	int times = sizex / (longest + 4);

	/**************************************
	 * show the nicks
	 **************************************/
	wio_colored_out(ch->win, USER_BLUE, " [%s]\n", argv[3]);
	if(nicks[1] != NULL) {
		for(j = 0; nicks[j] != NULL; j++) {
			if(j && j % times == 0)
				wio_out(ch->win, "\n");
			wio_colored_out(ch->win, USER_RED, " [");
			wio_colored_out(ch->win, USER_WHITE, "%-*s", longest, nicks[j]);
			wio_colored_out(ch->win, USER_RED, "] ");
		}
		wio_out(ch->win, "\n");
	}

	free(nicks);

	if(del_chn)
		delete_channel(ch);
	return 0;

}

static int join_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);

/* @DISABLED@
	window_t w = channel_get_window(argv[2]);

	if(current_username && strcasecmp(current_username, nick)) {
		channel_add_flags(argv[2], CHANNEL_JOIN);
		strncpy(channels[w], argv[2], 255);
		channels[w][255] = 0;
	}

	channel_set_server(argv[2], irc_pick_server());

	user_channel(nick, argv[2]);
	cwio_out(channel_get_window(argv[2]), "%s %s is now in %s\n", timestamp(), nick, argv[2]);
 @DISABLED@ */

	channel_t *ch = find_channel(argv[2], irc_pick_server());
	if (!ch)
		return -1;
	user_t *u = find_user(nick, irc_pick_server());
	assert(u);
	free(nick);
	u->add_channel(u, ch->name);

    if(bool_option("show_joins")) {
	    cwio_out(ch->win, "%s %s is now in %s [%s]\n", timestamp(), u->name,
		    ch->name, u->server);
    }

	if (bool_option("about_channels")) {
		/**************************************
		 * check if we are using the right
		 * channel name, if not, fix it
		 * this really is kind of a hack
		 **************************************/
		if (strcmp(ch->name, argv[2])) {
			free(ch->name);
			ch->name = copy_string(argv[2]);
		}
	}

	return 0;
}

static int channelmode_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);
	channel_t *ch = find_channel(argv[2], irc_pick_server());
	if (!ch)
		return -1;
	cwio_out(ch->win, "%s sets %s %s in %s [%s]\n",
		 nick, argv[4], argv[3], ch->name, ch->server);
	free(nick);
	return 0;
}

static int mode_hook(int argc, char *argv[])
{
	cio_out("mode %s %s\n", argv[3], argv[2]);
	return 0;
}

static int subject_hook(int argc, char *argv[])
{
	channel_t *ch = find_channel(argv[3], irc_pick_server());
	if (!ch)
		return -1;
	wio_colored_out(ch->win, USER_RED, "[%s] ", argv[3]);
	wio_colored_out(ch->win, USER_WHITE, "%s\n", argv[4]);
	return 0;
}

static int whois_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "[%s] Real Name: %s\n", argv[3], argv[4]);
	io_colored_out(USER_WHITE, "[%s] Host Name: %s\n", argv[3], argv[5]);
	io_colored_out(USER_WHITE, "[%s] User String: %s\n", argv[3], argv[7]);
	return 0;
}

static int user_channels_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "[%s] Channels: %s\n", argv[3], argv[4]);
	return 0;
}

static int user_info_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s\n", argv[4]);
	return 0;
}

static int unknown_command_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] %s\n", argv[3], argv[4]);
	return 0;
}

static int quit_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);
	user_t *u = find_user(nick, irc_pick_server());
	u->print_relevant_wins(u, "%s %s has quit [%s]\n", timestamp(), u->name,
			       argv[2]);
	if (u->flags == 0)
		delete_user(u);
	else
		list_init(&(u->chlist));
	free(nick);
	return 0;
}

static int part_hook(int argc, char *argv[])
{
	char *nick = grab_nick(argv[0]);
	user_t *u = find_user(nick, irc_pick_server());
	channel_t *ch = find_channel(argv[2], irc_pick_server());
	if (!ch)
		return -1;

    if(bool_option("show_parts")) {
	    cwio_out(ch->win, "%s %s has left %s [%s]\n",
		    timestamp(), u->name, ch->name, argv[3]);
    }

	u->rm_channel(u, ch->name);
	free(nick);
	return 0;
}

/*
 * why the fuck does it finish with a timestamp of the current time?
 */
static int topic_time_hook(int argc, char *argv[])
{
	channel_t *ch = find_channel(argv[3], irc_pick_server());
	if (!ch)
		return -1;
	wio_colored_out(ch->win, USER_RED, "[%s] ", argv[3]);
	wio_colored_out(ch->win, USER_WHITE, "topic by %s\n", argv[4]);
	return 0;
}

static int kick_hook(int argc, char *argv[])
{
	channel_t *ch = find_channel(argv[2], irc_pick_server());
	if (!ch)
		return -1;
	user_t *u = find_user(argv[3], irc_pick_server());
	char *nick = grab_nick(argv[0]);
	cwio_out(ch->win, "%s %s was kicked from %s by %s [%s]\n", timestamp(),
		 argv[3], argv[2], nick, argv[4]);
	u->rm_channel(u, ch->name);
	free(nick);
	return 0;
}

static int nick_away_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "is away [%s]\n", argv[4]);
	return 0;
}

static int rpl_whoisuser_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s@%s [%s]\n", argv[4], argv[5], argv[7]);
	return 0;
}

static int rpl_whoisserver_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "is on server %s [%s]\n", argv[4], argv[5]);
	return 0;
}

static int rpl_whoisoperator_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "is an IRC operator\n");
	return 0;
}

static int rpl_whoisidle_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "idle %s seconds\n", argv[4]);
	return 0;
}

static int rpl_whoischannels_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s\n", argv[4]);
	return 0;
}

static int dummy_hook(int argc, char *argv[])
{
	return 0;
}

static int rpl_channelmodes_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s [%s]\n", argv[4], argv[5]);
	return 0;
}

static int rpl_notopic_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "has no topic\n");
	return 0;
}

static int rpl_inviting_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "inviting %s to %s\n", argv[4], argv[3]);
	return 0;
}

static int rpl_summoning_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "summoning %s to IRC\n", argv[3]);
	return 0;
}

static int rpl_version_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "%s %s [%s]\n", argv[3], argv[4], argv[5]);
	return 0;
}

static int rpl_who_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "%s is in %s\n", argv[7], argv[3]);
	return 0;
}

static int rpl_links_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s [%s]\n", argv[4], argv[5]);
	return 0;
}

static int rpl_banlist_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "%s has been banned from %s\n", argv[4],
		       argv[3]);
	return 0;
}

static int rpl_rehashing_hook(int argc, char *argv[])
{
	io_colored_out(USER_WHITE, "rehashing %s\n", argv[3]);
	return 0;
}

static int rpl_whoisinfo_hook(int argc, char *argv[])
{
	io_colored_out(USER_RED, "[%s] ", argv[3]);
	io_colored_out(USER_WHITE, "%s\n", argv[4]);
	return 0;
}

static int generic_trace_hook(int argc, char *argv[])
{
	int i;
	cio_out("%s", argv[3]);
	for (i = 4; i < argc; i++) {
		cio_out(" %s", argv[i]);
	}
	io_out("\n");
	return 0;
}

static int generic_mode_hook(int argc, char *argv[])
{
	int i;
	char *doer = grab_nick(argv[0]);

	if (argc > 2 && argv[2][0] == '#') {
		channel_t *ch = find_channel(argv[2], irc_pick_server());
		if (!ch) {
			free(argv[0]);
			return -1;
		}

		cwio_out(ch->win, "%s", argv[2]);
		for (i = 3; i < argc; i++)
			cwio_out(ch->win, " %s ", argv[i]);
		cwio_out(ch->win, " by %s\n", doer);
	} else {
		generic_trace_hook(argc, argv);
	}

	free(doer);
	return 0;
}

static int bad_nick(int argc, char *argv[])
{
	char *nick = copy_string(argv[3]);
	user_t *u = find_user(nick, irc_pick_server());
	int len = strlen(nick);
	int i;

	for (i = len - 1; i >= 0; i--) {
		if (nick[i] != '_') {
			nick[i] = '_';
			break;
		}
	}

	if (i < 0) {
		cio_out("could not set nick\n");
		return -1;
	}

	irc_nick(nick);
	u->recv_nick(u, nick);
	return 0;
}

static server_string_t strings[] = {
	/*
	 * command responses (non-error)
	 */
	{"302", 1, 4, display_cdata},
	{"303", 1, 4, display_cdata},
	{"301", 1, 5, nick_away_hook},
	{"305", 1, 4, display_cdata},
	{"306", 1, 4, display_cdata},
	{"311", 1, 8, rpl_whoisuser_hook},
	{"312", 1, 6, rpl_whoisserver_hook},
	{"313", 1, 5, rpl_whoisoperator_hook},
	{"317", 1, 6, rpl_whoisidle_hook},
	{"318", 1, 5, dummy_hook},
	{"319", 1, 5, rpl_whoischannels_hook},
	{"314", 1, 8, rpl_whoisuser_hook},
	{"369", 1, 5, dummy_hook},
	{"321", 1, 5, dummy_hook},	/// RPL_LISTSTART "Channel :Users Name" <--- wtf does that mean?
	{"322", 1, 6, dummy_hook},	/// RPL_LIST "<channel> <# visible> :<topic> ?
	{"323", 1, 4, dummy_hook},
	{"324", 1, 7, generic_mode_hook},
	{"331", 1, 5, rpl_notopic_hook},
	{"332", 1, 5, subject_hook},
	{"341", 1, 5, rpl_inviting_hook},
	{"342", 1, 5, rpl_summoning_hook},
	{"351", 1, 6, rpl_version_hook},
	{"352", 1, 10, rpl_who_hook},
	{"315", 1, 5, dummy_hook},	// End of /WHO reply
	{"353", 1, 6, names_hook},
	{"366", 1, 5, end_of_names_hook},	// End of /NAMES reply
	{"364", 1, 6, rpl_links_hook},
	{"365", 1, 5, dummy_hook},	// End of /LINKS reply
	{"367", 1, 5, rpl_banlist_hook},
	{"368", 1, 5, dummy_hook},	// End of banlist
	{"371", 1, 4, display_cdata},	// reply to /INFO
	{"374", 1, 4, dummy_hook},	// end of /INFO reply
	{"375", 1, 4, display_cdata},	// /MOTD
	{"372", 1, 4, display_cdata},	// /MOTD
	{"376", 1, 4, dummy_hook},	// end of /MOTD
	{"381", 1, 4, display_cdata},
	{"382", 1, 5, rpl_rehashing_hook},
	{"391", 1, 5, display_cdata},	// /TIME
	{"392", 1, 4, display_cdata},
	{"394", 1, 4, display_cdata},
	{"395", 1, 4, display_cdata},
	{"200", 0, 7, generic_trace_hook},
	{"201", 0, 6, generic_trace_hook},
	{"202", 0, 6, generic_trace_hook},
	{"203", 0, 6, generic_trace_hook},
	{"204", 0, 6, generic_trace_hook},
	{"205", 0, 6, generic_trace_hook},
	{"206", 0, 9, generic_trace_hook},
	{"208", 0, 6, generic_trace_hook},
	{"261", 0, 6, generic_trace_hook},
	{"211", 0, 10, generic_trace_hook},
	{"212", 0, 5, generic_trace_hook},
	{"213", 0, 9, generic_trace_hook},
	{"214", 0, 9, generic_trace_hook},
	{"215", 0, 9, generic_trace_hook},
	{"216", 0, 9, generic_trace_hook},
	{"218", 0, 8, generic_trace_hook},
	{"219", 1, 5, dummy_hook},	// end of /STATS report
	{"241", 0, 8, generic_trace_hook},
	{"242", 1, 4, display_cdata},
	{"243", 0, 7, generic_trace_hook},
	{"244", 0, 7, generic_trace_hook},
	{"221", 0, 4, generic_trace_hook},
	{"251", 1, 4, display_cdata},
	{"252", 1, 5, generic_trace_hook},
	{"253", 1, 5, generic_trace_hook},
	{"254", 1, 5, generic_trace_hook},
	{"255", 1, 4, display_cdata},
	{"256", 1, 5, generic_trace_hook},
	{"257", 1, 4, display_cdata},
	{"258", 1, 4, display_cdata},
	{"259", 1, 4, display_cdata},

	/* not in the RFC, but freenode uses these */
	{"001", 1, 4, display_cdata},
	{"002", 1, 4, display_cdata},
	{"003", 1, 4, display_cdata},
	{"004", 1, 4, display_cdata},
	{"005", 1, 4, display_cdata},
	{"265", 1, 4, display_cdata},
	{"266", 1, 4, display_cdata},
	{"250", 1, 4, display_cdata},
	{"320", 1, 5, rpl_whoisinfo_hook},
	{"329", 1, 4, dummy_hook},

	/*
	 * error replies
	 */
	{"401", 1, 5, display_cdata},
	{"402", 1, 5, display_cdata},
	{"403", 1, 5, display_cdata},
	{"404", 1, 5, display_cdata},
	{"405", 1, 5, display_cdata},
	{"406", 1, 5, display_cdata},
	{"407", 1, 5, display_cdata},
	{"409", 1, 5, display_cdata},
	{"411", 1, 4, display_cdata},
	{"412", 1, 4, display_cdata},
	{"413", 1, 5, display_cdata},
	{"414", 1, 5, display_cdata},
	{"421", 1, 5, unknown_command_hook},
	{"422", 1, 4, display_cdata},
	{"423", 1, 5, display_cdata},
	{"424", 1, 4, display_cdata},
	{"431", 1, 4, display_cdata},
	{"432", 1, 5, display_cdata},
	{"433", 1, 5, bad_nick},
	{"436", 1, 5, display_cdata},
	{"441", 1, 6, display_cdata},
	{"442", 1, 5, display_cdata},
	{"443", 1, 6, display_cdata},
	{"444", 1, 5, display_cdata},
	{"445", 1, 4, display_cdata},
	{"446", 1, 4, display_cdata},
	{"451", 1, 4, display_cdata},
	{"461", 1, 5, display_cdata},
	{"462", 1, 4, display_cdata},
	{"463", 1, 4, display_cdata},
	{"464", 1, 4, display_cdata},
	{"465", 1, 4, display_cdata},
	{"467", 1, 5, display_cdata},
	{"471", 1, 5, display_cdata},
	{"472", 1, 5, display_cdata},
	{"473", 1, 5, display_cdata},
	{"474", 1, 5, display_cdata},
	{"475", 1, 5, display_cdata},
	{"477", 1, 5, display_cdata},
	{"481", 1, 4, display_cdata},
	{"482", 1, 5, display_cdata},
	{"483", 1, 4, display_cdata},
	{"491", 1, 4, display_cdata},
	{"501", 1, 4, display_cdata},
	{"502", 1, 4, display_cdata},
	{"632", 1, 4, display_cdata}, 	// ircops

	// used for cookie response on at 
	// least irc.umich.edu, possibly other servers too
	{"998", 1, 3, display_cdata},

	/* commands */
	{"nick", 1, 3, nick_hook},
	{"join", 1, 3, join_hook},
	{"quit", 1, 3, quit_hook},
	{"part", 1, 4, part_hook},
	{"kick", 1, 5, kick_hook},
	{"privmsg", 1, 4, privmsg_hook},
	{"mode", 0, 5, generic_mode_hook},
	{"notice", 1, 4, notice_hook},

	{NULL, 0, 0, NULL}
};

static plugin_t plugin = {
	.name = "server_strings",
	.version = "0.0.1",
	.description =
	    "supplies basic actions for reciept on message from server",
	.server_string_list = strings
};

REGISTER_PLUGIN(plugin, bnirc_server_strings)
