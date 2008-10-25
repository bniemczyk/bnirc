/**************************************
 * Copyright (C) 2005 Brandon Niemczyk
 *
 * Description:
 *
 * ChangeLog:
 *
 * License: GPL Version 2
 *
 * Todo:
 *
 **************************************/
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

#include "irc.h"

enum	{
	NOT_JOINED,
	JOIN_SENT,
	PART_SENT,
	JOINED
};

typedef struct {
	list_t		header;
	channel_oo_t	*ch;
} channel_oo_list_t;

static list_t	active_channels[255] = {{NULL, NULL}};

hash_t	channels_hash;

INIT_CODE(channels_init) {
	int i;
	hash_init(channels_hash);
	for(i = 0; i < 255; i++)
		list_init(&active_channels[i]);
	cio_out("channel.c initialized!\n");
}

void rejoin_channels ( const char *server )
{
	channel_t *i = NULL;
	for(i = hash_first(channels_hash, channel_t *); i != NULL; i = hash_next(channels_hash, i, channel_t *)) {
		if(!strcasecmp(server, i->server)) {
			irc_sout(server, "JOIN %s\n", i->name);
		}
	}
}

void	wall		( const char *msg )
{
	assert(msg);
	user_t *u;

	channel_t *i;
	for(i = hash_first(channels_hash, channel_t *); i != NULL; i = hash_next(channels_hash, i, channel_t *)) {
		if(i->flags & JOINED) {
			u = find_user(irc_server_nick(i->server), i->server);
			assert(u != NULL);
			irc_sout(i->server, "PRIVMSG %s :%s\n", i->name, msg);
			/* channel_msg(irc_server_nick(i->server), i->name, i->server, msg); */
			u->privmsg_print(u, i->name, msg);
		}
	}
}

static tablist_t *channel_tab_complete ( const char *partial )
{
	channel_t *i;
	tablist_t *tabs = TAB_INIT;

	for(i = hash_first(channels_hash, channel_t *); i != NULL;
			i = hash_next(channels_hash, i, channel_t *)) {
		if(strncasecmp(i->name, partial, strlen(partial)))
			continue;
		tablist_add_word(tabs, i->name);
	}

	return tabs;
}

INIT_CODE(add_channel_tabs) {
	add_tab_completes(channel_tab_complete);
}

static FILE * open_log_file ( const char *channel, const char *server )
{
	char *logdir = string_cat(2, getenv("HOME"), ".bnIRC/log");
	char *bnircdir = string_cat(2, getenv("HOME"), ".bnIRC");
	char *serverlogdir = string_cat(3, getenv("HOME"), ".bnIRC/log", server);
	char *logfile = string_cat(2, serverlogdir, &channel[1]);
	int i;

	for(i = 0; i < strlen(logdir); i++) {
		if(logdir[i] == ' ')
			logdir[i] = '/';
	}

	for(i = 0; i < strlen(bnircdir); i++) {
		if(bnircdir[i] == ' ')
			bnircdir[i] = '/';
	}

	for(i = 0; i < strlen(serverlogdir); i++) {
		if(serverlogdir[i] == ' ')
			serverlogdir[i] = '/';
	}

	for(i = 0; i < strlen(logfile); i++) {
		if(logfile[i] == ' ')
			logfile[i] = '/';
	}

	mkdir(bnircdir, 0777);
	mkdir(logdir, 0777);
	mkdir(serverlogdir, 0777);

	if(bool_option("verbose")) {
		cio_out("attempting to open logfile: %s\n", logfile);
	}

	FILE *f = fopen(logfile, "a");
	free(bnircdir);
	free(logdir);
	free(serverlogdir);
	free(logfile);

	return f;
}

static void log ( channel_oo_t *channel, const char *who, const char *msg )
{
	if(channel->logfile == NULL) {
		channel->logfile = open_log_file(channel->name, channel->server);
	}

	if(channel->logfile == NULL)
		return;

	fprintf(channel->logfile, "[%s] %s: %s\n", timestamp(), who, msg);
	fflush(channel->logfile);
}

static	void	remove_from_window	( channel_oo_t *ch )
{
	channel_oo_list_t *j, *i = (channel_oo_list_t *)active_channels[ch->win].next;
	assert(ch->win < 255);
	while(i != (channel_oo_list_t *)&active_channels[ch->win]) {
		// DPRINT("checking %p against %p\n", i, &active_channels[ch->win]);
		j = i;
		list_next(j);
		if(i->ch == ch)
			list_remove(i);
		i = j;
	}
}

static	void	set_channel_win		( channel_oo_t *ch, window_t w )
{
	remove_from_window(ch);
	ch->win = w;
	channel_oo_list_t *cl = malloc(sizeof(channel_oo_list_t));
	cl->ch = ch;
	list_insert(&active_channels[w], cl);
}

