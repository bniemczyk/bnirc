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
#include "list.h"


static history_t	head;

INIT_CODE(my_init) {
	list_init(&head);
	cio_out("history initialized!\n");
}

void	add_history	( const char *str )
{
	if(!str)
		return;

	history_t *h = malloc(sizeof(history_t));
	list_init(h);
	list_insert(&head, h);

	h->str = strdup(str);
}

history_t	*get_next_history	( history_t *p )
{
	if(!p) 
		if(head.header.next == (list_t *)&head)
			return NULL;
		else
			return (history_t *)head.header.next;
	else
		list_next(p);

	if(p == &head)
		return NULL;

	return p;
}

history_t	*get_prev_history	( history_t *n )
{
	if(!n)
		return NULL;

	list_prev(n);
	
	if(n == &head)
		return NULL;

	return n;
}
