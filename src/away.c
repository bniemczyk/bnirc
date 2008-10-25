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

#include "irc.h"

typedef struct {
	list_t	header;
	char	*user;
	char	*chan;
	char	*cdata;
	char	*tstamp;
} away_t;

away_t head;

INIT_CODE(my_init)
{
	list_init(&head);
}

static 	void	away_display_msgs(void);

int is_away = 0;

void	away	( int a )
{
	is_away = a;

	if(a == 0) {
		away_display_msgs();
		if(bool_option("display_away"))
			cio_out("you are back\n");
	} else {
		if(bool_option("display_away"))
			cio_out("you are away\n");
	}
}

void	away_handle_msg		( const char *user, const char *chan, const char *cdata )
{
	if(is_away) {
		away_add_msg(user, chan, cdata);
		if(irc_get_current_nick() && !strcmp(chan, irc_get_current_nick()))
			irc_out("NOTICE %s :%s is currently away, your message will be queued for delivery\n", 
					user, irc_get_current_nick());
	}
}

void	away_add_msg	( const char *user, const char *chan, const char *cdata )
{
	if(!is_away)
		return;

	if(!cdata || !user || !chan)
		return;

	away_t *a = malloc(sizeof(away_t));
	a->user = malloc(strlen(user) + 1);
	a->chan = malloc(strlen(chan) + 1);
	a->cdata = malloc(strlen(cdata) + 1);

	strcpy(a->user, user);
	strcpy(a->chan, chan);
	strcpy(a->cdata, cdata);

	a->tstamp = timestamp();

	list_insert(head.header.prev, a);
}

static void	away_display_msgs	( void )
{
	away_t *i, *j;

	if(head.header.next == (list_t *)&head)
		return;

	cio_out("while you were away:\n");
	for(i = (away_t *)head.header.next; i != (away_t *)&head; list_next(i)) {
		cio_out("%s %s -> %s: %s\n", i->tstamp, i->user, i->chan, i->cdata);
		if(i->header.prev != (list_t *)&head) {
			j = (away_t *)(i->header.prev);
			list_remove(j);
			free(j->user);
			free(j->chan);
			free(j->cdata);
			free(j->tstamp);
			free(j);
		}
	}

	i = (away_t *)head.header.prev;
	list_remove(i);
	free(i->user);
	free(i->chan);
	free(i->cdata);
	free(i->tstamp);
	free(i);
}