static	void	join	( channel_oo_t *ch )
{
	// char *key_option = string_cat(2, ch->name, "_key");
	char *key_option = malloc(strlen(ch->name) + 5);
	strcpy(key_option, ch->name);
	strcat(key_option, "_key");
	if(bool_option("verbose"))
		cio_out("key_option is %s\n", key_option);

	if(!bool_option(key_option))
		irc_sout(ch->server, "JOIN %s\n", ch->name);
	else {
		cio_out("attempting to join with key set by %s\n", key_option);
		irc_sout(ch->server, "JOIN %s %s\n", ch->name, string_option(key_option));
	}
	free(key_option);
	ch->state = JOIN_SENT;
	set_channel_win(ch, active_window);
}

static	void	part	( channel_oo_t *ch, const char *part_str  )
{
	const char *default_part_str = NULL;
	if(part_str)
		default_part_str = part_str;
	else if(bool_option("part_string"))
		default_part_str = string_option("part_string");
	irc_sout(ch->server, "PART %s :%s\n", ch->name, default_part_str != NULL ? default_part_str : "(none)");
	ch->state = PART_SENT;
	remove_from_window(ch);
}

static	void	recv_join	( channel_oo_t *ch )
{
	if(ch->state != JOIN_SENT)
		return;

	assert(ch);
	assert(ch->win >= 0);
	assert(ch->win < 255);

	channel_oo_list_t *cl = malloc(sizeof(channel_oo_list_t));
	cl->ch = ch;
	ch->state = JOINED;
	io_colored_out(USER_RED, "[joined] ");
	io_colored_out(USER_WHITE, "%s\n", ch->name);
}

static	int	is_active	( channel_oo_t *ch )
{
	channel_oo_list_t *cmp = (channel_oo_list_t *)active_channels[ch->win].next;
	if(cmp->ch == ch)
		return 1;
	return 0;
}

static	void	recv_part	( channel_oo_t *ch )
{
	if(ch->state != JOINED)
		return;

	assert(ch);
	assert(ch->win >= 0);
	assert(ch->win < 255);

	ch->state = NOT_JOINED;
}

void	construct_channel	( channel_oo_t *chan, const char *name, const char *server )
{
	int about = 0;
	if(bool_option("about_channels") && name[0] == '#' && name[1] == '#') {
		name++;
		about = 1;
	}

	char *key = malloc(strlen(name) + strlen(server) + 2);
	strcpy(key, name);
	strcat(key, ":");
	strcat(key, server);

	if(about)
		name--;

	chan->name = copy_string(name);
	chan->server = copy_string(server);
	chan->flags = 0;
	chan->state = NOT_JOINED;
	chan->part = part;
	chan->join = join;
	chan->recv_part = recv_part;
	chan->recv_join = recv_join;
	chan->set_window = set_channel_win;
	chan->is_active = is_active;
	chan->win = 0;
	chan->log = log;
	set_channel_win(chan, 0);
	chan->logfile = NULL;

	if(bool_option("verbose")) {
		cio_out("constructing channel with key [%s]\n", key);
	}

	hash_insert(channels_hash, chan, key);

	free(key);
}

void	destruct_channel	( channel_oo_t *chan )
{
	chan->part(chan, NULL);
	hash_remove(chan);

	if(chan->logfile != NULL) {
		fclose(chan->logfile);
	}

	free(chan->name);
	free(chan->server);
}

channel_oo_t	*new_channel	( const char *name, const char *server )
{

	channel_oo_t *ch = malloc(sizeof(channel_oo_t));
	construct_channel(ch, name, server);
	return ch;
}

void		delete_channel	( channel_oo_t *ch )
{
	destruct_channel(ch);
	free(ch);
}

channel_oo_t	*find_channel	( const char *name, const char *server )
{
	if(bool_option("about_channels") && name[0] == '#' && name[1]  == '#')
		name++;

	char *key = malloc(strlen(name) + strlen(server) + 2);
	strcpy(key, name);
	strcat(key, ":");
	strcat(key, server);


	if(bool_option("verbose"))
		cio_out("finding channel with key [%s]\n", key);

	channel_oo_t *ch = hash_find_nc(channels_hash, key, channel_oo_t *);

	free(key);
	return ch;
}

channel_oo_t	*active_channel	( void )
{
	window_t w = active_window;
	if(active_channels[w].next == &active_channels[w])
		return NULL;

	return ((channel_oo_list_t *)active_channels[w].next)->ch;
}

const char	*natural_channel_name	( void )
{
	channel_t *tmp = active_channel();
	if(!tmp)
		return NULL;
	return tmp->name;
}
